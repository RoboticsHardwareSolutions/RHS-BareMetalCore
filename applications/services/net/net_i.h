#pragma once

#include "net.h"

typedef struct NetListener NetListener;

struct Net
{
    struct mg_mgr*   mgr;  // Initialise Mongoose event manager
    NetConfig*       config;
    NetListener*     listeners;  // Linked list of registered listeners
    RHSThread*       thread;
    RHSMessageQueue* queue;
};