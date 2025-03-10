#include "rhs_hal_flash_ex.h"
#include "rhs.h"
#include "mt25ql128aba.h"

static RHSMutex* rhs_hal_flash_ex_mutex = NULL;

int rhs_hal_flash_ex_init(void)
{
    mt25ql128aba_init(&hqspi);
    rhs_hal_flash_ex_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
    return 0;
}

int rhs_hal_flash_ex_read(uint32_t addr, uint8_t* p_data, uint32_t size)
{
    int error = 0;
    rhs_mutex_acquire(rhs_hal_flash_ex_mutex, RHSWaitForever);
    if (mt25ql128aba_read(&hqspi, MT25QL128ABA_QPI_MODE, p_data, addr, size) != 0)
    {
        error = -1;
    }
    rhs_mutex_release(rhs_hal_flash_ex_mutex);
    return error;
}

int rhs_hal_flash_ex_erase_chip(void)
{
    int error = 0;
    rhs_mutex_acquire(rhs_hal_flash_ex_mutex, RHSWaitForever);
    /* Check Flash busy ? */
    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        error = -1;
    } /* Enable write operations */
    else if (mt25ql128aba_write_enable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        error = -1;
    }
    else
    {
        /* Issue Chip erase command */
        if (mt25ql128aba_chip_erase(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
        {
            error = -1;
        }
    }

    rhs_mutex_release(rhs_hal_flash_ex_mutex);
    /* Return BSP status */
    return error;
}

int rhs_hal_flash_ex_write(uint32_t addr, uint8_t* p_data, uint32_t size)
{
    uint32_t end_addr, current_size, current_addr;
    uint8_t* write_data;
    int      error = 0;

    rhs_mutex_acquire(rhs_hal_flash_ex_mutex, RHSWaitForever);
    /* Calculation of the size between the write address and the end of the page */
    current_size = MT25QL128ABA_PAGE_SIZE - (addr % MT25QL128ABA_PAGE_SIZE);

    /* Check if the size of the data is less than the remaining place in the page */
    if (current_size > size)
    {
        current_size = size;
    }

    /* Initialize the address variables */
    current_addr = addr;
    end_addr     = addr + size;
    write_data   = p_data;

    do
    {
        if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
        {
            error = -1;
            break;
        }
        if (mt25ql128aba_write_enable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
        {
            error = -1;
            break;
        }
        if (mt25ql128aba_page_program(&hqspi, MT25QL128ABA_QPI_MODE, write_data, current_addr, current_size) != 0)
        {
            error = -1;
            break;
        }
        if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
        {
            error = -1;
            break;
        }

        /* Update the address and size variables for next page programming */
        current_addr += current_size;
        write_data += current_size;
        current_size =
            ((current_addr + MT25QL128ABA_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MT25QL128ABA_PAGE_SIZE;
    } while ((current_addr < end_addr));
    rhs_mutex_release(rhs_hal_flash_ex_mutex);
    return error;
}

int rhs_hal_flash_ex_block_erase(uint32_t addr, uint32_t size)
{
    int      error = 0;
    uint32_t current_size, current_addr;
    rhs_mutex_acquire(rhs_hal_flash_ex_mutex, RHSWaitForever);
    /* Check Flash busy ? */
    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        error = -1;
    } /* Enable write operations */
    else if (mt25ql128aba_write_enable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        error = -1;
    }
    else
    {
        current_addr = addr - (addr % MT25QL128ABA_SUBSECTOR_4K);
        current_size = MT25QL128ABA_SUBSECTOR_4K - (size % MT25QL128ABA_SUBSECTOR_4K);
        do
        {
            if (mt25ql128aba_block_erase(&hqspi, MT25QL128ABA_QPI_MODE, current_addr, MT25QL128ABA_ERASE_4K))
            {
                error = -1;
                break;
            }
            current_addr += MT25QL128ABA_SUBSECTOR_4K;
            current_size = (current_size > MT25QL128ABA_SUBSECTOR_4K) ? current_size - MT25QL128ABA_SUBSECTOR_4K : 0;
        } while (current_size);
    }

    rhs_mutex_release(rhs_hal_flash_ex_mutex);
    /* Return BSP status */
    return error;
}
