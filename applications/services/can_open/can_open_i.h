#pragma once
#include "rhs.h"
#include "rhs_hal.h"
#include "data.h"
#include "timer.h"
#include "timers_driver.h"
#include "cli.h"

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
    SDOCallback   sdo_callback;
};

typedef struct FilterId
{
    uint16_t         id;
    struct FilterId* next;
} FilterId;

/* co_stack and rtimer use callbacks without context, so the instance is global (can_open_stack.c) */
extern CanOpenApp* can_open_app;

/* CAN IDs printed by the "can_open" CLI command (can_open_cli.c) */
extern FilterId* filter_list;

int canSend(CAN_PORT port, Message const* m);

void can_open_cli(char* args, void* context);
