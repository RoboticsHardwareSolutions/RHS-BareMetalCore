# eth_net

`eth_net` starts a full TCP/IP network stack on the STM32F physical Ethernet MAC using the RMII interface. It configures the GPIO pins, enables MAC clocks, initialises Mongoose, and spawns a dedicated FreeRTOS worker thread. The resulting `Net*` handle is the entry point for all higher-level services (HTTP listeners, Modbus TCP, etc.).

Supported MCUs: STM32F407, STM32F765. Adding support for other STM32 variants requires defining the RMII pin set in `eth_net.c`.

## CMake integration

### 1. Enable the sub-application

```cmake
set(RHS_APPLICATION_ETH_NET ON)
```

The parent `net/CMakeLists.txt` will add `eth_net` as a sub-directory automatically.

### 2. Network address defaults (optional)

These cache variables set the compile-time fallback values used when `eth_net_start()` is called with `net_config = NULL`, or when individual fields in `NetConfig` are left as empty strings.

| CMake variable         | Default          | Description      |
|------------------------|------------------|------------------|
| `RHS_ETH_NET_IP`       | `192.168.0.100`  | Static IP address |
| `RHS_ETH_NET_GATEWAY`  | `192.168.0.1`    | Default gateway  |
| `RHS_ETH_NET_NETMASK`  | `255.255.255.0`  | Network mask     |

Override them in your toolchain or preset file:

```cmake
set(RHS_ETH_NET_IP      "10.10.0.50"  CACHE STRING "" FORCE)
set(RHS_ETH_NET_GATEWAY "10.10.0.1"   CACHE STRING "" FORCE)
set(RHS_ETH_NET_NETMASK "255.255.0.0" CACHE STRING "" FORCE)
```

### 3. Mongoose compile-time flags

`eth_net/CMakeLists.txt` applies the following definitions to the `mongoose` target:

| Definition                    | Value              | Notes |
|-------------------------------|--------------------|-------|
| `MG_ENABLE_TCPIP`             | 1                  | Enable built-in TCP/IP stack |
| `MG_ENABLE_DRIVER_STM32F`     | 1                  | STM32F Ethernet MAC driver |
| `MG_ENABLE_TCPIP_DRIVER_INIT` | 0                  | Driver initialised manually (allows multiple interfaces) |
| `MG_TCPIP_PHY_ADDR`           | 1                  | Default PHY address on the MII bus |
| `MG_DRIVER_MDC_CR`            | 4                  | MDC clock divider (HCLK=168 MHz -> MDC ~2.1 MHz) |
| `MG_ENABLE_PACKED_FS`         | 1                  | Use packed (ROM) filesystem |
| `MG_ARCH`                     | `MG_ARCH_FREERTOS` | FreeRTOS integration |

If your hardware uses a different PHY address or HCLK frequency, pass an `EthPhyConfig` to `eth_net_start()` (see API below).

### 4. Required libraries

`eth_net` links against `mongoose`, `net`, `rhs`, and `rhs_hal`. All of these targets must be available before `add_subdirectory(eth_net)` is reached.

## API

```c
#include "eth_net.h"
```

### Structures

```c
// PHY-specific overrides (both fields: 0 = use compile-time default)
typedef struct {
    uint8_t phy_addr;  // PHY MII bus address
    uint8_t mdc_cr;    // MDC clock divider
} EthPhyConfig;

// Defined in net.h
typedef struct {
    char    ip[16];      // e.g. "192.168.0.100"
    char    mask[16];    // e.g. "255.255.255.0"
    char    gateway[16]; // e.g. "192.168.0.1"
    uint8_t mac[6];      // 6-byte MAC address (all zeros -> auto-generated)
} NetConfig;
```

### `Net *eth_net_start(const NetConfig *net_config, const EthPhyConfig *phy_config)`

Initialises the Ethernet hardware, configures the Mongoose TCP/IP interface, and starts the network worker thread.

- `net_config`: pass `NULL` to use compile-time defaults. Individual fields that are empty strings are also filled from compile-time defaults.
- `phy_config`: pass `NULL` to use `MG_TCPIP_PHY_ADDR` / `MG_DRIVER_MDC_CR`.

Returns a `Net *` handle valid until `eth_net_stop()` is called.

### `void eth_net_stop(Net *net)`

Signals the worker thread to stop, joins it, and frees all resources. After this call the `net` pointer is invalid.

### MAC address generation

If `NetConfig.mac` is all zeros (or `net_config` is `NULL`), a locally administered unicast MAC is derived from the MCU unique 96-bit hardware ID via `GENERATE_LOCALLY_ADMINISTERED_MAC`. The address is stable across power cycles for the same device. To use a fixed MAC, fill `NetConfig.mac` before calling `eth_net_start()`.

