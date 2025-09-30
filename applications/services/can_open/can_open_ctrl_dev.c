#include "can_open.h"
#include "can_open_srv.h"

/********************** GET FIELD FROM OD **********************/

/* Rx and Tx PDO */
static ODFieldType sdo_set_pdo(PDOField pdo_field, NumPDOType num_pdo, bool is_tx)
{
    rhs_assert(num_pdo < NumPDOMax);
    ODFieldType        field;
    const ODFieldType* base_field;

    switch (pdo_field)
    {
    case PDOFieldConfig:
        rhs_crash("You cannot set group field in PDO");
    case PDOFieldId:
        base_field = is_tx ? &od_tx_pdo_id : &od_rx_pdo_id;
        field      = (ODFieldType) {base_field->index + num_pdo, base_field->sub, base_field->size};
        break;
    case PDOFieldType:
        base_field = is_tx ? &od_tx_pdo_type : &od_rx_pdo_type;
        field      = (ODFieldType) {base_field->index + num_pdo, base_field->sub, base_field->size};
        break;
    case PDOFieldInhibit:
        base_field = is_tx ? &od_tx_pdo_inhubit : &od_rx_pdo_inhubit;
        field      = (ODFieldType) {base_field->index + num_pdo, base_field->sub, base_field->size};
        break;
    case PDOFieldRes:
        base_field = is_tx ? &od_tx_pdo_res : &od_rx_pdo_res;
        field      = (ODFieldType) {base_field->index + num_pdo, base_field->sub, base_field->size};
        break;
    case PDOFieldEventTime:
        base_field = is_tx ? &od_tx_pdo_event_timer : &od_rx_pdo_event_timer;
        field      = (ODFieldType) {base_field->index + num_pdo, base_field->sub, base_field->size};
        break;
    default:
        rhs_crash("You set an unknown field for PDO");
    }
    return field;
}

int sdo_set_rx_pdo(CanOpenApp* app, CO_Data* d, PDOField pdo_field, NumPDOType pdo, uint8_t node_id, void* data)
{
    ODFieldType field = sdo_set_pdo(pdo_field, pdo, false);
    return can_open_write_sdo(app, d, node_id, field.index, field.sub, field.size, data, NULL);
}

int sdo_set_tx_pdo(CanOpenApp* app, CO_Data* d, PDOField pdo_field, NumPDOType pdo, uint8_t node_id, void* data)
{
    ODFieldType field = sdo_set_pdo(pdo_field, pdo, true);
    return can_open_write_sdo(app, d, node_id, field.index, field.sub, field.size, data, NULL);
}

/********************** MAPPING **********************/

#define PDO_TO_MAP(index, subindex, size) (((index) << 16) | ((subindex) << 8) | ((size) << 3));

static ODFieldType sdo_set_pdo_map_generic(PDOMapType num_map, bool is_tx)
{
    const ODFieldType* base_field = is_tx ? od_tx_pdo_map : od_rx_pdo_map;
    ODFieldType field = (ODFieldType) {base_field[num_map].index, base_field[num_map].sub, base_field[num_map].size};

    return field;
}

int sdo_set_rx_pdo_map(CanOpenApp* app,
                       CO_Data*    d,
                       uint8_t     node_id,
                       NumPDOType  num_pdo,
                       PDOMapType  num_map,
                       uint16_t    index,
                       uint8_t     subindex,
                       uint8_t     size)
{
    rhs_assert(PDOMapNum < num_map && num_map < PDOMapMax);
    uint32_t    map   = PDO_TO_MAP(index, subindex, size);
    ODFieldType field = sdo_set_pdo_map_generic(num_map, false);
    return can_open_write_sdo(app, d, node_id, field.index + num_pdo, field.sub, field.size, &map, NULL);
}

int sdo_set_tx_pdo_map(CanOpenApp* app,
                       CO_Data*    d,
                       uint8_t     node_id,
                       NumPDOType  num_pdo,
                       PDOMapType  num_map,
                       uint16_t    index,
                       uint8_t     subindex,
                       uint8_t     size)
{
    rhs_assert(PDOMapNum < num_map && num_map < PDOMapMax);
    uint32_t    map   = PDO_TO_MAP(index, subindex, size);
    ODFieldType field = sdo_set_pdo_map_generic(num_map, true);
    return can_open_write_sdo(app, d, node_id, field.index + num_pdo, field.sub, field.size, &map, NULL);
}

static ODFieldType sdo_set_pdo_map_group(NumPDOType num_pdo, bool is_tx)
{
    const ODFieldType* base_field = is_tx ? od_tx_pdo_map : od_rx_pdo_map;
    ODFieldType        field =
        (ODFieldType) {base_field[PDOMapNum].index + num_pdo, base_field[PDOMapNum].sub, base_field[PDOMapNum].size};

    return field;
}

int sdo_set_rx_pdo_map_group(CanOpenApp* app, CO_Data* d, uint8_t node_id, NumPDOType num_pdo, PDOMapType num_map)
{
    rhs_assert(num_pdo < NumPDOMax);
    rhs_assert(num_map < PDOMapMax);
    ODFieldType field = sdo_set_pdo_map_group(num_pdo, false);
    return can_open_write_sdo(app, d, node_id, field.index, field.sub, field.size, &num_map, NULL);
}

int sdo_set_tx_pdo_map_group(CanOpenApp* app, CO_Data* d, uint8_t node_id, NumPDOType num_pdo, PDOMapType num_map)
{
    rhs_assert(num_pdo < NumPDOMax);
    rhs_assert(num_map < PDOMapMax);
    ODFieldType field = sdo_set_pdo_map_group(num_pdo, true);
    return can_open_write_sdo(app, d, node_id, field.index, field.sub, field.size, &num_map, NULL);
}

/********************** GET FIELD FROM OD **********************/
#define OD_GET_FIELD(type, func_name)                                                                                 \
    type func_name(CanOpenApp* app, CO_Data* d, uint16_t index, uint8_t subIndex)                                     \
    {                                                                                                                 \
        type data;                                                                                                    \
        getODentry(d, index, subIndex, (void*) &data, (uint32_t[]) {SDO_MAX_LENGTH_TRANSFER}, (uint8_t[]) {0x00}, 1); \
        return data;                                                                                                  \
    }

OD_GET_FIELD(uint8_t, od_get_field_uns8)
OD_GET_FIELD(int8_t, od_get_field_s8)
OD_GET_FIELD(uint16_t, od_get_field_uns16)
OD_GET_FIELD(int16_t, od_get_field_s16)
OD_GET_FIELD(uint32_t, od_get_field_uns32)
OD_GET_FIELD(int32_t, od_get_field_s32)
OD_GET_FIELD(float, od_get_field_float)
