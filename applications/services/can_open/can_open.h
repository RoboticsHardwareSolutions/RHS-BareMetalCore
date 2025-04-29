#pragma once

#include "rhs_hal.h"
#include "data.h"
#include "od_common_defs.h"

void can_open_start_node(CO_Data* d, uint8_t node_id, RHSHalCANId id, uint32_t baud);

typedef void (*sdo_cb)(CO_Data* d, uint16_t index, uint8_t subindex, uint32_t count, const uint8_t* data);

uint8_t can_open_read_sdo(CO_Data* d, uint8_t node_id, uint16_t index, uint8_t subindex, sdo_cb cb);

uint8_t can_open_write_sdo(CO_Data* d,
                           uint8_t  node_id,
                           uint16_t index,
                           uint8_t  subindex,
                           uint32_t count,
                           void*    data,
                           sdo_cb   cb);

#define OD_GET_FIELD_H(type, func_name) type func_name(CO_Data* d, uint16_t index, uint8_t subIndex)

OD_GET_FIELD_H(uint8_t, od_get_field_uns8);
OD_GET_FIELD_H(int8_t, od_get_field_s8);
OD_GET_FIELD_H(uint16_t, od_get_field_uns16);
OD_GET_FIELD_H(int16_t, od_get_field_s16);
OD_GET_FIELD_H(uint32_t, od_get_field_uns32);
OD_GET_FIELD_H(int32_t, od_get_field_s32);
OD_GET_FIELD_H(float, od_get_field_float);

uint8_t can_open_set_field(CO_Data* d, uint16_t index, uint8_t subIndex, const void* data, uint32_t size_data);

typedef struct CanOpenApp CanOpenApp;
