#include "eth_net.h"
#include "eth_net_srv.h"
#include "eth_net_listeners.h"
#include "hal.h"

static void ethernet_init(void)
{
// Initialise Ethernet. Enable MAC GPIO pins for STM32F407
#if defined(STM32F407xx) || defined(STM32F765xx)
    uint16_t pins[] = {PIN('A', 1),   // ETH_RMII_REF_CLK
                       PIN('A', 2),   // ETH_RMII_MDIO
                       PIN('A', 7),   // ETH_RMII_CRS_DV
                       PIN('B', 11),  // ETH_RMII_TX_EN
                       PIN('B', 12),  // ETH_RMII_TXD0
                       PIN('B', 13),  // ETH_RMII_TXD1
                       PIN('C', 1),   // ETH_RMII_MDC
                       PIN('C', 4),   // ETH_RMII_RXD0
                       PIN('C', 5)};  // ETH_RMII_RXD1
#else
#    error "Please define Ethernet pins for your STM32 model"
#endif
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        gpio_init(pins[i],
                  MG_GPIO_MODE_AF,
                  MG_GPIO_OTYPE_PUSH_PULL,
                  MG_GPIO_SPEED_INSANE,
                  MG_GPIO_PULL_NONE,
                  11);  // 11 is the Ethernet function
    }
    NVIC_EnableIRQ(ETH_IRQn);                // Setup Ethernet IRQ handler
    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;  // Use RMII. Goes first!

    // Enable Ethernet MAC clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN;

    // Perform hardware reset of Ethernet MAC peripheral
    RCC->AHB1RSTR |= RCC_AHB1RSTR_ETHMACRST;

    // Wait until reset is applied
    while ((RCC->AHB1RSTR & RCC_AHB1RSTR_ETHMACRST) == 0)
        ;

    // Release reset
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETHMACRST;

    // Wait until reset is released and MAC registers return to default values
    while ((RCC->AHB1RSTR & RCC_AHB1RSTR_ETHMACRST) != 0 || (ETH->MACCR & 0x00008000) == 0)
        ;
}

static void ethernet_deinit(void)
{
    // Disable Ethernet IRQ handler
    NVIC_DisableIRQ(ETH_IRQn);

    // Disable Ethernet MAC clocks
    RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN);
    SYSCFG->PMC &= ~SYSCFG_PMC_MII_RMII_SEL;  // Use RMII. Goes first!

    // Reset GPIO pins to default state
#if defined(STM32F407xx) || defined(STM32F765xx)
    uint16_t pins[] = {PIN('A', 1),   // ETH_RMII_REF_CLK
                       PIN('A', 2),   // ETH_RMII_MDIO
                       PIN('A', 7),   // ETH_RMII_CRS_DV
                       PIN('B', 11),  // ETH_RMII_TX_EN
                       PIN('B', 12),  // ETH_RMII_TXD0
                       PIN('B', 13),  // ETH_RMII_TXD1
                       PIN('C', 1),   // ETH_RMII_MDC
                       PIN('C', 4),   // ETH_RMII_RXD0
                       PIN('C', 5)};  // ETH_RMII_RXD1

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        gpio_init(pins[i],
                  MG_GPIO_MODE_INPUT,
                  MG_GPIO_OTYPE_PUSH_PULL,
                  MG_GPIO_SPEED_LOW,
                  MG_GPIO_PULL_NONE,
                  0);  // No alternate function
    }
#endif
}

static void eth_net_init_tcpip(EthNet* eth_net)
{
    struct mg_tcpip_if*                 ifp         = eth_net->ifp;
    struct mg_tcpip_driver_stm32f_data* driver_data = (struct mg_tcpip_driver_stm32f_data*) eth_net->driver_data;
    rhs_assert(eth_net && ifp && driver_data);

    // Clear interface and driver data structures
    memset(eth_net->ifp, 0, sizeof(struct mg_tcpip_if));
    memset(eth_net->driver_data, 0, sizeof(struct mg_tcpip_driver_stm32f_data));

    // Set default MAC address
    MG_SET_MAC_ADDRESS(config.mac);

    // Call user configuration function (weak)
    if (eth_net_set_config_on_startup)
    {
        eth_net_set_config_on_startup(&eth_net->config);
    }

    // Apply configuration
    driver_data->mdc_cr   = eth_net->config.mdc_cr;
    driver_data->phy_addr = eth_net->config.phy_addr;
    ifp->ip               = eth_net->config.ip;
    ifp->mask             = eth_net->config.mask;
    ifp->gw               = eth_net->config.gateway;
    ifp->driver           = &mg_tcpip_driver_stm32f;
    ifp->driver_data      = driver_data;

    // Copy MAC address
    for (int i = 0; i < 6; i++)
    {
        eth_net->ifp->mac[i] = eth_net->config.mac[i];
    }

    // Initialize TCP/IP interface
    mg_tcpip_init(eth_net->mgr, ifp);
    MG_INFO(("Driver: stm32f, MAC: %M", mg_print_mac, ifp->mac));
}

