#include "tusb.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
extern tusb_desc_device_t const desc_cdc_net;


#if CFG_TUD_ECM_RNDIS

extern uint8_t const rndis_configuration[];
extern uint8_t const ecm_configuration[];

#else

extern uint8_t const ncm_configuration[];

#endif

// Configuration array: RNDIS and CDC-ECM
// - Windows only works with RNDIS
// - MacOS only works with CDC-ECM
// - Linux will work on both
extern uint8_t const* const configuration_arr[2];

// array of pointer to string descriptors
extern char const* string_desc_cdc_net_arr[];
