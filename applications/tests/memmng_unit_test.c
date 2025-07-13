#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "rhs.h"
#include "cli.h"
#include "runit.h"

#define TAG "mem_test"

void rhs_memmgr_test(char* args, void* context)
{
    void* ptr;
    runit_counter_assert_passes = 0;
    runit_counter_assert_failures = 0;

    // allocate memory case
    ptr = malloc(100);
    runit_assert(ptr != NULL);
    
    // test that memory is zero-initialized after allocation
    for (int i = 0; i < 100; i++)
    {
        runit_assert(((uint8_t*) ptr)[i] == 0);
    }
    free(ptr);

    // reallocate memory case
    ptr = malloc(100);
    memset(ptr, 66, 100);
    ptr = realloc(ptr, 200);
    runit_assert(ptr != NULL);

    // test that memory is really reallocated
    for (int i = 0; i < 100; i++)
    {
        runit_assert(((uint8_t*) ptr)[i] == 66);
    }

    free(ptr);

    // allocate and zero-initialize array (calloc)
    ptr = calloc(100, 2);
    runit_assert(ptr != NULL);
    for (int i = 0; i < 100 * 2; i++)
    {
        runit_assert(((uint8_t*) ptr)[i] == 0);
    }
    free(ptr);
    RHS_LOG_D(TAG, "test mem OK");

    runit_report();
}

void rhs_memmgr_test_start_up(void)
{
    Cli *cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "mem_test", rhs_memmgr_test, NULL);
    rhs_record_close(RECORD_CLI);
}
