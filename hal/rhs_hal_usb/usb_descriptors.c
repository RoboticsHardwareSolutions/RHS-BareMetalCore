#include "tusb.h"
#include "../rhs_hal_version/rhs_hal_version.h"

// Configuration mode
// 0 : enumerated as Network (RNDIS/ECM/NCM). Board button is not pressed when enumerating
// 1 : enumerated as Dual CDC. Board button is pressed when enumerating
static tusb_desc_device_t desc;
static uint8_t const**    config                 = NULL;
static char const**       string_desc_arr        = NULL;
static size_t             string_desc_arr_count  = 0;

// Switches the USB descriptor mode and updates pointers to descriptors
void descriptor_switch_mode(tusb_desc_device_t* new_desc, uint8_t const** new_config, char const** new_string_desc_arr, size_t new_string_desc_arr_count)
{
    if (new_desc)
        desc = *new_desc;

    if (new_config)
        config = new_config;
    else
        config = NULL;  // or set to a default configuration if available

    if (new_string_desc_arr)
        string_desc_arr = new_string_desc_arr;
    else
        string_desc_arr = NULL;  // or set to a default string descriptor array if available

    string_desc_arr_count = new_string_desc_arr_count;
}

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void)
{
    return (uint8_t const*) (&desc);
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    if (index >= desc.bNumConfigurations)
        return NULL;
    return config[index];
}

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    static uint16_t _desc_str[32];
    unsigned int    chr_count = 0;

    switch (index)
    {
    case STRID_LANGID:
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
        break;

    case STRID_SERIAL:
    {
        static const char hex[]     = "0123456789ABCDEF";
        const uint8_t*    uid       = rhs_hal_version_uid();
        size_t            out       = 0;
        const size_t      max_chars = (sizeof(_desc_str) / sizeof(_desc_str[0])) - 1;

        if (uid)
        {
            // STM32 UID is 96-bit (12 bytes); represent it as 24 hex characters.
            for (size_t i = 0; i < 12 && (out + 1) < max_chars; i++)
            {
                _desc_str[1 + out++] = (uint16_t) hex[(uid[i] >> 4) & 0x0F];
                _desc_str[1 + out++] = (uint16_t) hex[uid[i] & 0x0F];
            }
        }

        chr_count = (unsigned int) out;
        break;
    }
#if CFG_TUD_ECM_RNDIS || CFG_TUD_NCM
    case STRID_MAC:
        // Convert MAC address into UTF-16
        for (unsigned i = 0; i < sizeof(tud_network_mac_address); i++)
        {
            _desc_str[1 + chr_count++] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 4) & 0xf];
            _desc_str[1 + chr_count++] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 0) & 0xf];
        }
        break;
#endif
    default:
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (index >= string_desc_arr_count)
        {
            return NULL;
        }

        const char* str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);

        const size_t max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;  // -1 for string type
        if (chr_count > max_count)
        {
            chr_count = max_count;
        }

        // Convert ASCII string into UTF-16
        for (size_t i = 0; i < chr_count; i++)
        {
            _desc_str[1 + i] = str[i];
        }
        break;
    }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}
