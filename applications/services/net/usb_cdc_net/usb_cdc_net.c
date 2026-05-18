#include "rhs.h"
#include "rhs_hal.h"
#include "usb_cdc_net.h"
#include "net_i.h"
#include "tusb.h"
#include "tud_net_dispatch.h"

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
    ITF_NUM_TOTAL
};

enum
{
#if CFG_TUD_ECM_RNDIS
    CONFIG_ID_ECM   = 0,  // ECM first: default for Android/macOS/Linux
    CONFIG_ID_RNDIS = 1,  // RNDIS second: Windows
#else
    CONFIG_ID_NCM = 0,
#endif
    CONFIG_ID_COUNT
};

#if CFG_TUD_NCM
#    define USB_BCD 0x0201
#else
#    define USB_BCD 0x0200
#endif

typedef struct
{
    Net                 net;
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
    .bcdUSB          = USB_BCD,

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

// full speed configuration
static uint8_t const rndis_fs_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_RNDIS + 1, ITF_NUM_TOTAL, 0, MAIN_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE, EPNUM_NET_NOTIF, 8, EPNUM_NET_OUT, EPNUM_NET_IN, 64),
};

static const uint8_t ecm_fs_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_ECM + 1, ITF_NUM_TOTAL, 0, ALT_CONFIG_TOTAL_LEN, 0, 100),

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

#    if TUD_OPT_HIGH_SPEED
// Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration

// high speed configuration
static uint8_t const rndis_hs_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_RNDIS + 1, ITF_NUM_TOTAL, 0, MAIN_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE, EPNUM_NET_NOTIF, 8, EPNUM_NET_OUT, EPNUM_NET_IN, 512),
};

static const uint8_t ecm_hs_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_ECM + 1, ITF_NUM_TOTAL, 0, ALT_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, description string index, MAC address string index, EP notification address and size, EP data
    // address (out, in), and size, max segment size.
    TUD_CDC_ECM_DESCRIPTOR(ITF_NUM_CDC,
                           STRID_INTERFACE,
                           STRID_MAC,
                           EPNUM_NET_NOTIF,
                           64,
                           EPNUM_NET_OUT,
                           EPNUM_NET_IN,
                           512,
                           CFG_TUD_NET_MTU),
};
#    endif  // highspeed

#else

// full speed configuration
static uint8_t const ncm_fs_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_NCM + 1, ITF_NUM_TOTAL, 0, NCM_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, description string index, MAC address string index, EP notification address and size, EP data
    // address (out, in), and size, max segment size, EP notification bInterval.
    TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_CDC,
                           STRID_INTERFACE,
                           STRID_MAC,
                           EPNUM_NET_NOTIF,
                           64,
                           EPNUM_NET_OUT,
                           EPNUM_NET_IN,
                           64,
                           CFG_TUD_NET_MTU,
                           50,
                           (uint8_t) ((uint8_t) NCM_NETWORK_CAPS_ETH_FILTER |
                                      (uint8_t) NCM_NETWORK_CAPS_NTB_INPUT_SIZE)),
};

#    if TUD_OPT_HIGH_SPEED
// Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration

// high speed configuration
// bInterval: FS=50 means 50ms; HS encodes as 2^(n-1) * 125us, so 9 = 2^8 * 125us = 32ms
static uint8_t const ncm_hs_configuration[] = {
    // Config number (index+1), interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_NCM + 1, ITF_NUM_TOTAL, 0, NCM_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, description string index, MAC address string index, EP notification address and size, EP data
    // address (out, in), and size, max segment size, EP notification bInterval.
    TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_CDC,
                           STRID_INTERFACE,
                           STRID_MAC,
                           EPNUM_NET_NOTIF,
                           64,
                           EPNUM_NET_OUT,
                           EPNUM_NET_IN,
                           512,
                           CFG_TUD_NET_MTU,
                           9,
                           (uint8_t) ((uint8_t) NCM_NETWORK_CAPS_ETH_FILTER |
                                      (uint8_t) NCM_NETWORK_CAPS_NTB_INPUT_SIZE)),
};
#    endif  // highspeed

#endif

// NCM work with all latest OS i.e macos 10.10+, windows 10+, and Linux.
// For older system Configuration array of RNDIS and CDC-ECM may be needed for better compatibility.
// - Windows only works with RNDIS
// - MacOS only works with CDC-ECM
// - Linux will work on both
#if CFG_TUD_ECM_RNDIS

static const uint8_t* const configuration_fs_arr[CONFIG_ID_COUNT] = {[CONFIG_ID_RNDIS] = rndis_fs_configuration,
                                                                     [CONFIG_ID_ECM]   = ecm_fs_configuration};

#    if TUD_OPT_HIGH_SPEED
static const uint8_t* const configuration_hs_arr[CONFIG_ID_COUNT] = {[CONFIG_ID_RNDIS] = rndis_hs_configuration,
                                                                     [CONFIG_ID_ECM]   = ecm_hs_configuration};

