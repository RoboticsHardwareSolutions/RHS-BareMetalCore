#include "rhs.h"
#include "rhs_hal.h"
#include "rndis.h"
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

#define TAG "rndis"
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
                "<html lang='ru'>"
                "<head>"
                "    <title>Добро пожаловать в RHS PLC</title>"
                "    <meta charset='UTF-8'>"
                "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "    <style>"
                "        * { margin: 0; padding: 0; box-sizing: border-box; }"
                "        body { "
                "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; "
                "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
                "            min-height: 100vh; "
                "            display: flex; "
                "            justify-content: center; "
                "            align-items: center; "
                "        }"
                "        .welcome-container { "
                "            background: rgba(255, 255, 255, 0.95); "
                "            padding: 50px; "
                "            border-radius: 20px; "
                "            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1); "
                "            text-align: center; "
                "            max-width: 500px; "
                "            width: 90%; "
                "            backdrop-filter: blur(10px); "
                "        }"
                "        .logo { "
                "            width: 80px; "
                "            height: 80px; "
                "            background: linear-gradient(45deg, #667eea, #764ba2); "
                "            border-radius: 50%; "
                "            margin: 0 auto 30px; "
                "            display: flex; "
                "            align-items: center; "
                "            justify-content: center; "
                "            color: white; "
                "            font-size: 30px; "
                "            font-weight: bold; "
                "        }"
                "        h1 { "
                "            color: #333; "
                "            font-size: 2.5em; "
                "            margin-bottom: 20px; "
                "            font-weight: 300; "
                "        }"
                "        .subtitle { "
                "            color: #666; "
                "            font-size: 1.2em; "
                "            margin-bottom: 30px; "
                "            line-height: 1.6; "
                "        }"
                "        .info-grid { "
                "            display: grid; "
                "            grid-template-columns: 1fr 1fr; "
                "            gap: 20px; "
                "            margin: 30px 0; "
                "        }"
                "        .info-item { "
                "            background: #f8f9ff; "
                "            padding: 20px; "
                "            border-radius: 10px; "
                "            border-left: 4px solid #667eea; "
                "        }"
                "        .info-label { "
                "            color: #667eea; "
                "            font-weight: bold; "
                "            font-size: 0.9em; "
                "            text-transform: uppercase; "
                "            letter-spacing: 1px; "
                "        }"
                "        .info-value { "
                "            color: #333; "
                "            font-size: 1.1em; "
                "            margin-top: 5px; "
                "        }"
                "        .footer { "
                "            margin-top: 30px; "
                "            color: #999; "
                "            font-size: 0.9em; "
                "        }"
                "        @keyframes fadeIn { "
                "            from { opacity: 0; transform: translateY(20px); } "
                "            to { opacity: 1; transform: translateY(0); } "
                "        }"
                "        .welcome-container { "
                "            animation: fadeIn 0.8s ease-out; "
                "        }"
                "        @media (max-width: 600px) { "
                "            .info-grid { grid-template-columns: 1fr; } "
                "            h1 { font-size: 2em; } "
                "            .welcome-container { padding: 30px; } "
                "        }"
                "    </style>"
                "</head>"
                "<body>"
                "    <div class='welcome-container'>"
                "        <div class='logo'>RHS</div>"
                "        <h1>Добро пожаловать!</h1>"
                "        <p class='subtitle'>Система управления промышленными контроллерами<br>Robotics Hardware Solutions</p>"
                "        <div class='info-grid'>"
                "            <div class='info-item'>"
                "                <div class='info-label'>Статус</div>"
                "                <div class='info-value'>Онлайн</div>"
                "            </div>"
                "            <div class='info-item'>"
                "                <div class='info-label'>IP адрес</div>"
                "                <div class='info-value'>192.168.3.1</div>"
                "            </div>"
                "            <div class='info-item'>"
                "                <div class='info-label'>Интерфейс</div>"
                "                <div class='info-value'>RNDIS/Ethernet</div>"
                "            </div>"
                "            <div class='info-item'>"
                "                <div class='info-label'>Версия</div>"
                "                <div class='info-value'>v1.0.0</div>"
                "            </div>"
                "        </div>"
                "        <div class='footer'>"
                "            <p>© 2025 Robotics Hardware Solutions. Все права защищены.</p>"
                "        </div>"
                "    </div>"
                "</body>"
                "</html>";
            mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", html);
        }
        else
        {
            mg_http_reply(c, 404, "", "Not found\n");
        }
    }
}

int32_t rndis_service(void* context)
{
    struct mg_mgr mgr;        // Initialise
    mg_mgr_init(&mgr);        // Mongoose event manager
    mg_log_set(MG_LL_DEBUG);  // Set log level

    const uint8_t* uid = rhs_hal_version_uid();

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
}