static EthNet* eth_net_alloc(void)
{
    EthNet* eth_net      = malloc(sizeof(EthNet));
    eth_net->cli         = rhs_record_open(RECORD_CLI);
    eth_net->mgr         = malloc(sizeof(struct mg_mgr));
    eth_net->ifp         = malloc(sizeof(struct mg_tcpip_if));
    eth_net->driver_data = malloc(sizeof(struct mg_tcpip_driver_stm32f_data));
    eth_net->thread      = rhs_thread_get_current();
    eth_net->queue       = rhs_message_queue_alloc(3, sizeof(EthNetApiEventMessage));
    eth_net->listeners   = NULL;  // Initialize listeners list

    // Initialize config with default values from compile-time macros
    eth_net->config.ip       = MG_TCPIP_IP;
    eth_net->config.mask     = MG_TCPIP_MASK;
    eth_net->config.gateway  = MG_TCPIP_GW;
    eth_net->config.phy_addr = MG_TCPIP_PHY_ADDR;
    eth_net->config.mdc_cr   = MG_DRIVER_MDC_CR;

    // Initialise Mongoose network stack
    ethernet_init();

    mg_log_set(MG_LL_DEBUG);    // Set log level
    mg_mgr_init(eth_net->mgr);  // and attach it to the interface

    eth_net_init_tcpip(eth_net);

    return eth_net;
}

void eth_net_start_http(EthNet* eth_net, const char* uri, mg_event_handler_t fn, void* context)
{
    EthNetApiEventMessage msg = {.lock = api_lock_alloc_locked(),
                                 .type = EthNetApiEventTypeSetHttp,
                                 .data = {.interface = {.uri = (char*) uri, .fn = fn, .context = context}}};
    rhs_message_queue_put(eth_net->queue, &msg, RHSWaitForever);
    api_lock_wait_unlock_and_free(msg.lock);
}

