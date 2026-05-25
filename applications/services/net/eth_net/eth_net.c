#include "eth_net.h"
#include "rhs.h"
#include "rhs_hal.h"
#include "net_i.h"

#define TAG "eth_net"

typedef struct
{
    Net net;
} EthNet;

static_assert(offsetof(EthNet, net) == 0, "EthNet must be compatible with Net for safe casting");

static void eth_net_init_tcpip(Net* net, const EthPhyConfig* phy_config)
{
    struct mg_tcpip_if*                 ifp    = malloc(sizeof(struct mg_tcpip_if));
    struct mg_tcpip_driver_stm32f_data* driver = malloc(sizeof(struct mg_tcpip_driver_stm32f_data));
    rhs_assert(net && ifp && driver);
    uint8_t* mac = net->config->mac;

    // Clear interface and driver data structures
    memset(ifp, 0, sizeof(struct mg_tcpip_if));
    memset(driver, 0, sizeof(struct mg_tcpip_driver_stm32f_data));

    // Apply Ethernet-specific PHY configuration (use provided values or defaults)
    driver->mdc_cr   = phy_config ? phy_config->mdc_cr : MG_DRIVER_MDC_CR;
    driver->phy_addr = phy_config ? phy_config->phy_addr : MG_TCPIP_PHY_ADDR;
    ifp->driver      = &mg_tcpip_driver_stm32f;
    ifp->driver_data = driver;

    // If config fields are empty, fill in with default values from compile-time macros
    if (net->config->ip[0] == '\0')
    {
        strcpy(net->config->ip, ETH_NET_IP_STRING);
    }
    if (net->config->mask[0] == '\0')
    {
        strcpy(net->config->mask, ETH_NET_MASK_STRING);
    }
    if (net->config->gateway[0] == '\0')
    {
        strcpy(net->config->gateway, ETH_NET_GW_STRING);
    }
    if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0)
    {
        uint8_t tmp[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
        memcpy(mac, tmp, sizeof(tmp));
    }

    // Initialize config
    unsigned int a, b, c, d;
    rhs_assert(string_to_ip(net->config->ip, &a, &b, &c, &d) == 0);
    ifp->ip = MG_IPV4(a, b, c, d);
    rhs_assert(string_to_ip(net->config->mask, &a, &b, &c, &d) == 0);
    ifp->mask = MG_IPV4(a, b, c, d);
    rhs_assert(string_to_ip(net->config->gateway, &a, &b, &c, &d) == 0);
    ifp->gw = MG_IPV4(a, b, c, d);
    memcpy(ifp->mac, mac, sizeof(mac));

    // Initialize TCP/IP interface
    mg_tcpip_init(net->mgr, ifp);
    RHS_LOG_I(TAG, "Ethernet, MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static EthNet* eth_net_alloc(const NetConfig* config, const EthPhyConfig* phy_config)
{
    EthNet* app = malloc(sizeof(EthNet));
    rhs_assert(app != NULL);

    memset(app, 0, sizeof(*app));
    app->net.queue  = rhs_message_queue_alloc(3, sizeof(NetApiEventMessage));
    app->net.mgr    = malloc(sizeof(struct mg_mgr));
    app->net.config = malloc(sizeof(NetConfig));
    rhs_assert(app->net.mgr != NULL && app->net.config != NULL);

    if (config == NULL)
    {
        strcpy(app->net.config->ip, ETH_NET_IP_STRING);
        strcpy(app->net.config->mask, ETH_NET_MASK_STRING);
        strcpy(app->net.config->gateway, ETH_NET_GW_STRING);
        uint8_t mac[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
        memcpy(app->net.config->mac, mac, sizeof(mac));
    }
    else
    {
        memcpy(app->net.config, config, sizeof(NetConfig));
    }

    // Initialise Mongoose network stack
    rhs_hal_eth_init();

    mg_mgr_init(app->net.mgr);  // and attach it to the interface

    eth_net_init_tcpip(&app->net, phy_config);

    return app;
}

static void eth_net_free(EthNet* app)
{
    rhs_thread_free(app->net.thread);
    rhs_message_queue_free(app->net.queue);
    mg_mgr_free(app->net.mgr);
    free(app->net.mgr->ifp->driver_data);
    free(app->net.mgr->ifp);
    free(app->net.config);
    free(app->net.mgr);
    free(app);
}

Net* eth_net_start(const NetConfig* net_config, const EthPhyConfig* phy_config)
{
    EthNet* app = eth_net_alloc(net_config, phy_config);

    int32_t                             net_worker(void* context);
    char                                t_name[16];
    struct mg_tcpip_driver_stm32f_data* driver = (struct mg_tcpip_driver_stm32f_data*) app->net.mgr->ifp->driver_data;
    snprintf(t_name, sizeof(t_name), "eth_net_%d", driver->phy_addr);
    app->net.thread = rhs_thread_alloc(t_name, 4 * 1024, net_worker, &app->net);
    rhs_thread_start(app->net.thread);

    return &app->net;
}

void eth_net_stop(Net* net)
{
    rhs_assert(net != NULL);
    EthNet* app = (EthNet*) net;
    net_stop(net);
    rhs_thread_join(app->net.thread);
    eth_net_free(app);
    rhs_hal_eth_deinit();
}
