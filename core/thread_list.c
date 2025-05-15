#include "thread_list.h"

struct RHSThreadList
{
    RHSThreadListItem* item;
    uint32_t           runtime_previous;
    uint32_t           runtime_current;
    uint32_t           isr_previous;
    uint32_t           isr_current;
};

// Create a new thread list
RHSThreadList* rhs_thread_list_create(void)
{
    RHSThreadList* list = (RHSThreadList*) malloc(sizeof(RHSThreadList));
    if (list)
    {
        list->item             = NULL;
        list->runtime_previous = 0;
        list->runtime_current  = 0;
        list->isr_previous     = 0;
        list->isr_current      = 0;
    }
    return list;
}

void rhs_thread_list_erase(RHSThreadList* list)
{
    rhs_assert(list);
    RHSThreadListItem* item = list->item;
    while (item)
    {
        RHSThreadListItem* next = item->next;
        free(item);
        item = next;
    }
    list->item = NULL;
}

// Destroy the thread list and free all item
void rhs_thread_list_destroy(RHSThreadList* list)
{
    rhs_assert(list);
    rhs_thread_list_erase(list);
    free(list);
}

// Find an item by thread pointer
RHSThreadListItem* rhs_thread_list_find(RHSThreadList* list, RHSThreadListItem* item)
{
    rhs_assert(list);
    rhs_assert(item);
    RHSThreadListItem* current = list->item;
    while (current)
    {
        if (current == item)
            return current;
        current = current->next;
    }
    return NULL;
}

// Add a new item to the thread list
RHSThreadListItem* rhs_thread_list_add(RHSThreadList* list)
{  
    rhs_assert(list);
    RHSThreadListItem* item = (RHSThreadListItem*) malloc(sizeof(RHSThreadListItem));
    if (item)
    {
        item->next = list->item;
        list->item = item;
    }
    return item;
}   

// Remove an item by thread pointer
int rhs_thread_list_remove(RHSThreadList* list, RHSThreadListItem* item)
{
    rhs_assert(list);
    rhs_assert(item);
    RHSThreadListItem* current = list->item;
    RHSThreadListItem* prev    = NULL;
    while (current)
    {
        if (current == item)
        {
            if (prev)
                prev->next = current->next;
            else
                list->item = current->next;
            free(current);
            return 1;
        }
        prev    = current;
        current = current->next;
    }
    return 0;
}

RHSThreadListItem* rhs_thread_list_at(RHSThreadList* list, size_t index)
{
    rhs_assert(list);
    RHSThreadListItem* item = list->item;
    size_t i = 0;
    while (item)
    {
        if (i == index)
            return item;
        item = item->next;
        i++;
    }
    return NULL;
}

// Get the number of item in the thread list
size_t rhs_thread_list_size(RHSThreadList* list)
{
    rhs_assert(list);
    size_t             count = 0;
    RHSThreadListItem* item  = list->item;
    while (item)
    {
        count++;
        item = item->next;
    }
    return count;
}