#include "rhs.h"
#include "rhs_hal.h"
#include "usb_cdc_net.h"
#include "mongoose.h"
#include "net_i.h"
#include "tusb.h"
#include "cli.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       NET | VENDOR | MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID                                                                                                  \
    (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) | \
     _PID_MAP(ECM_RNDIS, 5) | _PID_MAP(NCM, 5))

// Network configuration enums
enum
{
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL_NET
};

enum
{
#if CFG_TUD_ECM_RNDIS
    CONFIG_ID_RNDIS = 0,
    CONFIG_ID_ECM   = 1,
#else
    CONFIG_ID_NCM = 0,
#endif
    CONFIG_ID_COUNT
};

typedef struct
{
    Net                 net;
    Cli*                cli;
    RHSHalUsbInterface* prev_intf;
} CdcNet;

static_assert(offsetof(CdcNet, net) == 0, "CdcNet must be compatible with Net for safe casting");

static Net* usb_cdc_active_net;

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
static tusb_desc_device_t const desc_cdc_net = {
    .bLength         = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB          = 0x0200,

    // Use Interface Association Descriptor (IAD) device class
    .bDeviceClass    = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor  = 0xCafe,
    .idProduct = USB_PID,
    .bcdDevice = 0x0101,

    .iManufacturer = STRID_MANUFACTURER,
    .iProduct      = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,

    .bNumConfigurations = CONFIG_ID_COUNT  // multiple configurations
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define MAIN_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_RNDIS_DESC_LEN)
#define ALT_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_ECM_DESC_LEN)
#define NCM_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_NCM_DESC_LEN)

#define EPNUM_NET_NOTIF 0x81
#define EPNUM_NET_OUT 0x02
#define EPNUM_NET_IN 0x82

#if CFG_TUD_ECM_RNDIS

static uint8_t const rndis_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_RNDIS + 1, ITF_NUM_TOTAL_NET, 0, MAIN_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE, EPNUM_NET_NOTIF, 8, EPNUM_NET_OUT, EPNUM_NET_IN, 64),
};

static uint8_t const ecm_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_ECM + 1, ITF_NUM_TOTAL_NET, 0, ALT_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, description string index, MAC address string index, EP notification address and size, EP data
    // address (out, in), and size, max segment size.
    TUD_CDC_ECM_DESCRIPTOR(ITF_NUM_CDC,
                           STRID_INTERFACE,
                           STRID_MAC,
                           EPNUM_NET_NOTIF,
                           64,
                           EPNUM_NET_OUT,
                           EPNUM_NET_IN,
                           64,
                           CFG_TUD_NET_MTU),
};

#else

static uint8_t const ncm_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_NCM + 1, ITF_NUM_TOTAL_NET, 0, NCM_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, description string index, MAC address string index, EP notification address and size, EP data
    // address (out, in), and size, max segment size.
    TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_CDC,
                           STRID_INTERFACE,
                           STRID_MAC,
                           EPNUM_NET_NOTIF,
                           64,
                           EPNUM_NET_OUT,
                           EPNUM_NET_IN,
                           64,
                           CFG_TUD_NET_MTU),
};

#endif

// Configuration array: RNDIS and CDC-ECM
// - Windows only works with RNDIS
// - MacOS only works with CDC-ECM
// - Linux will work on both
static uint8_t const* const configuration_arr[2] = {
#if CFG_TUD_ECM_RNDIS
    [CONFIG_ID_RNDIS] = rndis_configuration,
    [CONFIG_ID_ECM]   = ecm_configuration
#else
    [CONFIG_ID_NCM] = ncm_configuration
#endif
};

// array of pointer to string descriptors
static char const* string_desc_cdc_net_arr[] = {
    [STRID_LANGID]       = (const char[]) {0x09, 0x04},  // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "TinyUSB",                    // Manufacturer
    [STRID_PRODUCT]      = "TinyUSB Device",             // Product
    [STRID_SERIAL]       = "123456",                     // Serial
    [STRID_INTERFACE]    = "TinyUSB Network Interface"   // Interface Description

    // STRID_MAC index is handled separately
};

static RHSHalUsbInterface usb_cdc_net_desc = {
    .device_desc       = &desc_cdc_net,
    .configuration_arr = configuration_arr,
    .string_desc_arr   = (char const* const*) string_desc_cdc_net_arr,
};

#define TAG "cdc_net"
uint8_t tud_network_mac_address[6] = {2, 2, 0x84, 0x6A, 0x96, 0};

