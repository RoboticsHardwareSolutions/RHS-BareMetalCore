#include "rhs.h"
#include "rhs_hal.h"
#include "usb_cdc_net.h"
#include "net_i.h"
#include "tusb.h"
#include "rhs_hal_cdc_net.h"

typedef struct
{
    Net                 net;
    RHSHalUsbInterface* prev_intf;
} CdcNet;

static_assert(offsetof(CdcNet, net) == 0, "CdcNet must be compatible with Net for safe casting");

#define TAG "cdc_net"

static TudNetOps cdc_net_ops = {0};

static bool cdc_net_recv_cb(const uint8_t* buf, uint16_t len, void* context)
{
    rhs_assert(context);
    mg_tcpip_qwrite((void*) buf, len, ((Net*) context)->mgr->ifp);
    // RHS_LOG_I(TAG, "RECV %hu", len);
    // mg_hexdump(buf, len);
    tud_network_recv_renew();
    return true;
}

static void cdc_net_init_cb(void* context)
{
    (void) context;
}

static uint16_t cdc_net_xmit_cb(uint8_t* dst, void* ref, uint16_t arg, void* context)
{
    (void) context;
    // RHS_LOG_I(TAG, "SEND %hu", arg);
    memcpy(dst, ref, arg);
    return arg;
}

static size_t usb_tx(const void* buf, size_t len, struct mg_tcpip_if* ifp)
{
    if (!tud_ready())
        return 0;
    while (!tud_network_can_xmit(len))
        tud_task_ext(0, false);
    tud_network_xmit((void*) buf, len);
    (void) ifp;
    return len;
}

static bool usb_poll(struct mg_tcpip_if* ifp, bool s1)
{
    (void) ifp;
    tud_task_ext(0, false);
    return s1 ? tud_inited() && tud_ready() && tud_connected() : false;
}

static void cdc_net_init_tcpip(Net* net)
{
    struct mg_tcpip_if*     ifp    = malloc(sizeof(struct mg_tcpip_if));
    struct mg_tcpip_driver* driver = malloc(sizeof(struct mg_tcpip_driver));
    rhs_assert(net && ifp && driver);
    uint8_t* mac = net->config->mac;

    // Clear interface and driver data structures
    memset(ifp, 0, sizeof(struct mg_tcpip_if));
    memset(driver, 0, sizeof(struct mg_tcpip_driver));

    // Apply configuration
    driver->tx              = usb_tx;
    driver->poll            = usb_poll;
    ifp->enable_dhcp_server = true;
    ifp->driver             = driver;
    ifp->recv_queue.size    = 4096;

    // If config fields are empty, fill in with default values from compile-time macros
    if (net->config->ip[0] == '\0')
    {
        strcpy(net->config->ip, CDC_NET_IP_STRING);
    }
    if (net->config->mask[0] == '\0')
    {
        strcpy(net->config->mask, CDC_NET_MASK_STRING);
    }
    if (net->config->gateway[0] == '\0')
    {
        strcpy(net->config->gateway, CDC_NET_GW_STRING);
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
    memcpy(ifp->mac, mac, sizeof(ifp->mac));

    // Initialize TCP/IP interface
    mg_tcpip_init(net->mgr, ifp);
    RHS_LOG_I(TAG, "CDC NET, MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void usb_cdc_net_free(CdcNet* app)
{
    rhs_thread_free(app->net.thread);
    rhs_message_queue_free(app->net.queue);
    mg_mgr_free(app->net.mgr);
    free(app->net.mgr->ifp->driver);
    free(app->net.mgr->ifp);
    free(app->net.config);
    free(app->net.mgr);
    free(app);
}

static CdcNet* usb_cdc_net_alloc(const NetConfig* config)
{
    CdcNet* app = malloc(sizeof(CdcNet));
    rhs_assert(app != NULL);

    memset(app, 0, sizeof(*app));
    app->net.queue = rhs_message_queue_alloc(3, sizeof(NetApiEventMessage));

    app->net.mgr    = malloc(sizeof(struct mg_mgr));
    app->net.config = malloc(sizeof(NetConfig));
    rhs_assert(app->net.mgr != NULL && app->net.config != NULL);

    if (config == NULL)
    {
        strcpy(app->net.config->ip, CDC_NET_IP_STRING);
        strcpy(app->net.config->mask, CDC_NET_MASK_STRING);
        strcpy(app->net.config->gateway, CDC_NET_GW_STRING);
        uint8_t mac[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
        memcpy(app->net.config->mac, mac, sizeof(mac));
    }
    else
    {
        memcpy(app->net.config, config, sizeof(NetConfig));
    }

    mg_mgr_init(app->net.mgr);  // Mongoose event manager

    // It is necessary that the mac for usb_cdc_net and usb_eth_bridge be different,
    // otherwise the system may incorrectly name the interface.
    tud_network_mac_address[5] = 0;

    app->prev_intf = rhs_hal_usb_get_interface();
    rhs_hal_usb_set_interface(&usb_cdc_net_desc);

    cdc_net_init_tcpip(&app->net);

    rhs_hal_usb_reinit();
    tusb_init();

    cdc_net_ops.recv    = cdc_net_recv_cb;
    cdc_net_ops.init    = cdc_net_init_cb;
    cdc_net_ops.xmit    = cdc_net_xmit_cb;
    cdc_net_ops.context = &app->net;

    rhs_hal_cdc_net_set(&cdc_net_ops);

    return app;
}

Net* usb_cdc_net_start(const NetConfig* config)
{
    CdcNet* app = usb_cdc_net_alloc(config);

    int32_t net_worker(void* context);
    app->net.thread = rhs_thread_alloc("cdc_net", 4 * 1024, net_worker, &app->net);
    rhs_thread_start(app->net.thread);

    return &app->net;
}

void usb_cdc_net_stop(Net* net)
{
    rhs_assert(net != NULL);
    CdcNet* app = (CdcNet*) net;
    rhs_hal_cdc_net_clear();
    net_stop(net);
    rhs_thread_join(app->net.thread);
    usb_cdc_net_free(app);
    tusb_deinit(0);
    rhs_hal_usb_set_interface(app->prev_intf);
}
