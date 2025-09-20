#pragma once

#include "rhs.h"
#include "rhs_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "mongoose.h"
#include "rpc.h"
#include "hal.h"

typedef struct
{
    char*              uri;
    mg_event_handler_t fn;
    void*              context;
} EthNetApiEventDataInterface;

typedef union
{
    EthNetApiEventDataInterface interface;
} EthNetApiEventData;

typedef enum
{
    EthNetApiEventTypeSetHttp = 0,
    EthNetApiEventTypeSetTcp  = 1,
} EthNetApiEventType;

typedef struct
{
    RHSApiLock         lock;
    EthNetApiEventType type;
    EthNetApiEventData data;
} EthNetApiEventMessage;

struct EthNet
{
    struct mg_mgr    mgr;  // Initialise Mongoose event manager
    RHSMessageQueue* queue;
};