## Usage

### Minimal startup — all compile-time defaults

```c
#include "eth_net.h"

Net *net = eth_net_start(NULL, NULL);
```

### Custom IP, gateway and PHY

```c
#include "eth_net.h"

NetConfig net_cfg = {
    .ip      = "10.0.0.10",
    .mask    = "255.255.0.0",
    .gateway = "10.0.0.1",
    .mac     = {0x02, 0xDE, 0xAD, 0xBE, 0xEF, 0x01},
};

EthPhyConfig phy_cfg = {
    .phy_addr = 0,
    .mdc_cr   = 4,
};

Net *net = eth_net_start(&net_cfg, &phy_cfg);
```

### Creating an application service on top of eth_net

0. Set the `eth_net` service in your toolchain `set(RHS_APPLICATION_ETH_NET ON)`
1. Create a new folder for your application (e.g., `applications/my_app/`).
2. Add `add_subdirectory(my_app)` to the parent `CMakeLists.txt`.
3. Create `my_app/CMakeLists.txt`:

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
    eth_net
)

# Register as an rhs service; the framework creates a thread and calls my_app().
service(my_app "my_app" 2048)
```

4. Create `my_app/my_app.c`:

```c
#include "rhs.h"
#include "rhs_hal.h"
#include "eth_net.h"

#define TAG "my_app"

typedef enum {
    AppEventDataReceived = (1 << 0),
} AppEvent;

typedef struct {
    Net*         net;
    RHSEventFlag event;
} MyApp;

static MyApp* app_alloc(void)
{
    MyApp* app  = malloc(sizeof(MyApp));
    app->net    = eth_net_start(NULL, NULL);
    app->event  = rhs_event_flag_alloc();
    return app;
}

static void tcp_handler(struct mg_connection* c, int ev, void* ev_data, void* fn_data)
{
    MyApp* app = (MyApp*)fn_data;

    if (ev == MG_EV_READ) {
        // Echo data back to the sender
        mg_send(c, c->recv.buf, c->recv.len);
        mg_iobuf_del(&c->recv, 0, c->recv.len);

        rhs_event_flag_set(app->event, AppEventDataReceived);
    }
}

int32_t my_app(void* context)
{
    MyApp* app = app_alloc();
    RHS_LOG_I(TAG, "my_app started");

    net_start_listener(app->net, "tcp://0.0.0.0:2345", tcp_handler, app);

    for (;;) {
        uint32_t flags = rhs_event_flag_wait(
            app->event, AppEventDataReceived, RHSFlagWaitAny, RHSWaitForever);

        if (flags & AppEventDataReceived) {
            RHS_LOG_I(TAG, "data received");
        }
    }
}
```

### Registering an HTTP handler

```c
static void http_handler(struct mg_connection *c, int ev,
                          void *ev_data, void *fn_data)
{
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = ev_data;
        if (mg_match(hm->uri, mg_str("/status"), NULL)) {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                          "{\"ok\":true}\n");
        } else {
            mg_http_reply(c, 404, "", "Not found\n");
        }
    }
}

net_start_http(net, "http://0.0.0.0:80", http_handler, NULL);
```

### Changing IP address at runtime

```c
NetConfig cfg;
net_get_config(net, &cfg);
strncpy(cfg.ip, "192.168.1.55", sizeof(cfg.ip));
net_set_config(net, &cfg);  // network stack restarts with the new address
```

### Stopping the service

```c
eth_net_stop(net);
net = NULL;
```

## CLI commands

When the worker thread starts it registers a CLI command named `eth_net_<phy_addr>`. Connect via the device console and run:

```
eth_net_1                      # print link state, IP, mask, gateway, MAC
eth_net_1 --ip 192.168.0.200   # change IP and restart the stack
eth_net_1 --restart            # restart with the existing config
```

mDNS is started automatically; the device is reachable as `eth_net_1.local`.

## Hardware pin mapping (STM32F407)

| Pin  | Signal           |
|------|------------------|
| PA1  | ETH_RMII_REF_CLK |
| PA2  | ETH_RMII_MDIO    |
| PA7  | ETH_RMII_CRS_DV  |
| PB11 | ETH_RMII_TX_EN   |
| PB12 | ETH_RMII_TXD0    |
| PB13 | ETH_RMII_TXD1    |
| PC1  | ETH_RMII_MDC     |
| PC4  | ETH_RMII_RXD0    |
| PC5  | ETH_RMII_RXD1    |

`SYSCFG_PMC_MII_RMII_SEL` is set to select RMII mode. The Ethernet IRQ (`ETH_IRQn`) is enabled by the driver.
