#include "rhs.h"
#include "hal_rs485.h"
#include "rhs_hal_serial.h"
#include "rhs_hal_serial_types.h"

#define TAG "rs485"

#if defined(RPLC_XL) || defined(RPLC_L)

#    include "stm32f7xx_ll_rcc.h"
#    include "stm32f7xx_ll_dma.h"
#    include "stm32f7xx_ll_usart.h"
#    include "stm32f7xx_ll_bus.h"

static uint32_t dma_bytes_available(RHSHalSerial* serial)
{
    rhs_assert(serial);
    size_t dma_remain = LL_DMA_GetDataLength(DMA2, LL_DMA_STREAM_6);

    serial->buffer_rx_index_write = RHS_HAL_SERIAL_DMA_BUFFER_SIZE - dma_remain;
    if (serial->buffer_rx_index_write >= serial->buffer_rx_index_read)
    {
        return serial->buffer_rx_index_write - serial->buffer_rx_index_read;
    }
    else
    {
        return RHS_HAL_SERIAL_DMA_BUFFER_SIZE - serial->buffer_rx_index_read + serial->buffer_rx_index_write;
    }
}

void rhs_hal_rs485_init(void)
{
    GPIO_InitTypeDef         GPIO_InitStruct     = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    /** Initializes the peripherals clock
     */

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART6;
    PeriphClkInitStruct.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
    rhs_assert(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK);

    /* USART6 clock enable */
    __HAL_RCC_USART6_CLK_ENABLE();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART6 GPIO Configuration
    PG8     ------> USART6_DE
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
    GPIO_InitStruct.Pin       = GPIO_PIN_8;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void rhs_hal_rs485_rx_irq_callback(void* context)
{
    rhs_assert(context);
    RHSHalSerial*       serial = (RHSHalSerial*) context;
    RHSHalSerialRxEvent event  = 0;
    // Notification flags

    if (USART6->ISR & USART_ISR_RXNE)
    {
        event |= RHSHalSerialRxEventData;
    }
    if (USART6->ISR & USART_ISR_IDLE)
    {
        USART6->ICR = USART_ICR_IDLECF;
        event |= RHSHalSerialRxEventIdle;
    }
    // Error flags
    if (USART6->ISR & USART_ISR_ORE)
    {
        RHS_LOG_E(TAG, "RxOverrun");
        USART6->ICR = USART_ICR_ORECF;
        event |= RHSHalSerialRxEventOverrunError;
    }
    if (USART6->ISR & USART_ISR_NE)
    {
        RHS_LOG_E(TAG, "RxNoise");
        USART6->ICR = USART_ICR_NCF;
        event |= RHSHalSerialRxEventNoiseError;
    }
    if (USART6->ISR & USART_ISR_FE)
    {
        RHS_LOG_E(TAG, "RxFrameError");
        USART6->ICR = USART_ICR_FECF;
        event |= RHSHalSerialRxEventFrameError;
    }
    if (USART6->ISR & USART_ISR_PE)
    {
        RHS_LOG_E(TAG, "RxFrameErrorP");
        USART6->ICR = USART_ICR_PECF;
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
        if (!LL_DMA_IsActiveFlag_TE1(DMA2)) /* If no error in DMA call a callback*/
        {
            if (serial->rx_dma_callback)
            {
                serial->rx_dma_callback(event, LL_DMA_GetDataLength(DMA2, LL_DMA_STREAM_1), serial->context);
            }
        }
        else
        {
            LL_DMA_ClearFlag_TE1(DMA2);
        }
        if (DMA2->LISR & DMA_LISR_TCIF1)
        {
            DMA2->LIFCR = DMA_LIFCR_CTCIF1;
        }
    }
}

void rhs_hal_rs485_tx_irq_callback(void* context)
{
    rhs_assert(context);
    RHSHalSerial* serial = (RHSHalSerial*) context;
    if (serial->tx_byte_callback)
    {
        serial->tx_byte_callback(serial->context);
    }
    else
    {
        if (DMA2->HISR & DMA_HISR_TCIF6)
        {
            DMA2->HIFCR = DMA_HIFCR_CTCIF6;
        }
        if (serial->tx_dma_callback)
        {
            serial->tx_dma_callback(serial->context);
        }
    }
}

void rhs_hal_rs485_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size)
{
    LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t) buffer);
    LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, buffer_size);
    LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_6);
}

void rhs_hal_rs485_async_tx_dma_stop(void)
{
    LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t) NULL);
    LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, 0);
    LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_6);

    LL_DMA_DisableIT_TC(DMA2, LL_DMA_STREAM_6);
    NVIC_DisableIRQ(DMA2_Stream6_IRQn);
}

