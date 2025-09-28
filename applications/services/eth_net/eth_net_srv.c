#include "eth_net.h"
#include "eth_net_srv.h"
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
    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN;
}

static EthNet* eth_net_alloc(void)
{
    EthNet* eth_net = malloc(sizeof(EthNet));
    eth_net->queue  = rhs_message_queue_alloc(1, sizeof(EthNetApiEventMessage));

    // Initialise Mongoose network stack
    ethernet_init();

    mg_log_set(MG_LL_DEBUG);     // Set log level
    mg_mgr_init(&eth_net->mgr);  // and attach it to the interface

    {
        static struct mg_tcpip_driver_stm32f_data driver_data_;
        static struct mg_tcpip_if                 mif_;

        // Initialize config with default values from compile-time macros
        EthNetConfig config = {.ip       = MG_TCPIP_IP,
                               .mask     = MG_TCPIP_MASK,
                               .gateway  = MG_TCPIP_GW,
                               .phy_addr = MG_TCPIP_PHY_ADDR,
                               .mdc_cr   = MG_DRIVER_MDC_CR};

        // Set default MAC address
        MG_SET_MAC_ADDRESS(config.mac);

        // Call user configuration function (weak)
        if (eth_net_set_config_on_startup)
        {
            eth_net_set_config_on_startup(&config);
        }

        // Apply configuration
        driver_data_.mdc_cr   = config.mdc_cr;
        driver_data_.phy_addr = config.phy_addr;
        mif_.ip               = config.ip;
        mif_.mask             = config.mask;
        mif_.gw               = config.gateway;
        mif_.driver           = &mg_tcpip_driver_stm32f;
        mif_.driver_data      = &driver_data_;

        // Copy MAC address
        for (int i = 0; i < 6; i++)
        {
            mif_.mac[i] = config.mac[i];
        }

        MG_ENABLE_ETH_IRQ();
        mg_tcpip_init(&eth_net->mgr, &mif_);
        MG_INFO(("Driver: stm32f, MAC: %M", mg_print_mac, mif_.mac));
    }
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
    EthNetApiEventMessage msg = {.lock = api_lock_alloc_locked(),
                                 .type = EthNetApiEventTypeSetTcp,
                                 .data = {.interface = {.uri = (char*) uri, .fn = fn, .context = context}}};
    rhs_message_queue_put(eth_net->queue, &msg, RHSWaitForever);
    api_lock_wait_unlock_and_free(msg.lock);
}

int32_t eth_net_service(void* context)
{
    EthNet* app = eth_net_alloc();
    rhs_record_create(RECORD_ETH_NET, app);

    EthNetApiEventMessage msg;
    MG_INFO(("Starting event loop"));
    for (;;)
    {
        mg_mgr_poll(&app->mgr, 1);  // Infinite event loop

        if (rhs_message_queue_get(app->queue, &msg, 0) == RHSStatusOk)
        {
            if (msg.type == EthNetApiEventTypeSetHttp)
            {
                mg_http_listen(&app->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                MG_INFO(("HTTP server started"));
            }
            else if (msg.type == EthNetApiEventTypeSetTcp)
            {
                mg_listen(&app->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                MG_INFO(("Starting TCP server on %d", msg.data.interface.uri));
            }

            api_lock_unlock(msg.lock);
        }
    }
}

bool mg_random(void* buf, size_t len)
{  // Use on-board RNG
    rhs_hal_random_fill_buf(buf, len);
    return true;
}
