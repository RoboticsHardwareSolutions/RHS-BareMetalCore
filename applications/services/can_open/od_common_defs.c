#include "od_common_defs.h"

const ODFieldType od_device_type     = {.index = 0x1000, .subindex = 0x00, .size = sizeof(uint32_t)};
const ODFieldType od_device_name     = {.index = 0x1008, .subindex = 0x00, .size = 32};
const ODFieldType od_producer_hb     = {.index = 0x1017, .subindex = 0x00, .size = sizeof(uint16_t)};
const ODFieldType od_device_vendor   = {.index = 0x1018, .subindex = 0x01, .size = sizeof(uint32_t)};
const ODFieldType od_device_code     = {.index = 0x1018, .subindex = 0x02, .size = sizeof(uint32_t)};
const ODFieldType od_device_revision = {.index = 0x1018, .subindex = 0x03, .size = sizeof(uint32_t)};
const ODFieldType od_device_serial   = {.index = 0x1018, .subindex = 0x04, .size = sizeof(uint32_t)};

/* TxPDO fields. Add NumPDOType to index to get RxPDO(N) */
const ODFieldType od_rx_pdo_cfg         = {.index = 0x1400, .subindex = 0x00, .size = sizeof(uint8_t)};
const ODFieldType od_rx_pdo_id          = {.index = 0x1400, .subindex = 0x01, .size = sizeof(uint32_t)};
const ODFieldType od_rx_pdo_type        = {.index = 0x1400, .subindex = 0x02, .size = sizeof(uint8_t)};
const ODFieldType od_rx_pdo_inhubit     = {.index = 0x1400, .subindex = 0x03, .size = sizeof(uint16_t)};
const ODFieldType od_rx_pdo_res         = {.index = 0x1400, .subindex = 0x04, .size = sizeof(uint8_t)};
const ODFieldType od_rx_pdo_event_timer = {.index = 0x1400, .subindex = 0x05, .size = sizeof(uint16_t)};

const ODFieldType od_rx_pdo_map[9] = {
    {.index = 0x1600, .subindex = 0x00, .size = sizeof(uint8_t)},
    {.index = 0x1600, .subindex = 0x01, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x02, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x03, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x04, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x05, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x06, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x07, .size = sizeof(uint32_t)},
    {.index = 0x1600, .subindex = 0x08, .size = sizeof(uint32_t)},
};

/* TxPDO fields. Add NumPDOType to index to get TxPDO(N) */
const ODFieldType od_tx_pdo_cfg         = {.index = 0x1800, .subindex = 0x00, .size = sizeof(uint8_t)};
const ODFieldType od_tx_pdo_id          = {.index = 0x1800, .subindex = 0x01, .size = sizeof(uint32_t)};
const ODFieldType od_tx_pdo_type        = {.index = 0x1800, .subindex = 0x02, .size = sizeof(uint8_t)};
const ODFieldType od_tx_pdo_inhubit     = {.index = 0x1800, .subindex = 0x03, .size = sizeof(uint16_t)};
const ODFieldType od_tx_pdo_res         = {.index = 0x1800, .subindex = 0x04, .size = sizeof(uint8_t)};
const ODFieldType od_tx_pdo_event_timer = {.index = 0x1800, .subindex = 0x05, .size = sizeof(uint16_t)};

const ODFieldType od_tx_pdo_map[9] = {
    {.index = 0x1A00, .subindex = 0x00, .size = sizeof(uint8_t)},
    {.index = 0x1A00, .subindex = 0x01, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x02, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x03, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x04, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x05, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x06, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x07, .size = sizeof(uint32_t)},
    {.index = 0x1A00, .subindex = 0x08, .size = sizeof(uint32_t)},
};
