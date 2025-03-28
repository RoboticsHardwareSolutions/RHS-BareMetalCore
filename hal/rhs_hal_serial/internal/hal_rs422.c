#include "hal_rs422.h"
#include "rhs_hal_serial.h"
#include "rhs_hal_serial_types.h"

#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_bus.h"

#include "rhs.h"

#define TAG "rs422"

static uint32_t dma_bytes_available(RHSHalSerial* serial)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs422_init(void)
{
    GPIO_InitTypeDef         GPIO_InitStruct     = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    /** Initializes the peripherals clock
     */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART5;
    PeriphClkInitStruct.Uart5ClockSelection  = RCC_UART5CLKSOURCE_PCLK1;

    rhs_assert (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK);

    __HAL_RCC_UART5_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**UART5 GPIO Configuration
    PC12     ------> UART5_TX
    PB5     ------> UART5_RX
    */
    GPIO_InitStruct.Pin       = GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_5;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_UART5;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void rhs_hal_rs422_irq_callback(void* context)
{
    rhs_assert(context);
    RHSHalSerial*       serial = (RHSHalSerial*) context;
    RHSHalSerialRxEvent event  = 0;
    // Notification flags

    if (USART5->ISR & USART_ISR_RXNE)
    {
        event |= RHSHalSerialRxEventData;
    }
    if (USART5->ISR & USART_ISR_IDLE)
    {
        USART5->ICR = USART_ICR_IDLECF;
        event |= RHSHalSerialRxEventIdle;
    }
    // Error flags
    if (USART5->ISR & USART_ISR_ORE)
    {
        RHS_LOG_T(TAG, "RxOverrun");
        USART5->ICR = USART_ICR_ORECF;
        event |= RHSHalSerialRxEventOverrunError;
    }
    if (USART5->ISR & USART_ISR_NE)
    {
        RHS_LOG_T(TAG, "RxNoise");
        USART5->ICR = USART_ICR_NCF;
        event |= RHSHalSerialRxEventNoiseError;
    }
    if (USART5->ISR & USART_ISR_FE)
    {
        RHS_LOG_T(TAG, "RxFrameError");
        USART5->ICR = USART_ICR_FECF;
        event |= RHSHalSerialRxEventFrameError;
    }
    if (USART5->ISR & USART_ISR_PE)
    {
        RHS_LOG_T(TAG, "RxFrameErrorP");
        USART5->ICR = USART_ICR_PECF;
        event |= RHSHalSerialRxEventFrameError;
    }

    if (serial->buffer_rx_ptr == NULL)
    {
        if (serial->rx_byte_callback)
        {
            serial->rx_byte_callback(event, serial->context);
        }
    }
    else
    {
        if (serial->rx_dma_callback)
        {
            serial->rx_dma_callback(event, dma_bytes_available(serial), serial->context);
        }
    }
}

void rhs_hal_rs422_tx_irq_callback(void* context)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs422_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs422_async_tx_dma_configure(void)
{
    rhs_crash("Not implemented");
}
