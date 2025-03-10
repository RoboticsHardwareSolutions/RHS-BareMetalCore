#include "hal_rs232.h"
#include "rhs_hal_serial.h"
#include "rhs_hal_serial_types.h"

#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_bus.h"

#include "rhs.h"

#define TAG "rs232"

static uint32_t dma_bytes_available(RHSHalSerial* serial)
{
    rhs_assert(serial);
    size_t dma_remain = LL_DMA_GetDataLength(DMA1, LL_DMA_STREAM_3);

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

void rhs_hal_rs232_init(void)
{
    GPIO_InitTypeDef         GPIO_InitStruct     = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    /** Initializes the peripherals clock
     */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;

    rhs_assert(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK);
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    */
    GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void rhs_hal_rs232_irq_callback(void* context)
{
    rhs_assert(context);
    RHSHalSerial*       serial = (RHSHalSerial*) context;
    RHSHalSerialRxEvent event  = 0;
    // Notification flags

    if (USART3->ISR & USART_ISR_RXNE)
    {
        event |= RHSHalSerialRxEventData;
    }
    if (USART3->ISR & USART_ISR_IDLE)
    {
        USART3->ICR = USART_ICR_IDLECF;
        event |= RHSHalSerialRxEventIdle;
    }
    // Error flags
    if (USART3->ISR & USART_ISR_ORE)
    {
        RHS_LOG_T(TAG, "RxOverrun\r\n", USART3->ISR);
        USART3->ICR = USART_ICR_ORECF;
        event |= RHSHalSerialRxEventOverrunError;
    }
    if (USART3->ISR & USART_ISR_NE)
    {
        RHS_LOG_T(TAG, "RxNoise\r\n", USART3->ISR);
        USART3->ICR = USART_ICR_NCF;
        event |= RHSHalSerialRxEventNoiseError;
    }
    if (USART3->ISR & USART_ISR_FE)
    {
        RHS_LOG_T(TAG, "RxFrameError\r\n", USART3->ISR);
        USART3->ICR = USART_ICR_FECF;
        event |= RHSHalSerialRxEventFrameError;
    }
    if (USART3->ISR & USART_ISR_PE)
    {
        RHS_LOG_T(TAG, "RxFrameErrorP\r\n", USART3->ISR);
        USART3->ICR = USART_ICR_PECF;
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
            serial->rx_dma_callback(event,
                                    dma_bytes_available(serial),
                                    serial->context);
        }
    }
}

void rhs_hal_rs232_tx_irq_callback(void* context)
{
    rhs_assert(context);
    RHSHalSerial* serial = (RHSHalSerial*) context;
    if (serial->tx_byte_callback)
    {
        serial->tx_byte_callback(serial->context);
    }
    else
    {
        if (DMA1->LISR & DMA_LISR_TCIF3)
        {
            DMA1->LIFCR = DMA_LIFCR_CTCIF3;
        }
        if (serial->tx_dma_callback)
        {
            serial->tx_dma_callback(serial->context);
        }
    }
}

void rhs_hal_rs232_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size)
{
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_3, (uint32_t) buffer);
    LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_3, buffer_size);
    LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_3);
}

void rhs_hal_rs232_async_tx_dma_configure(void)
{
    /* Init with LL driver */
    /* DMA controller clock enable */
    USART3->CR3 |= USART_CR3_DMAT;
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    DMA1_Stream3->CR &= ~(DMA_SxCR_EN);
    while ((DMA1_Stream3->CR & (DMA_SxCR_EN)))
        ;

    /* DMA interrupt init */
    /* DMA1_Stream3_IRQn interrupt configuration */
    LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_3, LL_DMA_CHANNEL_4);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_STREAM_3, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_3, (uint32_t) &USART3->TDR);
    LL_DMA_SetStreamPriorityLevel(DMA1, LL_DMA_STREAM_3, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_STREAM_3, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_STREAM_3, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_STREAM_3, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_STREAM_3, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_STREAM_3, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode(DMA1, LL_DMA_STREAM_3);

    LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_3);
    NVIC_SetPriority(DMA1_Stream3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}
