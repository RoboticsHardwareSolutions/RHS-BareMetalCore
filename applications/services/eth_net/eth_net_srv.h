#pragma once

#include "rhs.h"
#include "rhs_hal.h"
#include "cli.h"
#include "FreeRTOS.h"
#include "task.h"
#include "mongoose.h"
#include "eth_net_listeners.h"

typedef struct
{
    char*              uri;
    mg_event_handler_t fn;
    void*              context;
} EthNetApiEventDataInterface;

typedef union
{
    EthNetApiEventDataInterface interface;
    EthNetConfig                config;
} EthNetApiEventData;

typedef enum
{
    EthNetApiEventTypeSetHttp = 0,
    EthNetApiEventTypeSetTcp  = 1,
    EthNetApiEventTypeRstTcp  = 2,
    EthNetApiEventTypeRestart = 7,
} EthNetApiEventType;

typedef struct
{
    RHSApiLock         lock;
    EthNetApiEventType type;
    EthNetApiEventData data;
} EthNetApiEventMessage;

struct EthNet
{
    struct mg_mgr*      mgr;          // Initialise Mongoose event manager
    struct mg_tcpip_if* ifp;          // Builtin TCP/IP stack only. Interface pointer
    void*               driver_data;  // Driver-specific data
    EthNetConfig        config;
    Cli*                cli;
    RHSThread*          thread;
    RHSMessageQueue*    queue;
    EthNetListener*     listeners;    // Linked list of registered listeners
};