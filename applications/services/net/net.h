#pragma once

#include <stdint.h>
#include "rhs.h"
#include "rhs_hal.h"
#include "mongoose.h"


typedef struct
{
    uint32_t ip;        // IP address (e.g., MG_IPV4(192, 168, 1, 100))
    uint32_t mask;      // Network mask (e.g., MG_IPV4(255, 255, 255, 0))
    uint32_t gateway;   // Gateway address (e.g., MG_IPV4(192, 168, 1, 1))
    uint8_t  mac[6];    // MAC address
    uint8_t  phy_addr;  // PHY address
    uint8_t  mdc_cr;    // MDC clock divider
} NetConfig;

typedef struct Net Net;

void net_start_http(Net* net, const char* uri, mg_event_handler_t fn, void* context);

void net_start_listener(Net* net, const char* uri, mg_event_handler_t fn, void* context);

void net_stop_listener(Net* net, const char* uri);

void net_set_config(Net* net, NetConfig* config);

void net_get_config(Net* net, NetConfig* config);