void rhs_hal_rs485_async_tx_dma_configure(void)
{
    /* Init with LL driver */
    /* DMA controller clock enable */
    USART6->CR3 |= USART_CR3_DMAT;
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

    DMA2_Stream6->CR &= ~(DMA_SxCR_EN);
    while ((DMA2_Stream6->CR & (DMA_SxCR_EN)))
        ;

    /* DMA interrupt init */
    /* DMA2_Stream6_IRQn interrupt configuration */
    LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_6, LL_DMA_CHANNEL_5);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_6, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetPeriphAddress(DMA2, LL_DMA_STREAM_6, (uint32_t) &USART6->TDR);
    LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_6, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA2, LL_DMA_STREAM_6, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_6, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_6, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_6, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_6, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_6);

    LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_6);
    NVIC_SetPriority(DMA2_Stream6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
    NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}

void rhs_hal_rs485_async_rx_dma_configure(void)
{
    /* Init with LL driver */
    /* DMA controller clock enable */
    USART6->CR3 |= USART_CR3_DMAR;
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

    DMA2_Stream1->CR &= ~(DMA_SxCR_EN);
    while ((DMA2_Stream1->CR & (DMA_SxCR_EN)))
        ;

    /* DMA interrupt init */
    /* DMA2_Stream1_IRQn interrupt configuration */
    LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_1, LL_DMA_CHANNEL_5);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetPeriphAddress(DMA2, LL_DMA_STREAM_1, (uint32_t) &USART6->RDR);
    LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_1, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA2, LL_DMA_STREAM_1, LL_DMA_MODE_NORMAL);  // ??????
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_1, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_1, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_1, LL_DMA_MDATAALIGN_BYTE);

    if (LL_DMA_IsActiveFlag_HT1(DMA2))
        LL_DMA_ClearFlag_HT1(DMA2);
    if (LL_DMA_IsActiveFlag_TC1(DMA2))
        LL_DMA_ClearFlag_TC1(DMA2);
    if (LL_DMA_IsActiveFlag_TE1(DMA2))
        LL_DMA_ClearFlag_TE1(DMA2);

    LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_1);
    NVIC_SetPriority(DMA2_Stream1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

void rhs_hal_rs485_async_rx_dma_start(const uint8_t* buffer, uint16_t buffer_size)
{
    LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_1, (uint32_t) buffer);
    LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_1, buffer_size);
    LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_1);
}

void rhs_hal_rs485_async_rx_dma_stop(void)
{
    LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_1, (uint32_t) NULL);
    LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_1, 0);
    LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_1);

    LL_DMA_DisableIT_TC(DMA2, LL_DMA_STREAM_1);
    NVIC_DisableIRQ(DMA2_Stream1_IRQn);
}

#elif defined(RPLC_M)
#    include "stm32f1xx_ll_rcc.h"
#    include "stm32f1xx_ll_dma.h"
#    include "stm32f1xx_ll_usart.h"
#    include "stm32f1xx_ll_bus.h"

static uint32_t dma_bytes_available(RHSHalSerial* serial) {}

void rhs_hal_rs485_init(void)
{
    GPIO_InitTypeDef         GPIO_InitStruct     = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    __HAL_RCC_UART5_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**UART5 GPIO Configuration
    PC12     ------> UART5_TX
    PD02     ------> UART5_RX
    PA15     ------> UART5_DE
    */
    GPIO_InitStruct.Pin   = GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_15;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void rhs_hal_rs485_rx_irq_callback(void* context)
{
    rhs_assert(context);
    RHSHalSerial*       serial = (RHSHalSerial*) context;
    RHSHalSerialRxEvent event  = 0;
    // Notification flags

    if (UART5->SR & USART_SR_RXNE)
    {
        event |= RHSHalSerialRxEventData;
    }
    if (UART5->SR & USART_SR_IDLE)
    {
        // UART5->CR = USART_ICR_IDLECF;
        event |= RHSHalSerialRxEventIdle;
    }
    // Error flags
    if (UART5->SR & USART_SR_ORE)
    {
        RHS_LOG_T(TAG, "RxOverrun", UART5->SR);
        // UART5->CR1 = USART_CR1_ORECF;
        event |= RHSHalSerialRxEventOverrunError;
    }
    if (UART5->SR & USART_SR_NE)
    {
        RHS_LOG_T(TAG, "RxNoise", UART5->SR);
        // UART5->ICR = USART_ICR_NCF;
        event |= RHSHalSerialRxEventNoiseError;
    }
    if (UART5->SR & USART_SR_FE)
    {
        RHS_LOG_T(TAG, "RxFrameError", UART5->SR);
        // UART5->ICR = USART_ICR_FECF;
        event |= RHSHalSerialRxEventFrameError;
    }
    if (UART5->SR & USART_SR_PE)
    {
        RHS_LOG_T(TAG, "RxFrameErrorP", UART5->SR);
        // UART5->ICR = USART_ICR_PECF;
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

void rhs_hal_rs485_tx_irq_callback(void* context)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs485_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs485_async_tx_dma_stop(void)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs485_async_tx_dma_configure(void)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs485_async_rx_dma_configure(void)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs485_async_rx_dma_start(const uint8_t* buffer, uint16_t buffer_size)
{
    rhs_crash("Not implemented");
}

void rhs_hal_rs485_async_rx_dma_stop(void)
{
    rhs_crash("Not implemented");
}
#endif
