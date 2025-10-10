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

bool eth_net_listeners_remove(EthNetListener** listeners, const char* uri)
{
    rhs_assert(listeners);
    rhs_assert(uri);

    if (*listeners == NULL)
    {
        return false;
    }

    EthNetListener* current  = *listeners;
    EthNetListener* previous = NULL;

    while (current != NULL)
    {
        if (strcmp(current->uri, uri) == 0)
        {
            // Found the listener to remove
            if (previous == NULL)
            {
                // Removing the first element
                *listeners = current->next;
            }
            else
            {
                // Removing a middle or last element
                previous->next = current->next;
            }

            MG_INFO(("Listener removed: type=%d, uri=%s", current->type, current->uri));
            
            // Free the memory
            free(current->uri);
            free(current);
            
            return true;
        }

        previous = current;
        current  = current->next;
    }

    return false;
}
