#pragma once

#include "rhs_hal.h"
#include "data.h"
#include "od_common_defs.h"

#define RECORD_CAN_OPEN "can_open"

typedef struct CanOpenApp CanOpenApp;

void co_start_node(CanOpenApp* app, CO_Data* d, uint8_t id, RHSHalCANId can_id, uint32_t baud);

typedef void (*SDOCallback)(CO_Data* d, uint8_t id, uint16_t ind, uint8_t sub, uint32_t sz, const uint8_t* data);

// clang-format off
int co_get_sdo(CanOpenApp* app, CO_Data* d, uint8_t id, uint16_t ind, uint8_t sub, SDOCallback cb);
int co_set_sdo(CanOpenApp* app, CO_Data* d, uint8_t id, uint16_t ind, uint8_t sub, uint32_t sz, void* data, SDOCallback cb);
// clang-format on

int co_rpdo_config(CanOpenApp* app, CO_Data* d, uint8_t id, NPDO npdo, PDOField pdo_field, void* data);
int co_tpdo_config(CanOpenApp* app, CO_Data* d, uint8_t id, NPDO npdo, PDOField pdo_field, void* data);
int co_rpdo_map(CanOpenApp* app, CO_Data* d, uint8_t id, NPDO npdo, PDOMap nmap, uint16_t ind, uint8_t sub, uint8_t sz);

int co_tpdo_map(CanOpenApp* app, CO_Data* d, uint8_t id, NPDO npdo, PDOMap nmap, uint16_t ind, uint8_t sub, uint8_t sz);
int co_rpdo_map_group(CanOpenApp* app, CO_Data* d, uint8_t id, NPDO npdo, PDOMap nmap);
int co_tpdo_map_group(CanOpenApp* app, CO_Data* d, uint8_t id, NPDO npdo, PDOMap nmap);

#define OD_GET_FIELD_H(type, func_name) type func_name(CanOpenApp* app, CO_Data* d, uint16_t index, uint8_t subIndex)

OD_GET_FIELD_H(uint8_t, od_get_field_uns8);
OD_GET_FIELD_H(int8_t, od_get_field_s8);
OD_GET_FIELD_H(uint16_t, od_get_field_uns16);
OD_GET_FIELD_H(int16_t, od_get_field_s16);
OD_GET_FIELD_H(uint32_t, od_get_field_uns32);
OD_GET_FIELD_H(int32_t, od_get_field_s32);
OD_GET_FIELD_H(float, od_get_field_float);

uint8_t co_set_field(CanOpenApp* app, CO_Data* d, uint16_t ind, uint8_t sub, const void* data, uint32_t sz);
