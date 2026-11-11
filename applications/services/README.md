# Services

Every service in this directory follows the same layout, so a new one can be started by copying an existing one.

## Layout

```
<name>/
├── CMakeLists.txt     # library target <name>, registration via service() / start_up()
├── README.md          # what the service does, CMake option, API, CLI commands
├── <name>.h           # public API: types, functions, RECORD_<NAME>
├── <name>_i.h         # private types shared by several .c files (only if there are several)
├── <name>.c           # alloc/free, entry point, public API
└── <name>_<part>.c    # optional parts of a big service: _cli.c, _can.c, _messages.c, ...
```

| Item | Convention | Example |
|---|---|---|
| Directory / CMake target / `project()` | `<name>` | `notification` |
| CMake option for optional services | `RHS_SERVICE_<NAME>` | `RHS_SERVICE_NOTIFICATION` |
| Persistent task entry point | `int32_t <name>_srv(void* context)` | `notification_srv` |
| Task name in `service()` | `"<name>"` | `service(notification_srv "notification" 1024)` |
| Startup hook (no own task) | `void <name>_start_up(void)` | `usb_bridge_start_up` |
| Record name | `RECORD_<NAME>` with value `"<name>"` | `RECORD_CLI "cli"` |
| Log tag | `#define TAG "<name>"` in each `.c`, never in a header | `#define TAG "net"` |
| Public functions / types | `<name>_*` / `<Name>*` | `usb_bridge_start()`, `UsbBridgeConfig` |
| Headers | `#pragma once` | |

## CMakeLists.txt template

```cmake
cmake_minimum_required(VERSION 3.24)
project(<name> C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC
        <name>.c
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
)

service(<name>_srv "<name>" 2048)   # or start_up(<name>_start_up)
```

An optional service is added in `services/CMakeLists.txt` with
`service_option(RHS_SERVICE_<NAME> <name>)` (see `cmake/rhs.cmake`): it prints the ON/OFF line,
exports `RHS_SERVICE_<NAME>` as a compile definition of `rhs` and adds the subdirectory.

## Services

| Service | CMake option | Kind | README |
|---|---|---|---|
| `cli` | always on | task `cli_srv` | [cli/README.md](cli/README.md) |
| `loader` | always on | task `loader_srv` | [loader/README.md](loader/README.md) |
| `notification` | `RHS_SERVICE_NOTIFICATION` | task `notification_srv` | [notification/README.md](notification/README.md) |
| `usb_bridge` | `RHS_SERVICE_USB_BRIDGE` | start-up `usb_bridge_start_up` | [usb_bridge/README.md](usb_bridge/README.md) |
| `net` and its backends | `RHS_SERVICE_ETH_NET` / `RHS_SERVICE_USB_CDC_NET` | library + worker threads | [net/README.md](net/README.md) |

## Renamed in this layout

Old CMake options are still accepted with a deprecation warning (`cmake/rhs.cmake`), C API names are not aliased.

| Old | New |
|---|---|
| `RHS_APPLICATION_ETH_NET` / `USB_CDC_NET` | `RHS_SERVICE_ETH_NET` / `USB_CDC_NET` |
| `RHS_APPLICATION_USB_ETH_BRIDGE`, `RHS_SERVICE_USB_ETH_BRIDGE` / `USB_SERIAL_BRIDGE` / `USB_CAN_BRIDGE` | `RHS_SERVICE_USB_BRIDGE` |
| `RHS_CDC_NET_IP` / `GATEWAY` / `NETMASK` | `RHS_USB_CDC_NET_IP` / `GATEWAY` / `NETMASK` |
| `CDC_NET_*_STRING` | `USB_CDC_NET_*_STRING` |
| CMake target `cdc_net` | `usb_cdc_net` |
| `usb_serial_enable` / `usb_serial_disable`, `usb_serial_bridge_start` / `stop` | `usb_bridge_start` / `usb_bridge_stop` (target RS232 / RS485) |
| `usb_serial_set_config` / `get_config` / `get_state` | `usb_bridge_get_config` / `get_state` |
| `usb_eth_bridge_start` / `stop`, `RHS_SERVICE_USB_ETH_BRIDGE` | `usb_bridge_start` / `stop` (target Eth), `RHS_SERVICE_USB_BRIDGE` |
| `UsbSerialConfig` / `UsbSerialState` | `UsbSerialBridgeConfig` / `UsbSerialBridgeState` |
| `EthPhyConfig` | `EthNetPhyConfig` |
