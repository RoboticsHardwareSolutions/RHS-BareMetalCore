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
 * @brief Restore all listeners by calling their registration functions
 * @param listeners Head of the list
 * @param mgr Mongoose manager to register listeners with
 */
void eth_net_listeners_restore(EthNetListener* listeners, struct mg_mgr* mgr);

/**
 * @brief Free all listeners and their allocated memory
 * @param listeners Pointer to the head of the list
 */
void eth_net_listeners_free(EthNetListener** listeners);

/**
 * @brief Get the count of listeners in the list
 * @param listeners Head of the list
 * @return Number of listeners
 */
size_t eth_net_listeners_count(EthNetListener* listeners);
