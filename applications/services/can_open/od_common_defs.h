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
} NumPDOType;

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
} PDOMapType;

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
    uint16_t index;
    uint8_t  subindex;
    uint8_t  size;
} ODFieldType;

extern const ODFieldType od_device_type;
extern const ODFieldType od_device_name;
extern const ODFieldType od_producer_hb;
extern const ODFieldType od_device_vendor;
extern const ODFieldType od_device_code;
extern const ODFieldType od_device_revision;
extern const ODFieldType od_device_serial;

/* TxPDO fields. Add NumPDOType to index to get RxPDO(N) */
extern const ODFieldType od_rx_pdo_cfg;
extern const ODFieldType od_rx_pdo_id;
extern const ODFieldType od_rx_pdo_type;
extern const ODFieldType od_rx_pdo_inhubit;
extern const ODFieldType od_rx_pdo_res;
extern const ODFieldType od_rx_pdo_event_timer;

extern const ODFieldType od_rx_pdo_map[9];

/* TxPDO fields. Add NumPDOType to index to get TxPDO(N) */
extern const ODFieldType od_tx_pdo_cfg;
extern const ODFieldType od_tx_pdo_id;
extern const ODFieldType od_tx_pdo_type;
extern const ODFieldType od_tx_pdo_inhubit;
extern const ODFieldType od_tx_pdo_res;
extern const ODFieldType od_tx_pdo_event_timer;

extern const ODFieldType od_tx_pdo_map[9];
