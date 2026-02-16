#pragma once
#include "stdint.h"

/** RxPDO SETTINGS **/
/* RxPDO1 */

typedef enum
{
    NumPDO1,
    NumPDO2,
    NumPDO3,
    NumPDO4,
    NumPDOMax
} NPDO;

typedef enum
{
    PDOMapNum,
    PDOMap1,
    PDOMap2,
    PDOMap3,
    PDOMap4,
    PDOMap5,
    PDOMap6,
    PDOMap7,
    PDOMap8,
    PDOMapMax
} PDOMap;

typedef enum
{
    PDOFieldConfig,
    PDOFieldId,
    PDOFieldType,
    PDOFieldInhibit,
    PDOFieldRes,
    PDOFieldEventTime,
} PDOField;

typedef struct
{
    uint8_t sub;
    uint8_t size;
} PDOSubField;

typedef struct
{
    uint16_t index;
    uint8_t  sub;
    uint8_t  size;
} ODFieldType;

extern const ODFieldType od_device_type;
extern const ODFieldType od_device_name;
extern const ODFieldType od_producer_hb;
extern const ODFieldType od_device_vendor;
extern const ODFieldType od_device_code;
extern const ODFieldType od_device_revision;
extern const ODFieldType od_device_serial;

/* SDO server parameter */
extern const ODFieldType od_sdo_stx;
extern const ODFieldType od_sdo_srx;
extern const ODFieldType od_sdo_srv_id;

#define SET_SDO_COMMUNICATION(co, OD, id)                                                                            \
    co_set_field((co), (OD), od_sdo_stx.index, od_sdo_stx.sub, (uint32_t[]) {0x600 + (id)}, sizeof(uint32_t)); \
    co_set_field((co), (OD), od_sdo_srx.index, od_sdo_srx.sub, (uint32_t[]) {0x580 + (id)}, sizeof(uint32_t)); \
    co_set_field((co), (OD), od_sdo_srv_id.index, od_sdo_srv_id.sub, (uint8_t[]) {(id)}, sizeof(uint8_t));

#define RESET_SDO_COMMUNICATION(co, OD)                                                                   \
    co_set_field((co), (OD), od_sdo_stx.index, od_sdo_stx.sub, (uint32_t[]) {0}, sizeof(uint32_t)); \
    co_set_field((co), (OD), od_sdo_srx.index, od_sdo_srx.sub, (uint32_t[]) {0}, sizeof(uint32_t)); \
    co_set_field((co), (OD), od_sdo_srv_id.index, od_sdo_srv_id.sub, (uint8_t[]) {0}, sizeof(uint8_t));

/* TxPDO fields. Add NPDO to index to get RxPDO(N) */
extern const ODFieldType od_rx_pdo_cfg;
extern const ODFieldType od_rx_pdo_id;
extern const ODFieldType od_rx_pdo_type;
extern const ODFieldType od_rx_pdo_inhubit;
extern const ODFieldType od_rx_pdo_res;
extern const ODFieldType od_rx_pdo_event_timer;

extern const ODFieldType od_rx_pdo_map[9];

/* TxPDO fields. Add NPDO to index to get TxPDO(N) */
extern const ODFieldType od_tx_pdo_cfg;
extern const ODFieldType od_tx_pdo_id;
extern const ODFieldType od_tx_pdo_type;
extern const ODFieldType od_tx_pdo_inhubit;
extern const ODFieldType od_tx_pdo_res;
extern const ODFieldType od_tx_pdo_event_timer;

extern const ODFieldType od_tx_pdo_map[9];
