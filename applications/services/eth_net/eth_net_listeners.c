#include "eth_net_listeners.h"
#include "rhs.h"
#include <string.h>
#include <stdlib.h>

void eth_net_listeners_add(EthNetListener** listeners, EthNetListenerType type, const char* uri, mg_event_handler_t fn, void* context)
{
    rhs_assert(listeners);
    rhs_assert(uri);

    // Allocate new listener
    EthNetListener* listener = malloc(sizeof(EthNetListener));
    rhs_assert(listener);

    // Allocate and copy URI string
    listener->uri = malloc(strlen(uri) + 1);
    rhs_assert(listener->uri);
    strcpy(listener->uri, uri);

    // Set listener properties
    listener->type    = type;
    listener->fn      = fn;
    listener->context = context;
    listener->next    = NULL;

    // Add to the end of the list
    if (*listeners == NULL)
    {
        *listeners = listener;
    }
    else
    {
        EthNetListener* current = *listeners;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = listener;
    }

    MG_INFO(("Listener added: type=%d, uri=%s", type, uri));
}

void eth_net_listeners_restore(EthNetListener* listeners, struct mg_mgr* mgr)
{
    rhs_assert(mgr);

    if (listeners == NULL)
    {
        MG_INFO(("No listeners to restore"));
        return;
    }

    MG_INFO(("Restoring listeners..."));

    EthNetListener* listener = listeners;
    while (listener != NULL)
    {
        if (listener->type == EthNetListenerTypeHttp)
        {
            mg_http_listen(mgr, listener->uri, listener->fn, listener->context);
            MG_INFO(("Restored HTTP server on %s", listener->uri));
        }
        else if (listener->type == EthNetListenerTypeTcp)
        {
            mg_listen(mgr, listener->uri, listener->fn, listener->context);
            MG_INFO(("Restored TCP server on %s", listener->uri));
        }

        listener = listener->next;
    }
}

void eth_net_listeners_free(EthNetListener** listeners)
{
    rhs_assert(listeners);

    EthNetListener* listener = *listeners;

    while (listener != NULL)
    {
        EthNetListener* next = listener->next;
        free(listener->uri);
        free(listener);
        listener = next;
    }

    *listeners = NULL;
    MG_INFO(("All listeners freed"));
}

size_t eth_net_listeners_count(EthNetListener* listeners)
{
    size_t          count    = 0;
    EthNetListener* listener = listeners;

    while (listener != NULL)
    {
        count++;
        listener = listener->next;
    }

    return count;
}
