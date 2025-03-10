#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "rhs.h"
#include "cli.h"

#define TAG "test_mem"

void test_rhs_memmgr(char* args, void* context)
{
    void* ptr;

    // allocate memory case
    ptr = malloc(100);
    if (ptr == NULL)
    {
        RHS_LOG_D(TAG, "fault\r\n");
        return;
    }
    // test that memory is zero-initialized after allocation
    for (int i = 0; i < 100; i++)
    {
        if (0 != ((uint8_t*) ptr)[i])
        {
        RHS_LOG_D(TAG, "fault\r\n");
        return;
        }
    }
    free(ptr);

    // reallocate memory case
    ptr = malloc(100);
    memset(ptr, 66, 100);
    ptr = realloc(ptr, 200);
    if (ptr == NULL)
    {
        RHS_LOG_D(TAG, "fault\r\n");
        return;
    }

    // test that memory is really reallocated
    for (int i = 0; i < 100; i++)
    {
        if (66 != ((uint8_t*) ptr)[i])
        {
        RHS_LOG_D(TAG, "fault\r\n");
        return;
        }
    }

    free(ptr);

    // allocate and zero-initialize array (calloc)
    ptr = calloc(100, 2);
    if (ptr == NULL)
    {
        RHS_LOG_D(TAG, "fault\r\n");
        return;
    }
    for (int i = 0; i < 100 * 2; i++)
    {
        if (0 != ((uint8_t*) ptr)[i])
        {
        RHS_LOG_D(TAG, "fault\r\n");
        return;
        }
    }
    free(ptr);
    RHS_LOG_D(TAG, "test mem OK\r\n");
}

void test_rhs_memmgr_start_up(void)
{
    cli_add_command("test_mem", test_rhs_memmgr, NULL);
}
