#pragma once

#include "mongoose.h"

#define RECORD_ETH_NET "eth_net"

typedef struct EthNet EthNet;

void eth_net_start_http(EthNet* eth_net, const char* uri, mg_event_handler_t fn, void* context);

void eth_net_start_listener(EthNet* eth_net, const char* uri, mg_event_handler_t fn, void* context);
