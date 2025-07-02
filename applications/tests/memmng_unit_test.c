#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "rhs.h"
#include "cli.h"

#define TAG "mem_test"

void rhs_memmgr_test(char* args, void* context)
{
    void* ptr;

    // allocate memory case
    ptr = malloc(100);
    if (ptr == NULL)
    {
        RHS_LOG_D(TAG, "fault");
        return;
    }
    // test that memory is zero-initialized after allocation
    for (int i = 0; i < 100; i++)
    {
        if (0 != ((uint8_t*) ptr)[i])
        {
        RHS_LOG_D(TAG, "fault");
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
        RHS_LOG_D(TAG, "fault");
        return;
    }

    // test that memory is really reallocated
    for (int i = 0; i < 100; i++)
    {
        if (66 != ((uint8_t*) ptr)[i])
        {
        RHS_LOG_D(TAG, "fault");
        return;
        }
    }

    free(ptr);

    // allocate and zero-initialize array (calloc)
    ptr = calloc(100, 2);
    if (ptr == NULL)
    {
        RHS_LOG_D(TAG, "fault");
        return;
    }
    for (int i = 0; i < 100 * 2; i++)
    {
        if (0 != ((uint8_t*) ptr)[i])
        {
        RHS_LOG_D(TAG, "fault");
        return;
        }
    }
    free(ptr);
    RHS_LOG_D(TAG, "test mem OK");
}

void rhs_memmgr_test_start_up(void)
{
    Cli *cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "mem_test", rhs_memmgr_test, NULL);
    rhs_record_close(RECORD_CLI);
}
