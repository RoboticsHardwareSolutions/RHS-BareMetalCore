/*
 * External Flash HAL Implementation
 *
 * This file implements flash operations that are safe to call from both
 * task context and interrupt context. When called from ISR context,
 * non-blocking mutex operations are used to avoid deadlocks.
 */

#include "rhs_hal_flash_ex.h"
#include "rhs.h"
#include "mt25ql128aba.h"

#if defined(BMPLC_XL) || defined(BMPLC_L)
QSPI_HandleTypeDef hqspi;
static RHSMutex*   flash_mutex = NULL;

static void quadspi_init(void)
{
    hqspi.Instance                = QUADSPI;
    hqspi.Init.ClockPrescaler     = 2;
    hqspi.Init.FifoThreshold      = 4;
    hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE;
    hqspi.Init.FlashSize          = 23;
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_6_CYCLE;
    hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&hqspi) != HAL_OK)
    {
        rhs_crash("SystemClock_Config failed");
    }
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef* qspiHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (qspiHandle->Instance == QUADSPI)
    {
#    if defined(BMPLC_L)
        __HAL_RCC_QSPI_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        /**QUADSPI GPIO Configuration
        PE2     ------> QUADSPI_BK1_IO2
        PA1     ------> QUADSPI_BK1_IO3
        PB2     ------> QUADSPI_CLK
        PD11     ------> QUADSPI_BK1_IO0
        PD12     ------> QUADSPI_BK1_IO1
        PB6     ------> QUADSPI_BK1_NCS
        */
        GPIO_InitStruct.Pin       = GPIO_PIN_2;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_1;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_2;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_6;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

#    elif defined(BMPLC_XL)
        __HAL_RCC_QSPI_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**QUADSPI GPIO Configuration
        PF6     ------> QUADSPI_BK1_IO3
        PF7     ------> QUADSPI_BK1_IO2
        PF8     ------> QUADSPI_BK1_IO0
        PF9     ------> QUADSPI_BK1_IO1
        PF10     ------> QUADSPI_CLK
        PB6     ------> QUADSPI_BK1_NCS
        */
        GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_6;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#    endif
    }
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* qspiHandle)
{
    if (qspiHandle->Instance == QUADSPI)
    {
#    if defined(BMPLC_L)
        __HAL_RCC_QSPI_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2 | GPIO_PIN_6);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11 | GPIO_PIN_12);
#    elif defined(BMPLC_XL)
        __HAL_RCC_QSPI_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
#    endif
    }
}
#else
#    error "Not implementer flash QSPI for this platform"
#endif

int rhs_hal_flash_ex_init(void)
{
    flash_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
    if (flash_mutex == NULL)
    {
        return -1;
    }

    quadspi_init();
    mt25ql128aba_init(&hqspi);
    return 0;
}

int rhs_hal_flash_ex_read(uint32_t addr, uint8_t* p_data, uint32_t size)
{
    int error = RHS_FLASH_EX_OK;
    rhs_assert(addr + size <= MT25QL128ABA_FLASH_SIZE);

    if (RHS_IS_IRQ_MODE())
    {
        if (rhs_mutex_get_owner(flash_mutex) != NULL)
            return RHS_FLASH_EX_BUSY;
    }
    else if (rhs_mutex_acquire(flash_mutex, RHSWaitForever) != RHSStatusOk)
    {
        return RHS_FLASH_EX_ERROR;
    }

    /* Check Flash busy ? */
    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    }
    if (mt25ql128aba_read(&hqspi, MT25QL128ABA_QPI_MODE, p_data, addr, size) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    }
    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    }

    // Only release mutex if we successfully acquired it (not in IRQ mode)
    if (!RHS_IS_IRQ_MODE())
    {
        rhs_mutex_release(flash_mutex);
    }
    return error;
}

