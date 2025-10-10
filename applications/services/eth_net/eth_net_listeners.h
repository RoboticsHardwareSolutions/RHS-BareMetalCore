#pragma once

#include "mongoose.h"

typedef enum
{
    EthNetListenerTypeHttp = 0,
    EthNetListenerTypeTcp  = 1,
} EthNetListenerType;

typedef struct EthNetListener
{
    EthNetListenerType     type;
    char*                  uri;
    mg_event_handler_t     fn;
    void*                  context;
    struct EthNetListener* next;
} EthNetListener;

/**
 * @brief Add a new listener to the list
 * @param listeners Pointer to the head of the list
 * @param type Type of listener (HTTP or TCP)
 * @param uri URI string (will be copied)
 * @param fn Event handler function
 * @param context User context pointer
 */
void eth_net_listeners_add(EthNetListener** listeners, EthNetListenerType type, const char* uri, mg_event_handler_t fn, void* context);

/**
 * @brief Remove a listener from the list by URI
 * @param listeners Pointer to the head of the list
 * @param uri URI string to match
 * @return true if listener was found and removed, false otherwise
 */
bool eth_net_listeners_remove(EthNetListener** listeners, const char* uri);
