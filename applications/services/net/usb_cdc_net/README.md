# usb_cdc_net

`usb_cdc_net` exposes the device to a USB host as a USB network adapter and runs a full TCP/IP stack on the device side. The host receives a virtual network interface; no physical Ethernet is required. The transport uses TinyUSB in device mode and supports three USB network class variants:

| Mode         | USB class                  | Host support |
|--------------|---------------------------|--------------|
| RNDIS        | CDC-RNDIS                  | Windows (primary) |
| ECM          | CDC-ECM                    | macOS, Linux, Android |
| NCM          | CDC-NCM                    | macOS 10.10+, Windows 10+, Linux |

The active mode is selected at compile time via `tusb_config.h` (see below). NCM is recommended for new designs because it is supported by all modern operating systems.

## CMake integration

### 1. Enable the sub-application

```cmake
set(RHS_APPLICATION_USB_CDC_NET ON)
```

The parent `net/CMakeLists.txt` adds `usb_cdc_net` as a sub-directory automatically.

### 2. Network address defaults (optional)

| CMake variable          | Default          | Description      |
|-------------------------|------------------|------------------|
| `RHS_CDC_NET_IP`        | `192.168.3.100`  | Device IP address |
| `RHS_CDC_NET_GATEWAY`   | `192.168.3.1`    | Default gateway  |
| `RHS_CDC_NET_NETMASK`   | `255.255.255.0`  | Network mask     |

Override in your toolchain or preset:

```cmake
set(RHS_CDC_NET_IP      "192.168.7.2"   CACHE STRING "" FORCE)
set(RHS_CDC_NET_GATEWAY "192.168.7.1"   CACHE STRING "" FORCE)
set(RHS_CDC_NET_NETMASK "255.255.255.0" CACHE STRING "" FORCE)
```

After the USB cable is plugged in, the host must be assigned an IP in the same subnet (e.g., `192.168.7.1`) manually or via DHCP if a DHCP server is added.

### 3. Required libraries

`usb_cdc_net` requires both `tinyusb` and `mongoose` targets to be available. The CMakeLists will abort with a fatal error if either is missing.

### 4. Mongoose compile-time flags

Applied to the `mongoose` target by `usb_cdc_net/CMakeLists.txt`:

| Definition            | Value | Notes |
|-----------------------|-------|-------|
| `MG_ENABLE_TCPIP`     | 1     | Built-in TCP/IP stack |
| `MG_ENABLE_PACKED_FS` | 1     | Packed (ROM) filesystem |
| `MG_IO_SIZE`          | 512   | I/O buffer size |

## TinyUSB configuration

Copy `tusb_config.h.example` to your application's include path as `tusb_config.h` and adjust as needed.

Key options:

| Define                  | Typical value | Notes |
|-------------------------|---------------|-------|
| `CFG_TUSB_MCU`          | `OPT_MCU_STM32F4` | Target MCU family |
| `CFG_TUSB_OS`           | `OPT_OS_FREERTOS` | RTOS integration |
| `CFG_TUD_ENABLED`       | 1             | Enable USB device mode |
| `CFG_TUD_ECM_RNDIS`     | 0 or 1        | 1 = RNDIS+ECM dual config; 0 = NCM |
| `CFG_TUD_NCM`           | `1 - CFG_TUD_ECM_RNDIS` | NCM when ECM_RNDIS is 0 |
| `CFG_TUD_NET_MTU`       | 1514          | Maximum Ethernet frame size |

To select NCM (recommended):

```c
#define CFG_TUD_ECM_RNDIS 0
#define CFG_TUD_NCM       1
```

To support Windows 7 / older hosts with RNDIS:

```c
#define CFG_TUD_ECM_RNDIS 1
#define CFG_TUD_NCM       0
```

The VID/PID is derived automatically from the enabled interface bitmap. When using the default `0xCafe` VID in production, replace it with a properly assigned VID from your organisation.

## API

```c
#include "usb_cdc_net.h"
```

### `Net *usb_cdc_net_start(const NetConfig *config)`

Registers the TinyUSB network interface (taking ownership of the `tud_network_*` callbacks), initialises Mongoose, and starts the worker thread.

- `config`: pass `NULL` to use the compile-time defaults (`RHS_CDC_NET_IP`, etc.). Individual empty fields are filled from compile-time defaults.

