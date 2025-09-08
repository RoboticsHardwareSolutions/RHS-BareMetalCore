#include "rhs.h"
#include "rhs_hal.h"
#include "usb_cdc_net.h"
#include "mongoose.h"
#include "tusb.h"

// Helper macro for MAC generation
#define GENERATE_LOCALLY_ADMINISTERED_MAC(UUID) \
    {2,                                         \
     UUID[0] ^ UUID[1],                         \
     UUID[2] ^ UUID[3],                         \
     UUID[4] ^ UUID[5],                         \
     UUID[6] ^ UUID[7] ^ UUID[8],               \
     UUID[9] ^ UUID[10] ^ UUID[11]}

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
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_CDC,
                         STRID_INTERFACE,
                         EPNUM_NET_NOTIF,
                         8,
                         EPNUM_NET_OUT,
                         EPNUM_NET_IN,
                         CFG_TUD_NET_ENDPOINT_SIZE),
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
                           CFG_TUD_NET_ENDPOINT_SIZE,
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
                           CFG_TUD_NET_ENDPOINT_SIZE,
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
static struct mg_tcpip_if* s_ifp;
uint8_t                    tud_network_mac_address[6] = {2, 2, 0x84, 0x6A, 0x96, 0};

bool tud_network_recv_cb(const uint8_t* buf, uint16_t len)
{
    mg_tcpip_qwrite((void*) buf, len, s_ifp);
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

bool mg_random(void* buf, size_t len)
{  // Use on-board RNG
    rhs_hal_random_fill_buf(buf, len);
    return true;
}

static bool finish = false;

static void fn(struct mg_connection* c, int ev, void* ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message* hm = (struct mg_http_message*) ev_data;
        if (mg_match(hm->uri, mg_str("/"), NULL))
        {
            const char* html =
                "<!DOCTYPE html>"
                "<html lang='en'>"
                "<head>"
                "    <title>RHS PLC</title>"
                "    <meta charset='UTF-8'>"
                "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "    <style>"
                "        body { "
                "            font-family: Arial, sans-serif; "
                "            background: #f5f5f5; "
                "            margin: 0; "
                "            padding: 20px; "
                "        }"
                "        .container { "
                "            max-width: 400px; "
                "            margin: 50px auto; "
                "            background: white; "
                "            padding: 30px; "
                "            border-radius: 8px; "
                "            box-shadow: 0 2px 10px rgba(0,0,0,0.1); "
                "            text-align: center; "
                "        }"
                "        h1 { "
                "            color: #333; "
                "            margin: 0 0 10px 0; "
                "            font-size: 24px; "
                "        }"
                "        .subtitle { "
                "            color: #666; "
                "            margin-bottom: 30px; "
                "            font-size: 14px; "
                "        }"
                "        .info { "
                "            text-align: left; "
                "            margin: 20px 0; "
                "        }"
                "        .info-item { "
                "            margin: 10px 0; "
                "            padding: 8px 0; "
                "            border-bottom: 1px solid #eee; "
                "        }"
                "        .info-label { "
                "            color: #888; "
                "            font-size: 12px; "
                "        }"
                "        .info-value { "
                "            color: #333; "
                "            font-weight: bold; "
                "        }"
                "        .exit-button { "
                "            background: #dc3545; "
                "            color: white; "
                "            border: none; "
                "            padding: 10px 20px; "
                "            border-radius: 4px; "
                "            cursor: pointer; "
                "            margin-top: 20px; "
                "            text-decoration: none; "
                "            display: inline-block; "
                "        }"
                "        .exit-button:hover { "
                "            background: #c82333; "
                "        }"
                "    </style>"
                "</head>"
                "<body>"
                "    <div class='container'>"
                "        <h1>RHS PLC</h1>"
                "        <p class='subtitle'>Industrial Controller Management System</p>"
                "        <div class='info'>"
                "            <div class='info-item'>"
                "                <div class='info-label'>Status</div>"
                "                <div class='info-value'>Online</div>"
                "            </div>"
                "            <div class='info-item'>"
                "                <div class='info-label'>IP Address</div>"
                "                <div class='info-value'>192.168.3.1</div>"
                "            </div>"
                "            <div class='info-item'>"
                "                <div class='info-label'>Interface</div>"
                "                <div class='info-value'>RNDIS/Ethernet</div>"
                "            </div>"
                "            <div class='info-item'>"
                "                <div class='info-label'>Version</div>"
                "                <div class='info-value'>v1.0.0</div>"
                "            </div>"
                "        </div>"
                "        <a href='/exit' class='exit-button'>Exit</a>"
                "    </div>"
                "</body>"
                "</html>";
            mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", html);
        }
        else if (mg_match(hm->uri, mg_str("/exit"), NULL))
        {
            // Handle exit button click
            finish = true;
            const char* response = 
                "<!DOCTYPE html>"
                "<html lang='en'>"
                "<head>"
                "    <title>Exit</title>"
                "    <meta charset='UTF-8'>"
                "    <style>"
                "        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }"
                "        .message { color: #667eea; font-size: 1.5em; margin-bottom: 20px; }"
                "        .info { color: #666; font-size: 14px; text-align: left; max-width: 400px; margin: 0 auto; }"
                "    </style>"
                "</head>"
                "<body>"
                "    <div class='message'>Application is shutting down...</div>"
                "    <div class='info'>"
                "        <h3>What happens next:</h3>"
                "        <ul>"
                "            <li>Device will be reconfigured</li>"
                "            <li>Network interface will be disconnected</li>"
                "            <li>Control will be available via RTT terminal</li>"
                "        </ul>"
                "        <p><strong>Note:</strong> VCP (Virtual COM Port) control is not implemented yet</p>"
                "    </div>"
                "</body>"
                "</html>";
            mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", response);
        }
        else
        {
            mg_http_reply(c, 404, "", "Not found\n");
        }
    }
}

int32_t cdc_net_service(void* context)
{
    struct mg_mgr mgr;        // Initialise
    mg_mgr_init(&mgr);        // Mongoose event manager
    mg_log_set(MG_LL_DEBUG);  // Set log level

    const uint8_t* uid = rhs_hal_version_uid();
    RHSHalUsbInterface* prev_intf = rhs_hal_usb_get_interface();
    rhs_hal_usb_set_interface(&usb_cdc_net_desc);

    MG_INFO(("Init TCP/IP stack ..."));
    struct mg_tcpip_driver driver = {.tx = usb_tx, .poll = usb_poll};
    struct mg_tcpip_if     mif    = {.mac                = GENERATE_LOCALLY_ADMINISTERED_MAC(uid),
                                     .ip                 = mg_htonl(MG_U32(192, 168, 3, 1)),
                                     .mask               = mg_htonl(MG_U32(255, 255, 255, 0)),
                                     .enable_dhcp_server = true,
                                     .driver             = &driver,
                                     .recv_queue.size    = 4096};
    s_ifp                         = &mif;
    mg_tcpip_init(&mgr, &mif);
    mg_http_listen(&mgr, "tcp://0.0.0.0:80", fn, &mgr);

    MG_INFO(("Init USB ..."));
    rhs_hal_usb_reinit();
    tusb_init();

    MG_INFO(("Init done, starting main loop ..."));
    while (!finish)
    {
        mg_mgr_poll(&mgr, 0);
    }

    MG_INFO(("Finish ..."));
    mg_mgr_free(&mgr);
    tusb_deinit(0);
    rhs_hal_usb_set_interface(prev_intf);
    while(1){rhs_delay_ms(1000);} // FIXME to do application
}
