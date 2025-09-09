#include "tusb.h"

// Configuration mode
// 0 : enumerated as Network (RNDIS/ECM/NCM). Board button is not pressed when enumerating
// 1 : enumerated as Dual CDC. Board button is pressed when enumerating
static tusb_desc_device_t desc;
static uint8_t const**    config          = NULL;
static char const**       string_desc_arr = NULL;

// Switches the USB descriptor mode and updates pointers to descriptors
void descriptor_switch_mode(tusb_desc_device_t* new_desc, uint8_t const** new_config, char const** new_string_desc_arr)
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
    return config[index];
}

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    static uint16_t _desc_str[32];
    unsigned int    chr_count = 0;

    if (STRID_LANGID == index)
    {
        memcpy(&_desc_str[1], string_desc_arr[STRID_LANGID], 2);
        chr_count = 1;
    }
#if CFG_TUD_ECM_RNDIS || CFG_TUD_NCM
    else if (STRID_MAC == index)
    {
        // Convert MAC address into UTF-16

        for (unsigned i = 0; i < sizeof(tud_network_mac_address); i++)
        {
            _desc_str[1 + chr_count++] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 4) & 0xf];
            _desc_str[1 + chr_count++] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 0) & 0xf];
        }
    }
#endif
    else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char* str = string_desc_arr[index];

        // Cap at max char
        chr_count = (uint8_t) strlen(str);
        if (chr_count > (TU_ARRAY_SIZE(_desc_str) - 1))
            chr_count = TU_ARRAY_SIZE(_desc_str) - 1;

        // Convert ASCII string into UTF-16
        for (unsigned int i = 0; i < chr_count; i++)
        {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}