Returns a `Net *` handle valid until `usb_cdc_net_stop()` is called.

### `void usb_cdc_net_stop(Net *net)`

Deregisters the TinyUSB callbacks, stops the worker thread, and frees all resources. After this call the `net` pointer is invalid.

## Usage

### Minimal startup

```c
#include "usb_cdc_net.h"

Net *net = usb_cdc_net_start(NULL);
```

The device will be reachable at `RHS_CDC_NET_IP` once the USB cable is connected and the host assigns itself an IP in the same subnet.

### Custom IP address

```c
#include "usb_cdc_net.h"

NetConfig cfg = {
    .ip      = "192.168.7.2",
    .mask    = "255.255.255.0",
    .gateway = "192.168.7.1",
    .mac     = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE},
};
Net *net = usb_cdc_net_start(&cfg);
```

### Creating an application service

1. Add `set(RHS_APPLICATION_USB_CDC_NET ON)` to your toolchain.
2. Create `my_app/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_app C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC my_app.c)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    rhs
    rhs_hal
    cdc_net
)

service(my_app "my_app" 2048)
```

3. Implement `my_app/my_app.c`:

```c
#include "rhs.h"
#include "rhs_hal.h"
#include "usb_cdc_net.h"

#define TAG "my_app"

typedef struct {
    Net* net;
} MyApp;

static void http_handler(struct mg_connection *c, int ev,
                          void *ev_data, void *fn_data)
{
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = ev_data;
        mg_http_reply(c, 200, "", "Hello from USB network\n");
    }
}

int32_t my_app(void* context)
{
    MyApp* app = malloc(sizeof(MyApp));
    app->net   = usb_cdc_net_start(NULL);

    net_start_http(app->net, "http://0.0.0.0:80", http_handler, NULL);

    RHS_LOG_I(TAG, "USB CDC net started");

    // Application main loop
    for (;;) {
        rhs_delay(1000);
    }
}
```

### Registering a raw TCP listener

```c
static void tcp_handler(struct mg_connection *c, int ev,
                         void *ev_data, void *fn_data)
{
    if (ev == MG_EV_READ) {
        mg_send(c, c->recv.buf, c->recv.len);
        mg_iobuf_del(&c->recv, 0, c->recv.len);
    }
}

net_start_listener(net, "tcp://0.0.0.0:9000", tcp_handler, NULL);
```

### Changing IP address at runtime

```c
NetConfig cfg;
net_get_config(net, &cfg);
strncpy(cfg.ip, "192.168.7.10", sizeof(cfg.ip));
net_set_config(net, &cfg);
```

### Stopping the service

```c
usb_cdc_net_stop(net);
net = NULL;
```

## CLI and mDNS

When the worker thread starts it registers a CLI command named `cdc_net` and starts an mDNS responder under the same name. The device is discoverable on the USB network as `cdc_net.local`.

Connect via the device console and run:

```
cdc_net                        # print link state, IP, mask, gateway, MAC
cdc_net --ip 192.168.7.10      # change IP and restart the stack
cdc_net --restart              # restart with the existing config
```

From the host side you can reach the device by name instead of IP:

```sh
ping cdc_net.local
curl http://cdc_net.local/status
```

## Runtime exclusion with usb_eth_bridge

The TinyUSB network callbacks are owned by `rhs_hal_usb` and dispatched via `tud_net_dispatch`. Both `usb_cdc_net` and `usb_eth_bridge` may be compiled into the same binary, but only one may be active at a time. To switch between them, stop the current one and start the other:

```c
usb_cdc_net_stop(cdc_net);
cdc_net = NULL;
bridge = usb_eth_bridge_start(NULL);
```

`usb_cdc_net` and `eth_net` are fully independent and can run simultaneously. Each produces its own `Net*` handle and worker thread.

## Host-side setup

After plugging in the USB cable:

- **Linux/macOS**: A new network interface appears (e.g., `usb0` or `en5`). Assign a static IP in the same subnet as the device.
- **Windows (NCM)**: Requires Windows 10+. The interface appears as "Remote NDIS Compatible Device" or similar.
- **Windows (RNDIS)**: Works on older Windows versions. Select `CFG_TUD_ECM_RNDIS=1` in `tusb_config.h`.

Example Linux host configuration:

```sh
sudo ip addr add 192.168.3.1/24 dev usb0
sudo ip link set usb0 up
```
