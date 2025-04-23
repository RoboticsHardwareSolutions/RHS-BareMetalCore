#pragma once
#include "rhs.h"
#include "rhs_hal.h"
#include "data.h"
#include "timer.h"
#include "timers_driver.h"
#include "cli.h"

#define TAG "CanOpen App"

#define EVENT_FLAG_SDO (1 << 0)

#define MAX_OD 3  // FIXME one day...

typedef enum
{
    CanOpenAppEventTypeRX  = (1 << 0),
    CanOpenAppEventTypeTX  = (1 << 1),
    CanOpenAppEventTypePDO = (1 << 2),
} CanOpenAppEventType;

typedef struct
{
    CO_Data*    od;
    RHSHalCANId can_id;
    Message     data;
} CanOpenAppMessage;

struct CanOpenApp
{
    RHSEventFlag* srv_event;

    uint8_t counter_od;
    struct
    {
        CO_Data*    od;
        RHSHalCANId can_id;
    } handler[MAX_OD];

    RHSMessageQueue* rx_queue;
    RHSMessageQueue* tx_queue;

    RHSMutex*     sdo_mutex;
    RHSEventFlag* sdo_event;
    sdo_cb        sdo_callback;
};

typedef struct FilterId
{
    uint16_t         id;
    struct FilterId* next;
} FilterId;
