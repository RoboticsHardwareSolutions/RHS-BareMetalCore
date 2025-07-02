#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "rhs.h"
#include "rhs_hal.h"
#include "cli.h"
#include "rhs_hal_flash_ex.h"

#define TAG "flash_ex_test"

#define FLASH_COUNT_BLOCKS 256
#define FLASH_DATA_SIZE 256
#define FLASH_START_ADDRESS 0x400000

void flash_ex_test(char* args, void* context)
{
    uint8_t data[FLASH_DATA_SIZE] = {0};
    uint8_t data_pulled[FLASH_DATA_SIZE];

    if (rhs_hal_flash_ex_block_erase(FLASH_START_ADDRESS, FLASH_DATA_SIZE * FLASH_COUNT_BLOCKS) != 0)
    {
        RHS_LOG_D(TAG, "Flash ERASE fault!!!");
        return;
    }

    for (int i = 0; i < FLASH_COUNT_BLOCKS; i++)
    {
        if (rhs_hal_flash_ex_read(FLASH_START_ADDRESS + i * FLASH_COUNT_BLOCKS, data_pulled, sizeof(data_pulled)) != 0)
        {
            RHS_LOG_D(TAG, "Flash READ fault!!!");
            return;
        }
        for (int j = 0; j < sizeof(data_pulled); j++)
        {
            if (data_pulled[j] != 0xFF)
            {
                RHS_LOG_D(TAG, "Not erased data at index %d of record %d", j, i);
                return;
            }
        }
    }

    RHS_LOG_D(TAG, "Flash ERASE test completed successfully.");

    for (int i = 0; i < FLASH_COUNT_BLOCKS; i++)
    {
        rhs_hal_random_fill_buf(data, sizeof(data));

        if (rhs_hal_flash_ex_write(FLASH_START_ADDRESS + i * FLASH_COUNT_BLOCKS, data, sizeof(data)) != 0)
        {
            RHS_LOG_D(TAG, "Flash WRITE fault!!!");
            return;
        }
        if (rhs_hal_flash_ex_read(FLASH_START_ADDRESS + i * FLASH_COUNT_BLOCKS, data_pulled, sizeof(data_pulled)) != 0)
        {
            RHS_LOG_D(TAG, "Flash READ fault!!!");
            return;
        }

        for (int j = 0; j < sizeof(data); j++)
        {
            if (data_pulled[j] != data[j])
            {
                RHS_LOG_D(TAG, "Data mismatch at index %d of record %d", j, i);
                return;
            }
        }
        RHS_LOG_D(TAG, "Flash WRITE test %d OK", i);
    }

    RHS_LOG_D(TAG, "Flash WRITE test completed successfully.");
}

void flash_ex_test_start_up(void)
{
    Cli *cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "flash_ex_test", flash_ex_test, NULL);
    rhs_record_close(RECORD_CLI);
}