// Size array for each configuration
static const uint16_t configuration_sz_arr[CONFIG_ID_COUNT] = {[CONFIG_ID_RNDIS] = MAIN_CONFIG_TOTAL_LEN,
                                                               [CONFIG_ID_ECM]   = ALT_CONFIG_TOTAL_LEN};

// Scratch buffer for other speed configuration (sized to hold the largest config)
#        define MAX_CONFIG_TOTAL_LEN TU_MAX(MAIN_CONFIG_TOTAL_LEN, ALT_CONFIG_TOTAL_LEN)
#    endif

#else

static const uint8_t* const configuration_fs_arr[CONFIG_ID_COUNT] = {[CONFIG_ID_NCM] = ncm_fs_configuration};

#    if TUD_OPT_HIGH_SPEED
static const uint8_t* const configuration_hs_arr[CONFIG_ID_COUNT] = {[CONFIG_ID_NCM] = ncm_hs_configuration};

// Size array for each configuration
static const uint16_t configuration_sz_arr[CONFIG_ID_COUNT] = {[CONFIG_ID_NCM] = NCM_CONFIG_TOTAL_LEN};

// Scratch buffer for other speed configuration (sized to hold the largest config)
#        define MAX_CONFIG_TOTAL_LEN NCM_CONFIG_TOTAL_LEN
#    endif
#endif

#if TUD_OPT_HIGH_SPEED
static uint8_t desc_other_speed_config[MAX_CONFIG_TOTAL_LEN];

// device qualifier: device descriptor fields that differ at other speed
static tusb_desc_device_qualifier_t const desc_device_qualifier = {.bLength = sizeof(tusb_desc_device_qualifier_t),
                                                                   .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
                                                                   .bcdUSB          = USB_BCD,

                                                                   .bDeviceClass    = TUSB_CLASS_MISC,
                                                                   .bDeviceSubClass = MISC_SUBCLASS_COMMON,
                                                                   .bDeviceProtocol = MISC_PROTOCOL_IAD,

                                                                   .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
                                                                   .bNumConfigurations = CONFIG_ID_COUNT,
                                                                   .bReserved          = 0x00};

// Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
// device_qualifier descriptor describes information about a high-speed capable device that would
// change if the device were operating at the other speed. If not highspeed capable stall this request.
uint8_t const* tud_descriptor_device_qualifier_cb(void)
{
    return (uint8_t const*) &desc_device_qualifier;
}

// Invoked when received GET OTHER SPEED CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
// Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
    if (index >= CONFIG_ID_COUNT)
        return NULL;

    // if link speed is high return fullspeed config, and vice versa
    const uint8_t* const* arr = (tud_speed_get() == TUSB_SPEED_HIGH) ? configuration_fs_arr : configuration_hs_arr;

    // Note: the descriptor type is OTHER_SPEED_CONFIG instead of CONFIG
    memcpy(desc_other_speed_config, arr[index], configuration_sz_arr[index]);
    desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;

    return desc_other_speed_config;
}

#endif  // highspeed

#if CFG_TUD_NCM
//--------------------------------------------------------------------+
// BOS Descriptor
//--------------------------------------------------------------------+

/* Used to automatically load the NCM driver on Windows 10, otherwise manual driver install is needed.
   Associate NCM interface with WINNCM driver. */

/* Microsoft OS 2.0 registry property descriptor
Per MS requirements https://msdn.microsoft.com/en-us/library/windows/hardware/hh450799(v=vs.85).aspx
device should create DeviceInterfaceGUIDs. It can be done by driver and
in case of real PnP solution device should expose MS "Microsoft OS 2.0
registry property descriptor". Such descriptor can insert any record
into Windows registry per device/configuration/interface. In our case it
will insert "DeviceInterfaceGUIDs" multistring property.

GUID is freshly generated and should be OK to use.

https://developers.google.com/web/fundamentals/native-hardware/build-for-webusb/
(Section Microsoft OS compatibility descriptors)
*/

#    define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

#    define MS_OS_20_DESC_LEN 0xB2

// BOS Descriptor is required for webUSB
const uint8_t desc_bos[] = {
    // total length, number of device caps
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),

    // Microsoft OS 2.0 descriptor
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, 1)};

const uint8_t* tud_descriptor_bos_cb(void)
{
    return desc_bos;
}
// clang-format off
const uint8_t desc_ms_os_20[] = {
  // Set header: length, type, windows version, total length
  U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000),
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

  // Configuration subset header: length, type, configuration index, reserved, configuration total length
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0,
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),

  // Function Subset header: length, type, first interface, reserved, subset length
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_CDC, 0,
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),

  // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'N', 'C', 'M', 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

  // MS OS 2.0 Registry property descriptor: length, type
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
  U16_TO_U8S_LE(0x0007),
  U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r',
  0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
  U16_TO_U8S_LE(0x0050), // wPropertyDataLength
                         //bPropertyData: {12345678-0D08-43FD-8B3E-127CA8AFFF9D}
  '{', 0x00, '1', 0x00, '2', 0x00, '3', 0x00, '4', 0x00, '5', 0x00, '6', 0x00, '7', 0x00, '8', 0x00, '-', 0x00, '0',
  0x00, 'D', 0x00, '0', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00, 'D', 0x00, '-', 0x00, '8', 0x00,
  'B', 0x00, '3', 0x00, 'E', 0x00, '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00, 'C', 0x00, 'A', 0x00, '8', 0x00, 'A',
  0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00, '9', 0x00, 'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00};
