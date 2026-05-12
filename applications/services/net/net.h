#pragma once

#include <stdint.h>
#include "rhs.h"
#include "rhs_hal.h"
#include "mongoose.h"
#include "net_utils.h"

// Helper macro for MAC generation
#define GENERATE_LOCALLY_ADMINISTERED_MAC(UUID) \
    {2,                                         \
     UUID[0] ^ UUID[1],                         \
     UUID[2] ^ UUID[3],                         \
     UUID[4] ^ UUID[5],                         \
     UUID[6] ^ UUID[7] ^ UUID[8],               \
     UUID[9] ^ UUID[10] ^ UUID[11]}

typedef struct
{
    uint32_t ip;        // IP address (e.g., MG_IPV4(192, 168, 1, 100))
    uint32_t mask;      // Network mask (e.g., MG_IPV4(255, 255, 255, 0))
    uint32_t gateway;   // Gateway address (e.g., MG_IPV4(192, 168, 1, 1))
    uint8_t  mac[6];    // MAC address
} NetConfig;

typedef struct Net Net;

void net_start_http(Net* net, const char* uri, mg_event_handler_t fn, void* context);

void net_start_listener(Net* net, const char* uri, mg_event_handler_t fn, void* context);

void net_stop_listener(Net* net, const char* uri);

void net_set_config(Net* net, NetConfig* config);

void net_get_config(Net* net, NetConfig* config);
