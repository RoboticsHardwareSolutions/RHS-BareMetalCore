#pragma once

#include "mongoose.h"
#include "net_utils.h"
#include "net.h"

// Ethernet-specific PHY configuration (optional)
typedef struct
{
    uint8_t phy_addr;  // PHY address (NULL = use default MG_TCPIP_PHY_ADDR)
    uint8_t mdc_cr;    // MDC clock divider (NULL = use default MG_DRIVER_MDC_CR)
} EthPhyConfig;

/**
 * @brief Start Ethernet network service with specified configuration.
 * @param net_config Pointer to NetConfig structure with desired network settings (IP, mask, gateway, MAC).
 * If NULL, defaults will be used.
 * @param phy_config Pointer to EthPhyConfig structure with desired PHY settings.
 * If NULL, defaults will be used.
 * @return Pointer to initialized Net structure representing the Ethernet network service.
 */
Net* eth_net_start(const NetConfig* net_config, const EthPhyConfig* phy_config);

/**
 * @brief Stop Ethernet network service and free associated resources.
 * @param net Pointer to Net structure representing the Ethernet network service to stop.
 */
void eth_net_stop(Net* net);