bool tud_network_recv_cb(const uint8_t* buf, uint16_t len)
{
    rhs_assert(usb_cdc_active_net != NULL);
    mg_tcpip_qwrite((void*) buf, len, usb_cdc_active_net->mgr->ifp);
    // MG_INFO(("RECV %hu", len));
    // mg_hexdump(buf, len);
    tud_network_recv_renew();
    return true;
}

void tud_network_init_cb(void) {}

uint16_t tud_network_xmit_cb(uint8_t* dst, void* ref, uint16_t arg)
{
    // MG_INFO(("SEND %hu", arg));
    memcpy(dst, ref, arg);
    return arg;
}

static size_t usb_tx(const void* buf, size_t len, struct mg_tcpip_if* ifp)
{
    if (!tud_ready())
        return 0;
    while (!tud_network_can_xmit(len))
        tud_task();
    tud_network_xmit((void*) buf, len);
    (void) ifp;
    return len;
}

static bool usb_poll(struct mg_tcpip_if* ifp, bool s1)
{
    (void) ifp;
    tud_task();
    return s1 ? tud_inited() && tud_ready() && tud_connected() : false;
}

static void cdc_net_init_tcpip(Net* net)
{
    struct mg_tcpip_if*     ifp    = malloc(sizeof(struct mg_tcpip_if));
    struct mg_tcpip_driver* driver = malloc(sizeof(struct mg_tcpip_driver));
    rhs_assert(net && ifp && driver);

    // Clear interface and driver data structures
    memset(ifp, 0, sizeof(struct mg_tcpip_if));
    memset(driver, 0, sizeof(struct mg_tcpip_driver));

    // Apply configuration
    driver->tx   = usb_tx;
    driver->poll = usb_poll;

    ifp->ip                 = net->config->ip;
    ifp->mask               = net->config->mask;
    ifp->gw                 = net->config->gateway;
    ifp->enable_dhcp_server = true;
    ifp->driver             = driver;
    ifp->recv_queue.size    = 4096;

    // Copy MAC address
    for (int i = 0; i < 6; i++)
    {
        ifp->mac[i] = net->config->mac[i];
    }

    // Initialize TCP/IP interface
    mg_tcpip_init(net->mgr, ifp);
    MG_INFO(("Driver: CDC TCPIP, MAC: %M", mg_print_mac, ifp->mac));
}

static CdcNet* usb_cdc_net_alloc(const NetConfig* config)
{
    CdcNet* app = malloc(sizeof(CdcNet));
    rhs_assert(app != NULL);

    memset(app, 0, sizeof(*app));
    app->net.mgr    = malloc(sizeof(struct mg_mgr));
    app->net.config = malloc(sizeof(NetConfig));
    rhs_assert(app->net.mgr != NULL && app->net.config != NULL);

    // Use provided config if valid, otherwise use defaults from compile-time macros
    if (config != NULL && config->ip != 0 && config->mask != 0)
    {
        memcpy(app->net.config, config, sizeof(NetConfig));
    }
    else
    {
        // Initialize config with default values from compile-time macros
        unsigned int a, b, c, d;
        uint8_t      mac[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
        if (string_to_ip(CDC_NET_IP_STRING, &a, &b, &c, &d) == 0)
        {
            app->net.config->ip = MG_IPV4(a, b, c, d);
        }
        if (string_to_ip(CDC_NET_MASK_STRING, &a, &b, &c, &d) == 0)
        {
            app->net.config->mask = MG_IPV4(a, b, c, d);
        }
        if (string_to_ip(CDC_NET_GW_STRING, &a, &b, &c, &d) == 0)
        {
            app->net.config->gateway = MG_IPV4(a, b, c, d);
        }

        memcpy(app->net.config->mac, mac, sizeof(mac));
    }

    mg_mgr_init(app->net.mgr);  // Mongoose event manager

    app->prev_intf = rhs_hal_usb_get_interface();

    rhs_hal_usb_set_interface(&usb_cdc_net_desc);

    cdc_net_init_tcpip(&app->net);

    rhs_hal_usb_reinit();
    tusb_init();

    usb_cdc_active_net = &app->net;

    return app;
}

Net* usb_cdc_net_start(const NetConfig* config)
{
    if(usb_cdc_active_net != NULL)
    {
        MG_ERROR(("USB CDC network interface is already active"));
        return usb_cdc_active_net;
    }
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
    rhs_crash("Not implemented yet");
    tusb_deinit(0);
    rhs_thread_join(app->net.thread);
    rhs_thread_free(app->net.thread);
    free(app->net.config);
    free(app->net.mgr);
    if (usb_cdc_active_net == &app->net)
    {
        usb_cdc_active_net = NULL;
    }
    free(app);
}
