#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "rhs.h"
#include "rhs_hal.h"
#include "cli.h"
#include "rhs_hal_flash_ex.h"
#include "runit.h"

#define TAG "flash_ex_test"

/* memory area for tests */
#define FLASH_COUNT_BLOCKS 256
#define FLASH_DATA_SIZE 256
#define FLASH_START_ADDRESS 0x400000

static int erase_all(void)
{
    return rhs_hal_flash_ex_block_erase(FLASH_START_ADDRESS, FLASH_DATA_SIZE * FLASH_COUNT_BLOCKS);
}

static int check_erased_data(void)
{
    uint8_t data[FLASH_DATA_SIZE] = {0};
    uint8_t data_pulled[FLASH_DATA_SIZE];

    for (int i = 0; i < FLASH_COUNT_BLOCKS; i++)
    {
        if (rhs_hal_flash_ex_read(FLASH_START_ADDRESS + i * FLASH_COUNT_BLOCKS, data_pulled, sizeof(data_pulled)) != 0)
        {
            return -1;
        }
        for (int j = 0; j < sizeof(data_pulled); j++)
        {
            if (data_pulled[j] != 0xFF)
            {
                return -1;
            }
        }
    }
    return 0;
}

static int random_test(void)
{
    uint8_t data[FLASH_DATA_SIZE] = {0};
    uint8_t data_pulled[FLASH_DATA_SIZE];

    for (int i = 0; i < FLASH_COUNT_BLOCKS; i++)
    {
        rhs_hal_random_fill_buf(data, sizeof(data));

        if (rhs_hal_flash_ex_write(FLASH_START_ADDRESS + i * FLASH_COUNT_BLOCKS, data, sizeof(data)) != 0)
        {
            return -1;
        }
        if (rhs_hal_flash_ex_read(FLASH_START_ADDRESS + i * FLASH_COUNT_BLOCKS, data_pulled, sizeof(data_pulled)) != 0)
        {
            return -1;
        }

        for (int j = 0; j < sizeof(data); j++)
        {
            if (data_pulled[j] != data[j])
            {
                return -1;
            }
        }
    }
    return 0;
}

void flash_ex_test(char* args, void* context)
{
    runit_counter_assert_passes   = 0;
    runit_counter_assert_failures = 0;

    runit_assert(erase_all() == 0);
    runit_assert(check_erased_data() == 0);
    runit_assert(random_test() == 0);

    runit_report();
}

void flash_ex_test_start_up(void)
{
    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "flash_ex_test", flash_ex_test, NULL);
    rhs_record_close(RECORD_CLI);
}
