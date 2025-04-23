#include "can_open.h"
#include "can_open_srv.h"

FilterId* filter_list = NULL;

extern int canSend(CAN_PORT port, Message const* m);

static void add_filter_id(uint16_t id)
{
    FilterId* new_node = (FilterId*) malloc(sizeof(FilterId));
    if (new_node == NULL)
    {
        RHS_LOG_E(TAG, "Memory allocation failed");
        return;
    }
    new_node->id   = id;
    new_node->next = filter_list;
    filter_list    = new_node;
    RHS_LOG_I(TAG, "Added to view id 0x%03X", id);
}

static void remove_filter_id(uint16_t id)
{
    FilterId** current = &filter_list;
    while (*current != NULL)
    {
        if ((*current)->id == id)
        {
            FilterId* to_delete = *current;
            *current            = (*current)->next;
            free(to_delete);
            RHS_LOG_I(TAG, "Removed from view id 0x%03X", id);
            return;
        }
        current = &((*current)->next);
    }
    RHS_LOG_E(TAG, "ID 0x%03X not found in filter list", id);
}

static void clear_filter_list()
{
    while (filter_list != NULL)
    {
        FilterId* to_delete = filter_list;
        filter_list         = filter_list->next;
        free(to_delete);
    }
    RHS_LOG_I(TAG, "Cleared filter list");
}

static void list_filter_ids()
{
    RHS_LOG_I(TAG, "CAN ID list :");
    FilterId* current = filter_list;
    while (current != NULL)
    {
        RHS_LOG_I(TAG, "0x%03X", current->id);
        current = current->next;
    }
}

void can_open_cli(char* args, void* context)
{
    rhs_assert(context);
    CanOpenApp* app = (CanOpenApp*) context;
    if (args == NULL)
    {
        RHS_LOG_E(TAG, "Invalid argument");
        return;
    }

    char*    separator = strchr(args, ' ');
    uint32_t value     = 0;
    if (separator != NULL && *(separator + 1) != 0)
    {
        value = (uint32_t) strtoul(separator + 1, NULL, 16);
    }

    if (strstr(args, "-f") == args)
    {
        add_filter_id((uint16_t) value);
    }
    else if (strstr(args, "-e") == args)
    {
        remove_filter_id((uint16_t) value);
    }
    else if (strstr(args, "-c") == args)
    {
        clear_filter_list();
    }
    else if (strstr(args, "-l") == args)
    {
        list_filter_ids();
    }
    else if (strstr(args, "-s") == args)
    {
        uint8_t data[8]     = {0};  // Default data payload
        size_t  data_length = 0;

        if (separator != NULL && *(separator + 1) != 0)
        {
            char* data_str = strchr(separator + 1, ' ');
            if (data_str != NULL && *(data_str + 1) != 0)
            {
                data_str++;
                char* token = strtok(data_str, " ");
                while (token != NULL && data_length < sizeof(data))
                {
                    data[data_length++] = (uint8_t) strtoul(token, NULL, 16);
                    token               = strtok(NULL, " ");
                }
            }
        }

        CanOpenAppMessage msg = {.od          = app->handler[0].od,
                                 .data.cob_id = (uint16_t) value,
                                 .data.rtr    = 0,
                                 .data.len    = data_length};
        memcpy(msg.data.data, data, data_length);
        if (canSend(msg.od->canHandle, &msg.data) == 0)
        {
            RHS_LOG_I(TAG, "Sent CAN package with ID 0x%03X", msg.data.cob_id);
        }
        else
        {
            RHS_LOG_E(TAG, "Failed to send CAN package with ID 0x%03X", msg.data.cob_id);
        }
    }
    else if (strstr(args, "-m") == args)
    {
        RHS_LOG_I(TAG, "Usage:");
        RHS_LOG_I(TAG, "-f <id>: Add a CAN ID to the filter list");
        RHS_LOG_I(TAG, "-e <id>: Remove a CAN ID from the filter list");
        RHS_LOG_I(TAG, "-c: Clear the filter list");
        RHS_LOG_I(TAG, "-l: List all CAN IDs in the filter list");
        RHS_LOG_I(TAG, "-s <id>: Send a CAN package with the specified ID");
        RHS_LOG_I(TAG, "-m: Show this manual");
    }
    else
    {
        RHS_LOG_E(TAG, "Invalid argument");
    }
}
