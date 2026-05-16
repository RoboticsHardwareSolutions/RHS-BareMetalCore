#pragma once

#include "cli.h"
#include "net.h"

typedef struct NetListener NetListener;

typedef struct
{
    char*              uri;
    mg_event_handler_t fn;
    void*              context;
} NetApiEventDataInterface;

typedef union
{
    NetApiEventDataInterface interface;
    NetConfig                config;
} NetApiEventData;

typedef enum
{
    NetApiEventTypeSetHttp = 0,
    NetApiEventTypeSetTcp  = 1,
    NetApiEventTypeRstTcp  = 2,
    NetApiEventTypeRestart = 7,
} NetApiEventType;

typedef struct
{
    RHSApiLock      lock;
    NetApiEventType type;
    NetApiEventData data;
} NetApiEventMessage;

struct Net
{
    struct mg_mgr*   mgr;  // Initialise Mongoose event manager
    NetConfig*       config;
    NetListener*     listeners;  // Linked list of registered listeners
    RHSThread*       thread;
    RHSMessageQueue* queue;
    Cli*             cli;
};