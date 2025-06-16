#pragma once

#include "rhs_hal.h"
#include "data.h"
#include "od_common_defs.h"

#define RECORD_CAN_OPEN "can_open"

typedef struct CanOpenApp CanOpenApp;

void can_open_start_node(CanOpenApp* app, CO_Data* d, uint8_t node_id, RHSHalCANId id, uint32_t baud);

typedef void (
    *sdo_cb)(CO_Data* d, uint8_t node_id, uint16_t index, uint8_t subindex, uint32_t count, const uint8_t* data);

uint8_t can_open_read_sdo(CanOpenApp* app, CO_Data* d, uint8_t node_id, uint16_t index, uint8_t subindex, sdo_cb cb);

uint8_t can_open_write_sdo(CanOpenApp* app,
                           CO_Data*    d,
                           uint8_t     node_id,
                           uint16_t    index,
                           uint8_t     subindex,
                           uint32_t    count,
                           void*       data,
                           sdo_cb      cb);

int sdo_set_rx_pdo(CanOpenApp* app, CO_Data* d, PDOField pdo_field, NumPDOType pdo, uint8_t node_id, void* data);
int sdo_set_tx_pdo(CanOpenApp* app, CO_Data* d, PDOField pdo_field, NumPDOType pdo, uint8_t node_id, void* data);
int sdo_set_rx_pdo_map(CanOpenApp* app,
                       CO_Data*    d,
                       uint8_t     node_id,
                       NumPDOType  num_pdo,
                       PDOMapType  num_map,
                       uint16_t    index,
                       uint8_t     subindex,
                       uint8_t     size);

int sdo_set_tx_pdo_map(CanOpenApp* app,
                       CO_Data*    d,
                       uint8_t     node_id,
                       NumPDOType  num_pdo,
                       PDOMapType  num_map,
                       uint16_t    index,
                       uint8_t     subindex,
                       uint8_t     size);
int sdo_set_rx_pdo_map_group(CanOpenApp* app, CO_Data* d, uint8_t node_id, NumPDOType num_pdo, PDOMapType num_map);
int sdo_set_tx_pdo_map_group(CanOpenApp* app, CO_Data* d, uint8_t node_id, NumPDOType num_pdo, PDOMapType num_map);

#define OD_GET_FIELD_H(type, func_name) type func_name(CanOpenApp* app, CO_Data* d, uint16_t index, uint8_t subIndex)

OD_GET_FIELD_H(uint8_t, od_get_field_uns8);
OD_GET_FIELD_H(int8_t, od_get_field_s8);
OD_GET_FIELD_H(uint16_t, od_get_field_uns16);
OD_GET_FIELD_H(int16_t, od_get_field_s16);
OD_GET_FIELD_H(uint32_t, od_get_field_uns32);
OD_GET_FIELD_H(int32_t, od_get_field_s32);
OD_GET_FIELD_H(float, od_get_field_float);

uint8_t can_open_set_field(CanOpenApp* app, CO_Data* d, uint16_t index, uint8_t subIndex, const void* data, uint32_t size_data);
