#include "eth_net.h"
#include "rhs.h"
#include "rhs_hal.h"
#include "cli.h"
#include "mongoose.h"
#include "net_i.h"

#define UUID ((uint8_t*) UID_BASE)  // Unique 96-bit chip ID. TRM 39.1

typedef struct
{
    Cli* cli;
    Net* net;
} EthNet;

static EthNet* eth;

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

static void eth_net_init_tcpip(Net* net, const EthPhyConfig* phy_config)
{
    struct mg_tcpip_if*                 ifp    = malloc(sizeof(struct mg_tcpip_if));
    struct mg_tcpip_driver_stm32f_data* driver = malloc(sizeof(struct mg_tcpip_driver_stm32f_data));
    rhs_assert(net && ifp && driver);

    // Clear interface and driver data structures
    memset(ifp, 0, sizeof(struct mg_tcpip_if));
    memset(driver, 0, sizeof(struct mg_tcpip_driver_stm32f_data));

    // Apply Ethernet-specific PHY configuration (use provided values or defaults)
    driver->mdc_cr   = phy_config ? phy_config->mdc_cr : MG_DRIVER_MDC_CR;
    driver->phy_addr = phy_config ? phy_config->phy_addr : MG_TCPIP_PHY_ADDR;
    ifp->ip          = net->config->ip;
    ifp->mask        = net->config->mask;
    ifp->gw          = net->config->gateway;
    ifp->driver      = &mg_tcpip_driver_stm32f;
    ifp->driver_data = driver;

    // Copy MAC address
    for (int i = 0; i < 6; i++)
    {
        ifp->mac[i] = net->config->mac[i];
    }

    // Initialize TCP/IP interface
    mg_tcpip_init(net->mgr, ifp);
    MG_INFO(("Driver: stm32f, MAC: %M", mg_print_mac, ifp->mac));
}

static EthNet* eth_net_alloc(const NetConfig* net_config, const EthPhyConfig* phy_config)
{
    EthNet* app      = malloc(sizeof(EthNet));
    app->net         = malloc(sizeof(struct Net));
    app->net->mgr    = malloc(sizeof(struct mg_mgr));
    app->net->config = malloc(sizeof(NetConfig));

    // Use provided base config if valid, otherwise use defaults from compile-time macros
    if (net_config != NULL && net_config->ip != 0 && net_config->mask != 0)
    {
        memcpy(app->net->config, net_config, sizeof(NetConfig));
    }
    else
    {
        unsigned int a, b, c, d;
        uint8_t      mac[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
        if (string_to_ip(ETH_NET_IP_STRING, &a, &b, &c, &d) == 0)
        {
            app->net->config->ip = MG_IPV4(a, b, c, d);
        }
        if (string_to_ip(ETH_NET_MASK_STRING, &a, &b, &c, &d) == 0)
        {
            app->net->config->mask = MG_IPV4(a, b, c, d);
        }
        if (string_to_ip(ETH_NET_GW_STRING, &a, &b, &c, &d) == 0)
        {
            app->net->config->gateway = MG_IPV4(a, b, c, d);
        }

        memcpy(app->net->config->mac, mac, sizeof(mac));
    }

    // Initialise Mongoose network stack
    ethernet_init();

    mg_mgr_init(app->net->mgr);  // and attach it to the interface

    eth_net_init_tcpip(app->net, phy_config);

    return app;
}

static void eth_net_cli_command(char* args, void* context)
{
    EthNet* eth = (EthNet*) context;
    if (args == NULL)
    {
        return;
    }
    else
    {
        char* separator = strchr(args, ' ');
        if (separator == NULL || *(separator + 1) == 0)
        {
            if (strstr(args, "--restart") == args)
            {
                // Restart network manager
                // net_set_config(eth->net, &eth->net->config);
                return;
            }
            printf("Invalid argument\n");
            return;
        }
        else if (strstr(args, "--ip") == args)
        {
            // Parse IP address from string like "192.168.1.100"
            char*        ip_str = separator + 1;
            unsigned int a, b, c, d;

            if (string_to_ip(ip_str, &a, &b, &c, &d) == 0)
            {
                // Update IP address in config
                eth->net->config->ip = MG_IPV4(a, b, c, d);

                printf("IP address will be changed to %u.%u.%u.%u\n", a, b, c, d);
                printf("Restarting network manager...\n");

                // Restart network manager to apply new IP
                net_set_config(eth->net, eth->net->config);
                return;
            }

            printf("Invalid IP address format. Expected: eth --ip 192.168.1.100\n");
            return;
        }
        printf("Invalid argument\n");
    }
}

Net* eth_net_start(const NetConfig* net_config, const EthPhyConfig* phy_config)
{
    rhs_assert(eth == NULL);
    eth      = eth_net_alloc(net_config, phy_config);
    eth->cli = rhs_record_open(RECORD_CLI);

    int32_t net_worker(void* context);
    eth->net->thread = rhs_thread_alloc("eth_net", 4 * 1024, net_worker, eth->net);
    rhs_thread_start(eth->net->thread);

    cli_add_command(eth->cli, "eth", eth_net_cli_command, eth);

    return eth->net;
}

void eth_net_stop(Net* net)
{
    rhs_assert(eth != NULL);
    rhs_assert(eth->net == net);
    rhs_crash("Not implemented yet");
    rhs_record_close(RECORD_CLI);
    rhs_thread_join(eth->net->thread);
    rhs_thread_free(eth->net->thread);
    free(eth->net->config);
    free(eth->net->mgr);
    free(eth->net);
    free(eth);
    eth = NULL;
}
