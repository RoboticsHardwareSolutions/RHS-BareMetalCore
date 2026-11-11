#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "rhs_hal.h"

typedef struct UsbBridge UsbBridge;

/** What the USB side is bridged to */
typedef enum
{
    UsbBridgeTargetCan,   /**< CAN, host side SLCAN over USB CDC */
    UsbBridgeTargetRs232, /**< RS232, raw bytes over USB CDC */
    UsbBridgeTargetRs485, /**< RS485, raw bytes over USB CDC */
    UsbBridgeTargetEth,   /**< Ethernet, L2 frames over USB NCM (boards with RHS_HAL_ETH) */
} UsbBridgeTarget;

typedef enum
{
    UsbBridgeModeBridge,    /**< USB <-> device */
    UsbBridgeModeApp,       /**< USB <-> application, the device is not used */
    UsbBridgeModeBridgeApp, /**< USB <-> device, the application sees both directions and sends to either */
} UsbBridgeMode;

/** Sides of the bridge, bits: usb_bridge_*_send() takes a combination */
typedef enum
{
    UsbBridgePortUsb    = (1 << 0),
    UsbBridgePortDevice = (1 << 1), /**< CAN, UART or Ethernet MAC */
} UsbBridgePort;

typedef struct
{
    UsbBridgeTarget target;
    UsbBridgeMode   mode;
    uint32_t        baudrate; /**< CAN bitrate / UART baud until the host changes it (Sn / line coding), unused for Eth */
} UsbBridgeConfig;

/** Counters are frames for CAN and Eth, bytes for RS232/RS485 */
typedef struct
{
    bool     open;     /**< CAN: SLCAN channel opened (O); serial: host DTR; Eth: USB network up */
    uint32_t baudrate; /**< Current CAN bitrate / UART baud */

    uint32_t usb_rx;       /**< From the host */
    uint32_t usb_tx;       /**< To the host */
    uint32_t usb_tx_drops; /**< Not sent to the host: channel closed or host not reading */

    uint32_t device_rx;        /**< From the device */
    uint32_t device_rx_drops;  /**< Lost from the device: RX queue full */
    uint32_t device_tx;        /**< To the device */
    uint32_t device_tx_errors; /**< Not sent to the device */
    uint32_t device_errors;    /**< Bus / line error events (CAN: error warning, error passive, bus-off) */

    uint32_t app_rx; /**< Passed to the application callback */
    uint32_t app_tx; /**< Sent by the application */
} UsbBridgeState;

/** Can @p cfg start now
 * @return NULL when it can, otherwise why not: USB not available (BMPLC_XL with strap PG10 low), another bridge
 *         running, or the target / mode is not supported on this board (no Ethernet, no UART: serial only in App
 *         mode, BMPLC_M: CAN only in App mode, CAN and USB share SRAM on STM32F103) */
const char* usb_bridge_check(const UsbBridgeConfig* cfg);

/** Start a bridge. One bridge at a time.
 * @return handle or NULL when usb_bridge_check() fails, the reason is logged */
UsbBridge* usb_bridge_start(const UsbBridgeConfig* cfg);

/** Stop the bridge, release USB and the device, free the handle */
void usb_bridge_stop(UsbBridge* bridge);

void usb_bridge_get_config(UsbBridge* bridge, UsbBridgeConfig* cfg);

void usb_bridge_get_state(UsbBridge* bridge, UsbBridgeState* state);

const char* usb_bridge_target_name(UsbBridgeTarget target);

const char* usb_bridge_mode_name(UsbBridgeMode mode);

/* Application side (UsbBridgeModeApp / UsbBridgeModeBridgeApp).
 * Callbacks run in the bridge thread; *_send() called from a callback sends right away, from another thread the data
 * is queued (false: queue full). Sending to UsbBridgePortDevice in UsbBridgeModeApp fails. */

/** CAN target */
typedef void (*UsbBridgeCanCallback)(UsbBridgePort from, const RHSHalCANFrameType* frame, void* context);

void usb_bridge_set_can_callback(UsbBridge* bridge, UsbBridgeCanCallback callback, void* context);

bool usb_bridge_can_send(UsbBridge* bridge, uint8_t to, const RHSHalCANFrameType* frame);

/** RS232 / RS485 targets: bytes. Eth target: one L2 frame per call. */
typedef void (*UsbBridgeDataCallback)(UsbBridgePort from, const uint8_t* data, size_t len, void* context);

void usb_bridge_set_data_callback(UsbBridge* bridge, UsbBridgeDataCallback callback, void* context);

bool usb_bridge_data_send(UsbBridge* bridge, uint8_t to, const uint8_t* data, size_t len);
