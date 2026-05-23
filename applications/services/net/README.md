# Module Net

The `net` service provides a unified networking layer for embedded firmware running on STM32 microcontrollers. It wraps the Mongoose event-driven network stack and exposes a simple API for registering HTTP and raw TCP listeners. Three transport back-ends are available as independent sub-applications and can all be compiled into the same binary. `usb_eth_bridge` and `usb_cdc_net` share the USB interface at runtime, so only one of them may be active at a time; switching between them is done by calling the respective `_stop` / `_start` functions.

## Module tree

```
net/
├── CMakeLists.txt          # Top-level build file; gates sub-apps on CMake options
├── net.h                   # Public API: NetConfig, net_start_http/listener, net_set_config
├── net.c                   # Core worker thread and message-queue dispatch
├── net_i.h                 # Internal types (Net struct, event queue messages)
│
├── eth_net/                # Transport: physical Ethernet via STM32F RMII MAC
│   ├── CMakeLists.txt
│   ├── eth_net.h
│   └── eth_net.c
│
├── usb_cdc_net/            # Transport: device-mode USB network (RNDIS / ECM / NCM)
│   ├── CMakeLists.txt
│   ├── tusb_config.h.example
│   ├── usb_cdc_net.h
│   └── usb_cdc_net.c
│
├── usb_eth_bridge/         # Layer-2 bridge: USB CDC-Net <-> physical Ethernet (no IP stack)
│   ├── CMakeLists.txt
│   ├── usb_eth_bridge.h
│   └── usb_eth_bridge.c
│
├── net_listeners/          # Internal: linked-list of active Mongoose connection handlers
│   ├── net_listeners.h
│   └── net_listeners.c
│
├── net_utils/              # IP address string parsing and validation helpers
│   ├── net_utils.h
│   └── net_utils.c
│
└── modbus_tcp/             # Modbus TCP server that runs on top of a Net instance
    ├── modbus_tcp.h
    └── modbus_tcp.c
```

## Sub-application selection

Enable any combination of transport back-ends in your CMake configuration:

| CMake option                        | Transport                          |
|-------------------------------------|------------------------------------|
| `RHS_APPLICATION_ETH_NET=ON`        | Physical Ethernet (RMII)           |
| `RHS_APPLICATION_USB_CDC_NET=ON`    | USB device network (RNDIS/ECM/NCM) |
| `RHS_APPLICATION_USB_ETH_BRIDGE=ON` | Layer-2 USB-to-Ethernet bridge     |

The `net` core library is built automatically when any of these options is ON. All three options may be enabled simultaneously in the same binary.

The TinyUSB network callbacks (`tud_network_recv_cb`, `tud_network_xmit_cb`, `tud_network_init_cb`) are owned by `rhs_hal_usb` and dispatched via `tud_net_dispatch`. `usb_cdc_net` and `usb_eth_bridge` each register their handlers at runtime using `tud_net_dispatch_set()`. Only one of them may be active at a time; use the respective `_stop` / `_start` pair to switch.

## Quick examples

### Ethernet — static IP, HTTP handler

```c
#include "eth_net.h"

static void my_http_handler(struct mg_connection *c, int ev,
                             void *ev_data, void *fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = ev_data;
        mg_http_reply(c, 200, "", "Hello\n");
    }
}

// In application init:
Net *net = eth_net_start(NULL, NULL);          // use compile-time defaults
net_start_http(net, "http://0.0.0.0:80", my_http_handler, NULL);
```

### USB CDC network — custom IP

```c
#include "usb_cdc_net.h"

NetConfig cfg = {
    .ip      = "192.168.3.200",
    .mask    = "255.255.255.0",
    .gateway = "192.168.3.1",
    .mac     = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE},
};
Net *net = usb_cdc_net_start(&cfg);
net_start_http(net, "http://0.0.0.0:80", my_http_handler, NULL);
```

### USB-to-Ethernet bridge

```c
#include "usb_eth_bridge.h"

UsbEthBridge *bridge = usb_eth_bridge_start(NULL);
// The device is now a transparent L2 bridge; no IP stack on the MCU.
```

### Adding a Modbus TCP server to an existing Net instance

```c
#include "modbus_tcp.h"

static uint8_t  coils[16]    = {0};
static uint16_t holdings[32] = {0};

ModbusTcpApi modbus = {
    .coils_data     = coils,
    .coils_size     = 16,
    .holdings_data  = holdings,
    .holdings_size  = 32,
};

modbus_tcp_start(net, &modbus, 502);
```

### Dynamic IP change at runtime

```c
NetConfig new_cfg;
net_get_config(net, &new_cfg);
strncpy(new_cfg.ip, "10.0.0.50", sizeof(new_cfg.ip));
net_set_config(net, &new_cfg);   // restarts the network stack
```

## Core API summary

| Function | Description |
|---|---|
| `net_start_http(net, uri, fn, ctx)` | Register an HTTP listener on the given URI |
| `net_start_listener(net, uri, fn, ctx)` | Register a raw TCP listener |
| `net_stop_listener(net, uri)` | Remove a previously registered listener |
| `net_set_config(net, config)` | Apply a new `NetConfig` and restart the stack |
| `net_get_config(net, config)` | Read the current `NetConfig` |

See the sub-module README files for full configuration and usage details:
- [eth_net/README.md](eth_net/README.md)
- [usb_cdc_net/README.md](usb_cdc_net/README.md)
- [usb_eth_bridge/README.md](usb_eth_bridge/README.md)