// clang-format on
TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "Incorrect size");

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t* request)
{
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP)
    {
        return true;
    }

    switch (request->bmRequestType_bit.type)
    {
    case TUSB_REQ_TYPE_VENDOR:
        switch (request->bRequest)
        {
        case 1:
            if (request->wIndex == 7)
            {
                // Get Microsoft OS 2.0 compatible descriptor
                uint16_t total_len;
                memcpy(&total_len, desc_ms_os_20 + 8, 2);

                return tud_control_xfer(rhport, request, (void*) (uintptr_t) desc_ms_os_20, total_len);
            }
            else
            {
                return false;
            }

        default:
            break;  // nothing to do
        }
        break;

    default:
        break;  // nothing to do
    }

    // stall unknown request
    return false;
}

#endif

// array of pointer to string descriptors
static char const* string_desc_cdc_net_arr[] = {
    [STRID_LANGID]       = (const char[]) {0x09, 0x04},  // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "TinyUSB",                    // Manufacturer
    [STRID_PRODUCT]      = "TinyUSB Device",             // Product
    [STRID_SERIAL]       = NULL,                         // Serials will use unique ID if possible
    [STRID_INTERFACE]    = "TinyUSB Network Interface",  // Interface Description
    [STRID_MAC]          = NULL                          // STRID_MAC index is handled separately
};

static RHSHalUsbInterface usb_cdc_net_desc = {
    .device_desc            = &desc_cdc_net,
    .configuration_arr      = configuration_fs_arr,  // TODO - support high speed configurations
    .string_desc_arr        = (char const* const*) string_desc_cdc_net_arr,
    .string_desc_arr_count  = TU_ARRAY_SIZE(string_desc_cdc_net_arr),
};

#define TAG "cdc_net"

static bool cdc_net_recv_cb(const uint8_t* buf, uint16_t len)
{
    rhs_assert(usb_cdc_active_net != NULL);
    mg_tcpip_qwrite((void*) buf, len, usb_cdc_active_net->mgr->ifp);
    // MG_INFO(("RECV %hu", len));
    // mg_hexdump(buf, len);
    tud_network_recv_renew();
    return true;
}

static void cdc_net_init_cb(void) {}

static uint16_t cdc_net_xmit_cb(uint8_t* dst, void* ref, uint16_t arg)
{
    // MG_INFO(("SEND %hu", arg));
    memcpy(dst, ref, arg);
    return arg;
}

static const TudNetOps cdc_net_ops = {
    .recv = cdc_net_recv_cb,
    .init = cdc_net_init_cb,
    .xmit = cdc_net_xmit_cb,
};

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
    if (net->config->mac[0] == 0 && net->config->mac[1] == 0 && net->config->mac[2] == 0 && net->config->mac[3] == 0 &&
        net->config->mac[4] == 0 && net->config->mac[5] == 0)
    {
        uint8_t mac[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
        memcpy(net->config->mac, mac, sizeof(mac));
    }
    // Initialize config with default values from compile-time macros
    unsigned int a, b, c, d;
    rhs_assert(string_to_ip(net->config->ip, &a, &b, &c, &d) == 0);
    ifp->ip = MG_IPV4(a, b, c, d);
    rhs_assert(string_to_ip(net->config->mask, &a, &b, &c, &d) == 0);
    ifp->mask = MG_IPV4(a, b, c, d);
    rhs_assert(string_to_ip(net->config->gateway, &a, &b, &c, &d) == 0);
    ifp->gw = MG_IPV4(a, b, c, d);
    rhs_assert(net->config->mac[0] != 0 || net->config->mac[1] != 0 || net->config->mac[2] != 0 ||
               net->config->mac[3] != 0 || net->config->mac[4] != 0 || net->config->mac[5] != 0);
    memcpy(ifp->mac, net->config->mac, sizeof(net->config->mac));

    ifp->enable_dhcp_server = true;
    ifp->driver             = driver;
    ifp->recv_queue.size    = 4096;

    // Initialize TCP/IP interface
    mg_tcpip_init(net->mgr, ifp);
    MG_INFO(("Driver: CDC TCPIP, MAC: %M", mg_print_mac, ifp->mac));
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

    app->prev_intf = rhs_hal_usb_get_interface();

    rhs_hal_usb_set_interface(&usb_cdc_net_desc);

    cdc_net_init_tcpip(&app->net);

    rhs_hal_usb_reinit();
    tusb_init();

    usb_cdc_active_net = &app->net;
    tud_net_dispatch_set(&cdc_net_ops);

    return app;
}

Net* usb_cdc_net_start(const NetConfig* config)
{
    if (usb_cdc_active_net != NULL)
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
    tud_net_dispatch_clear();
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
