# usb_eth_bridge

`usb_eth_bridge` implements a transparent Layer-2 bridge between a USB CDC-Net interface (device side) and the STM32F physical Ethernet MAC. No IP stack runs on the MCU itself. The USB host receives a virtual network adapter and can communicate directly with devices on the physical LAN as if the MCU were a USB-to-Ethernet dongle.

```
USB host  <-- USB CDC-Net (RNDIS/ECM/NCM) -->  MCU  <-- RMII -->  Physical LAN
```

Ethernet frames arrive from the LAN via the STM32F MAC DMA and are forwarded to the USB host via TinyUSB. Frames from the USB host arrive via `tud_network_recv_cb` and are forwarded to the LAN via the STM32F MAC transmitter. The MAC is configured in promiscuous mode so all LAN frames are received, not only those addressed to the MCU's own MAC.

## When to use this instead of usb_cdc_net

| Scenario | Recommended module |
|---|---|
| Device needs its own HTTP/TCP server | `usb_cdc_net` |
| Device should be a transparent network bridge | `usb_eth_bridge` |
| Host needs to access a physical LAN through the device | `usb_eth_bridge` |

The TinyUSB network callbacks are owned by `rhs_hal_usb` and dispatched via `tud_net_dispatch`. Both `usb_eth_bridge` and `usb_cdc_net` may be compiled into the same binary, but only one may be active at a time. To switch:

```c
usb_eth_bridge_stop(bridge);
bridge = NULL;
cdc_net = usb_cdc_net_start(NULL);
```

`usb_eth_bridge` and `eth_net` are fully independent and can run simultaneously. Each owns a separate Mongoose manager and worker thread.

## CMake integration

### 1. Enable the sub-application

```cmake
set(RHS_APPLICATION_USB_ETH_BRIDGE ON)
```

The parent `net/CMakeLists.txt` adds `usb_eth_bridge` as a sub-directory. `RHS_APPLICATION_USB_CDC_NET` and `RHS_APPLICATION_ETH_NET` may be enabled at the same time. `usb_cdc_net` and `usb_eth_bridge` can coexist in the same binary but cannot be running simultaneously.

### 2. Required libraries

`usb_eth_bridge` requires both `tinyusb` and `mongoose` targets. The CMakeLists will abort with a fatal error if either is missing.

### 3. Mongoose compile-time flags

Applied to the `mongoose` target:

| Definition                    | Value              | Notes |
|-------------------------------|--------------------|-------|
| `MG_ENABLE_TCPIP`             | 1                  | TCP/IP stack (used for ETH driver only; no sockets opened by bridge) |
| `MG_ENABLE_DRIVER_STM32F`     | 1                  | STM32F Ethernet MAC driver |
| `MG_ENABLE_TCPIP_DRIVER_INIT` | 0                  | Driver initialised manually |
| `MG_TCPIP_PHY_ADDR`           | 1                  | Default PHY address |
| `MG_DRIVER_MDC_CR`            | 4                  | MDC clock divider (HCLK=168 MHz) |
| `MG_ENABLE_PACKED_FS`         | 1                  | Packed filesystem |

### 4. TinyUSB configuration

Use the same `tusb_config.h` structure as for `usb_cdc_net`. Choose RNDIS/ECM or NCM via `CFG_TUD_ECM_RNDIS` / `CFG_TUD_NCM`. NCM is preferred.

## API

```c
#include "usb_eth_bridge.h"
```

### Structures

```c
// PHY overrides (0 = use compile-time defaults)
typedef struct {
    uint8_t phy_addr;
    uint8_t mdc_cr;
} UsbEthBridgePhyConfig;
```

### `UsbEthBridge *usb_eth_bridge_start(const UsbEthBridgePhyConfig *phy_config)`

Initialises the Ethernet hardware, registers TinyUSB network callbacks via `tud_net_dispatch_set()`, and starts the bridge worker thread.

- `phy_config`: pass `NULL` to use `MG_TCPIP_PHY_ADDR` / `MG_DRIVER_MDC_CR`.

Returns an opaque `UsbEthBridge *` handle valid until `usb_eth_bridge_stop()` is called.

### `void usb_eth_bridge_stop(UsbEthBridge *bridge)`

Clears TinyUSB network callbacks, signals the worker thread to stop, joins it, and frees all resources. After this call the handle is invalid.

## Usage

### Minimal startup

```c
#include "usb_eth_bridge.h"

UsbEthBridge *bridge = usb_eth_bridge_start(NULL);
```

Once the USB cable is connected the host sees a network adapter. Frames flow transparently between the USB host and the physical LAN.

### Custom PHY settings

```c
#include "usb_eth_bridge.h"

UsbEthBridgePhyConfig phy = {
    .phy_addr = 0,
    .mdc_cr   = 4,
};
UsbEthBridge *bridge = usb_eth_bridge_start(&phy);
```

### Creating an application service

1. Add `set(RHS_APPLICATION_USB_ETH_BRIDGE ON)` to your toolchain.
2. Create `my_bridge/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_bridge C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC my_bridge.c)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    rhs
    rhs_hal
    usb_eth_bridge
)

service(my_bridge "my_bridge" 2048)
```

3. Implement `my_bridge/my_bridge.c`:

```c
#include "rhs.h"
#include "usb_eth_bridge.h"

#define TAG "my_bridge"

int32_t my_bridge(void* context)
{
    UsbEthBridge* bridge = usb_eth_bridge_start(NULL);
    RHS_LOG_I(TAG, "USB-ETH bridge started");

    // The bridge worker thread handles all frame forwarding.
    for (;;) {
        // Do other work here if needed, or just sleep.
    }
}
```

### Stopping the bridge

```c
usb_eth_bridge_stop(bridge);
bridge = NULL;
```

## Host-side behaviour

The USB host sees a standard USB network adapter. No special drivers are required on Linux or macOS. On Windows, NCM requires Windows 10+ (auto-bound via the MS OS 2.0 descriptor embedded in the firmware); RNDIS works on older versions.

Example Linux host network configuration:

```sh
sudo ip link set usb0 up
# The host is now directly bridged to the physical LAN.
# Obtain an address via DHCP or assign one statically:
sudo dhclient usb0
```