void eth_net_start_listener(EthNet* eth_net, const char* uri, mg_event_handler_t fn, void* context)
{
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != eth_net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    EthNetApiEventMessage msg = {.lock = lock,
                                 .type = EthNetApiEventTypeSetTcp,
                                 .data = {.interface = {.uri = (char*) uri, .fn = fn, .context = context}}};

    rhs_message_queue_put(eth_net->queue, &msg, RHSWaitForever);

    if (thread != eth_net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void eth_net_stop_listener(EthNet* eth_net, const char* uri)
{
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != eth_net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    EthNetApiEventMessage msg = {.lock = lock,
                                 .type = EthNetApiEventTypeRstTcp,
                                 .data = {.interface = {.uri = (char*) uri}}};

    rhs_message_queue_put(eth_net->queue, &msg, RHSWaitForever);

    if (thread != eth_net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void eth_net_set_config(EthNet* eth_net, EthNetConfig* config)
{
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != eth_net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    EthNetApiEventMessage msg = {.lock = lock, .type = EthNetApiEventTypeRestart, .data = {.config = *config}};
    rhs_message_queue_put(eth_net->queue, &msg, RHSWaitForever);

    if (thread != eth_net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void eth_net_get_config(EthNet* eth_net, EthNetConfig* config)
{
    if (eth_net == NULL || config == NULL)
        return;

    // Copy current configuration to the provided config structure
    config->ip      = eth_net->mgr->ifp->ip;
    config->mask    = eth_net->mgr->ifp->mask;
    config->gateway = eth_net->mgr->ifp->gw;
}

void eth_net_cli_command(char* args, void* context)
{
    EthNet* eth_net = (EthNet*) context;
    if (args == NULL)
    {
        return;
    }
    else
    {
        char* separator = strchr(args, ' ');
        if (separator == NULL || *(separator + 1) == 0)
        {
            if (strstr(args, "-restart") == args)
            {
                // Restart network manager
                eth_net_set_config(eth_net, &eth_net->config);
                return;
            }
            printf("Invalid argument\n");
            return;
        }
        else if (strstr(args, "-ip") == args)
        {
            // Parse IP address from string like "192.168.1.100"
            char*        ip_str = separator + 1;
            unsigned int a, b, c, d;

            if (string_to_ip(ip_str, &a, &b, &c, &d) == 0)
            {
                // Update IP address in config
                eth_net->config.ip = MG_IPV4(a, b, c, d);

                printf("IP address will be changed to %u.%u.%u.%u\n", a, b, c, d);
                printf("Restarting network manager...\n");

                // Restart network manager to apply new IP
                eth_net_set_config(eth_net, &eth_net->config);
                return;
            }

            printf("Invalid IP address format. Expected: eth -ip 192.168.1.100\n");
            return;
        }
        printf("Invalid argument\n");
    }
}

int32_t eth_net_service(void* context)
{
    EthNet* app = eth_net_alloc();
    rhs_record_create(RECORD_ETH_NET, app);
    cli_add_command(app->cli, "eth", eth_net_cli_command, app);

    EthNetApiEventMessage msg;
    MG_INFO(("Starting event loop"));
    for (;;)
    {
        mg_mgr_poll(app->mgr, 1);  // Infinite event loop

        if (rhs_message_queue_get(app->queue, &msg, 0) == RHSStatusOk)
        {
            if (msg.type == EthNetApiEventTypeSetHttp)
            {
                // Example of mDNS
                // uint32_t response_ip = app->mgr->ifp->ip;
                // struct mg_connection* c = mg_mdns_listen(app->mgr, "sr_yahoo");
                // if (c != NULL)
                // {
                //     memcpy(c->data, &response_ip, sizeof(response_ip));
                //     MG_INFO(("mDNS responder started"));
                // }
                mg_http_listen(app->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                MG_INFO(("HTTP server started on %s", msg.data.interface.uri));

                // Add listener to the buffer for later restoration
                eth_net_listeners_add(&app->listeners,
                                      EthNetListenerTypeHttp,
                                      msg.data.interface.uri,
                                      msg.data.interface.fn,
                                      msg.data.interface.context);
            }
            else if (msg.type == EthNetApiEventTypeSetTcp)
            {
                mg_listen(app->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                MG_INFO(("TCP server started on %s", msg.data.interface.uri));

                // Add listener to the buffer for later restoration
                eth_net_listeners_add(&app->listeners,
                                      EthNetListenerTypeTcp,
                                      msg.data.interface.uri,
                                      msg.data.interface.fn,
                                      msg.data.interface.context);
            }
            else if (msg.type == EthNetApiEventTypeRstTcp)
            {
                struct mg_connection* c;
                struct mg_addr        target_addr;

                uint16_t port = mg_url_port(msg.data.interface.uri);

                // Parse the target URL to get address
                if (mg_aton(mg_url_host(msg.data.interface.uri), &target_addr))
                {
                    // Close all connections matching this listener address
                    for (c = app->mgr->conns; c != NULL; c = c->next)
                    {
                        // Check if this is a listening connection with matching address
                        if (c->is_listening && c->loc.port == mg_htons(port))
                        {
                            MG_INFO(("ip %s, port %d", c->loc.ip, mg_htons(c->loc.port)));
                            c->is_closing = 1;
                        }
                    }

                    // Process the closing - this will actually close the sockets
                    mg_mgr_poll(app->mgr, 0);

                    // Remove listener from the stored list
                    if (eth_net_listeners_remove(&app->listeners, msg.data.interface.uri) != true)
                    {
                        MG_ERROR(("Listener not found in stored list: %s", msg.data.interface.uri));
                    }
                }
                else
                {
                    MG_ERROR(("invalid listening URL: %s", msg.data.interface.uri));
                }
            }
            else if (msg.type == EthNetApiEventTypeRestart)
            {
                struct mg_connection* c;
                app->mgr->ifp->ip   = msg.data.config.ip;
                app->mgr->ifp->mask = msg.data.config.mask;
                app->mgr->ifp->gw   = msg.data.config.gateway;

                // Close all existing connections
                for (c = app->mgr->conns; c != NULL; c = c->next)
                    c->is_closing = 1;
                mg_mgr_poll(app->mgr, 0);

                // Restore all registered listeners
                MG_INFO(("Restoring listeners..."));
                for (EthNetListener* listener = app->listeners; listener != NULL; listener = listener->next)
                {
                    if (listener->type == EthNetListenerTypeHttp)
                    {
                        mg_http_listen(app->mgr, listener->uri, listener->fn, listener->context);
                        MG_INFO(("Restored HTTP server on %s", listener->uri));
                    }
                    else if (listener->type == EthNetListenerTypeTcp)
                    {
                        mg_listen(app->mgr, listener->uri, listener->fn, listener->context);
                        MG_INFO(("Restored TCP server on %s", listener->uri));
                    }
                }
            }

            if (msg.lock)
                api_lock_unlock(msg.lock);
        }
    }
}

bool mg_random(void* buf, size_t len)
{  // Use on-board RNG
    rhs_hal_random_fill_buf(buf, len);
    return true;
}