int rhs_hal_flash_ex_erase_chip(void)
{
    int error = RHS_FLASH_EX_OK;

    if (RHS_IS_IRQ_MODE())
    {
        if (rhs_mutex_get_owner(flash_mutex) != NULL)
            return RHS_FLASH_EX_BUSY;
    }
    else if (rhs_mutex_acquire(flash_mutex, RHSWaitForever) != RHSStatusOk)
    {
        return RHS_FLASH_EX_ERROR;
    }

    /* Check Flash busy ? */
    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    } /* Enable write operations */
    else if (mt25ql128aba_write_enable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    }
    else
    {
        /* Issue Chip erase command */
        if (mt25ql128aba_chip_erase(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
        {
            error = RHS_FLASH_EX_ERROR;
        }
    }

    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, MT25QL128ABA_BULK_ERASE_MAX_TIME) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    }

    if (!RHS_IS_IRQ_MODE())
    {
        rhs_mutex_release(flash_mutex);
    }
    /* Return BSP status */
    return error;
}

int rhs_hal_flash_ex_write(uint32_t addr, uint8_t* p_data, uint32_t size)
{
    uint32_t end_addr, current_size, current_addr;
    uint8_t* write_data;
    int      error = RHS_FLASH_EX_OK;
    rhs_assert(addr + size <= MT25QL128ABA_FLASH_SIZE);

    if (RHS_IS_IRQ_MODE())
    {
        if (rhs_mutex_get_owner(flash_mutex) != NULL)
            return RHS_FLASH_EX_BUSY;
    }
    else if (rhs_mutex_acquire(flash_mutex, RHSWaitForever) != RHSStatusOk)
    {
        return RHS_FLASH_EX_ERROR;
    }

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
        if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
        {
            error = RHS_FLASH_EX_ERROR;
            break;
        }
        if (mt25ql128aba_write_enable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
        {
            error = RHS_FLASH_EX_ERROR;
            break;
        }
        if (mt25ql128aba_page_program(&hqspi, MT25QL128ABA_QPI_MODE, write_data, current_addr, current_size) != 0)
        {
            error = RHS_FLASH_EX_ERROR;
            break;
        }
        if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
        {
            error = RHS_FLASH_EX_ERROR;
            break;
        }

        /* Update the address and size variables for next page programming */
        current_addr += current_size;
        write_data += current_size;
        current_size =
            ((current_addr + MT25QL128ABA_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MT25QL128ABA_PAGE_SIZE;
    } while ((current_addr < end_addr));

    if (!RHS_IS_IRQ_MODE())
    {
        rhs_mutex_release(flash_mutex);
    }
    return error;
}

int rhs_hal_flash_ex_block_erase(uint32_t addr, uint32_t size)
{
    int      error = RHS_FLASH_EX_OK;
    uint32_t current_size, current_addr;
    rhs_assert(addr + size <= MT25QL128ABA_FLASH_SIZE);

    if (RHS_IS_IRQ_MODE())
    {
        if (rhs_mutex_get_owner(flash_mutex) != NULL)
            return RHS_FLASH_EX_BUSY;
    }
    else if (rhs_mutex_acquire(flash_mutex, RHSWaitForever) != RHSStatusOk)
    {
        return RHS_FLASH_EX_ERROR;
    }

    /* Check Flash busy ? */
    if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
    {
        error = RHS_FLASH_EX_ERROR;
    }
    else
    {
        current_addr = addr - (addr % MT25QL128ABA_SUBSECTOR_4K);
        current_size = size;
        do
        {
            if (mt25ql128aba_write_enable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
            {
                error = RHS_FLASH_EX_ERROR;
                break;
            }
            if (mt25ql128aba_block_erase(&hqspi, MT25QL128ABA_QPI_MODE, current_addr, MT25QL128ABA_ERASE_4K))
            {
                error = RHS_FLASH_EX_ERROR;
                break;
            }
            if (mt25ql128aba_auto_polling_mem_ready(&hqspi,
                                                    MT25QL128ABA_QPI_MODE,
                                                    MT25QL128ABA_SUBSECTOR_4K_ERASE_MAX_TIME) != 0)
            {
                error = RHS_FLASH_EX_ERROR;
                break;
            }
            current_addr += MT25QL128ABA_SUBSECTOR_4K;
            current_size = (current_size > MT25QL128ABA_SUBSECTOR_4K) ? current_size - MT25QL128ABA_SUBSECTOR_4K : 0;
        } while (current_size);
    }
    if (!RHS_IS_IRQ_MODE())
    {
        rhs_mutex_release(flash_mutex);
    }
    /* Return BSP status */
    return error;
}
