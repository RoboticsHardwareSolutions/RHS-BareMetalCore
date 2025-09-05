#include "tusb.h"

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
tusb_desc_device_t const desc_cdc_net = {
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

uint8_t const rndis_configuration[] = {
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

uint8_t const ecm_configuration[] = {
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

uint8_t const ncm_configuration[] = {
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
uint8_t const* const configuration_arr[2] = {
#if CFG_TUD_ECM_RNDIS
    [CONFIG_ID_RNDIS] = rndis_configuration,
    [CONFIG_ID_ECM]   = ecm_configuration
#else
    [CONFIG_ID_NCM] = ncm_configuration
#endif
};

// array of pointer to string descriptors
char const* string_desc_cdc_net_arr[] = {
    [STRID_LANGID]       = (const char[]) {0x09, 0x04},  // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "TinyUSB",                    // Manufacturer
    [STRID_PRODUCT]      = "TinyUSB Device",             // Product
    [STRID_SERIAL]       = "123456",                     // Serial
    [STRID_INTERFACE]    = "TinyUSB Network Interface"   // Interface Description

    // STRID_MAC index is handled separately
};
