#pragma once

#include "mongoose.h"

typedef enum
{
    NetListenerTypeHttp = 0,
    NetListenerTypeTcp  = 1,
} NetListenerType;

typedef struct NetListener
{
    NetListenerType     type;
    char*               uri;
    mg_event_handler_t  fn;
    void*               context;
    struct NetListener* next;
} NetListener;

/**
 * @brief Add a new listener to the list
 * @param listeners Pointer to the head of the list
 * @param type Type of listener (HTTP or TCP)
 * @param uri URI string (will be copied)
 * @param fn Event handler function
 * @param context User context pointer
 */
void net_listeners_add(NetListener**      listeners,
                       NetListenerType    type,
                       const char*        uri,
                       mg_event_handler_t fn,
                       void*              context);

/**
 * @brief Remove a listener from the list by URI
 * @param listeners Pointer to the head of the list
 * @param uri URI string to match
 * @return true if listener was found and removed, false otherwise
 */
bool net_listeners_remove(NetListener** listeners, const char* uri);
