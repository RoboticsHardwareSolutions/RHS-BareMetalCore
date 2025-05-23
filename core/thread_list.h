#pragma once
#include "rhs.h"

typedef struct RHSThreadListItem
{
    RHSThread*        thread;         /**< Pointer to RHSThread, valid while it is running */
    const char*       name;           /**< Thread name, valid while it is running */
    RHSThreadPriority priority;       /**< Thread priority */
    uint32_t          stack_address;  /**< Thread stack address */
    uint32_t          stack_size;     /**< Thread stack size */
    uint32_t          stack_min_free; /**< Thread minimum of the stack size ever reached */
    const char* state; /**< Thread state, can be: "Running", "Ready", "Blocked", "Suspended", "Deleted", "Invalid" */

    uint32_t counter_previous; /**< Thread previous runtime counter */
    uint32_t counter_current;  /**< Thread current runtime counter */
    uint32_t cpu;              /**< Thread CPU usage time in percents (including interrupts happened while running) */
    uint32_t tick;             /**< Thread last seen tick */

    struct RHSThreadListItem* next;
} RHSThreadListItem;

RHSThreadList* rhs_thread_list_create(void);

void rhs_thread_list_erase(RHSThreadList* list);

void rhs_thread_list_destroy(RHSThreadList* list);

RHSThreadListItem* rhs_thread_list_find(RHSThreadList* list, RHSThreadListItem* item);

RHSThreadListItem* rhs_thread_list_add(RHSThreadList* list);

RHSThreadListItem* rhs_thread_list_at(RHSThreadList* list, uint16_t index);

int rhs_thread_list_remove(RHSThreadList* list, RHSThreadListItem* item);

uint16_t rhs_thread_list_size(RHSThreadList* list);

void rhs_thread_list_process(RHSThreadList* instance, uint32_t runtime, uint32_t tick);
