#include "net_utils.h"
#include "net.h"

/**
 * @brief Start USB CDC network service with specified configuration.
 * @param config Pointer to NetConfig structure with desired network settings (IP, mask, gateway, MAC).
 * If NULL, defaults will be used.
 * @return Pointer to initialized Net structure representing the USB CDC network service.
 */
Net* usb_cdc_net_start(const NetConfig* config);

/**
 * @brief Stop USB CDC network service and free associated resources.
 * @param net Pointer to Net structure representing the USB CDC network service to stop.
 */
void usb_cdc_net_stop(Net* net);
