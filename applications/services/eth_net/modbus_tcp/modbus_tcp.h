#pragma once
#include <stdint.h>
#include "mongoose.h"
#include "eth_net.h"

typedef struct
{
    void*    coils_data;
    uint16_t coils_size;
    void (*coils_read_cb)(uint16_t, uint8_t*, uint16_t, void*);
    void (*coils_write_cb)(uint16_t, uint8_t*, uint16_t, void*);
    void* coils_context;

    void*    holdings_data;
    uint16_t holdings_size;
    void (*holdings_cb)(uint16_t, uint16_t*, uint16_t, void*);
    void* holdings_context;

    void*    discrete_inputs_data;
    uint16_t discrete_inputs_size;
    void (*discrete_inputs_cb)(uint16_t, uint8_t*, uint16_t, void*);
    void* discrete_inputs_context;

} ModbusTcpApi;

void modbus_tcp_start(EthNet* app, ModbusTcpApi* api, uint16_t port);
