#pragma once
#include "stdint.h"

typedef struct
{
    uint8_t phy_addr;  // PHY address (NULL = use default MG_TCPIP_PHY_ADDR)
    uint8_t mdc_cr;    // MDC clock divider (NULL = use default MG_DRIVER_MDC_CR)
} UsbEthBridgePhyConfig;

/**
 * @brief Opaque bridge handle.
 */
typedef struct UsbEthBridge UsbEthBridge;

/**
 * @brief Start a Layer-2 bridge between USB CDC-Net and physical Ethernet.
 *
 * The device presents itself to the USB host as a network adapter (same USB
 * class as usb_cdc_net) and transparently forwards Ethernet frames in both
 * directions between the USB host and the physical LAN.  No IP stack is
 * instantiated on the device itself.
 *
 * @note This module owns the TinyUSB network callbacks (tud_network_recv_cb,
 *       tud_network_xmit_cb, tud_network_init_cb).  Do NOT compile
 *       usb_cdc_net.c in the same build - they define the same symbols.
 *
 * @param phy_config  PHY settings (NULL -> use defaults MG_TCPIP_PHY_ADDR /
 *                    MG_DRIVER_MDC_CR).
 * @return  Handle to the running bridge.
 */
UsbEthBridge* usb_eth_bridge_start(const UsbEthBridgePhyConfig* phy_config);

/**
 * @brief Stop the bridge and release all resources.
 * @param bridge  Handle returned by usb_eth_bridge_start().
 */
void usb_eth_bridge_stop(UsbEthBridge* bridge);
