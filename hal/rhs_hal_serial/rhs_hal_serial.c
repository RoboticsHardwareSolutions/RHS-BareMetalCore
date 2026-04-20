#include "stdbool.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "rserial.h"
#include "rhs.h"
#include "rhs_hal.h"
#include "hal_rs232.h"
#include "hal_rs485.h"

#if defined(BMPLC_XL) || defined(BMPLC_L)
#    include "stm32f7xx_ll_usart.h"
#    define RHS_INTERFACE_RS232 USART3
#    define RHS_INTERRUPT_RS232 RHSHalInterruptIdUsart3
#    define RHS_DMA_TX_RS232 RHSHalInterruptIdDMA1Stream3
// #    define RHS_DMA_RX_RS232 RHSHalInterruptIdUsart3 // TODO
#    define RHS_INTERFACE_RS485 USART6
#    define RHS_INTERRUPT_RS485 RHSHalInterruptIdUsart6
#    define RHS_DMA_TX_RS485 RHSHalInterruptIdDMA2Stream6
#    define RHS_DMA_RX_RS485 RHSHalInterruptIdDMA2Stream1
#    if !defined(BMPLC_XL)
#        define RHS_INTERFACE_RS422 USART5
#        define RHS_INTERRUPT_RS422 RHSHalInterruptIdUart5
// #        define RHS_DMA_TX_RS422 RHSHalInterruptIdDMA1Stream3 // TODO
// #        define RHS_DMA_RX_RS422 RHSHalInterruptIdUsart3 // TODO
#    endif
#elif defined(BMPLC_M)
#    include "stm32f1xx_ll_usart.h"
#    define RHS_INTERFACE_RS232 USART3
#    define RHS_INTERRUPT_RS232 RHSHalInterruptIdUsart3
#    define RHS_DMA_TX_RS232 RHSHalInterruptIdDMA1Channel2
#    define RHS_DMA_RX_RS232 RHSHalInterruptIdDMA1Channel3
#    define RHS_INTERFACE_RS485 UART5
#    define RHS_INTERRUPT_RS485 RHSHalInterruptIdUart5
#    define RHS_DMA_TX_RS485 RHSHalInterruptIdDMA1Channel2  // TODO
#    define RHS_DMA_RX_RS485 RHSHalInterruptIdDMA1Channel3  // TODO
#    define RHS_INTERFACE_RS422 UART4
#    define RHS_INTERRUPT_RS422 RHSHalInterruptIdUart4
#    define RHS_DMA_TX_RS422 RHSHalInterruptIdDMA1Channel2  // TODO
#    define RHS_DMA_RX_RS422 RHSHalInterruptIdDMA1Channel3  // TODO
#else
#    error "Not implemented Serial for this platform"
#endif

#include "rhs_hal_serial_types_i.h"

static RHSHalSerial rhs_hal_serial[RHSHalSerialIdMax] = {0};

static RHSHalSerialId rhs_hal_serial_get_id(RHSHalSerial* serial)
{
    rhs_assert(serial != NULL);
    for (uint32_t i = 0; i < RHSHalSerialIdMax; i++)
    {
        if (serial == &rhs_hal_serial[i])
            return (RHSHalSerialId) i;
    }
    rhs_crash("No serial handle");
}

/*********************************** SERIAL INIT ************************************/

RHSHalSerial* rhs_hal_serial_init(RHSHalSerialId id, uint32_t baud)
{
    rhs_assert(rhs_hal_serial[id].enabled == false);

    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_rs232_init();
        rserial_open(&rhs_hal_serial[RHSHalSerialIdRS232].rserial, "UART3", (int) baud, "8N1", FLOW_CTRL_NONE, 4000);
        rhs_hal_serial[RHSHalSerialIdRS232].enabled = true;
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_rs485_init();
#if defined(BMPLC_L) || defined(BMPLC_XL)
        rserial_open(&rhs_hal_serial[RHSHalSerialIdRS485].rserial, "UART6", (int) baud, "8N1", FLOW_CTRL_DE, 4000);
#elif defined(BMPLC_M)
        rserial_open(&rhs_hal_serial[RHSHalSerialIdRS485].rserial, "UART5", (int) baud, "8N1", FLOW_CTRL_DE, 4000);
#endif
        rhs_hal_serial[RHSHalSerialIdRS485].enabled = true;
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }

    return &rhs_hal_serial[id];
}

void rhs_hal_serial_deinit(RHSHalSerial* serial)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    rhs_assert(serial->enabled == true);
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_interrupt_set_isr(RHS_INTERRUPT_RS232, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHS_DMA_TX_RS232, NULL, NULL);
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_rs485_async_tx_dma_stop();
        rhs_hal_rs485_async_rx_dma_stop();
        rhs_hal_interrupt_set_isr(RHS_INTERRUPT_RS485, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHS_DMA_RX_RS485, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHS_DMA_TX_RS485, NULL, NULL);
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
    rserial_close(&serial->rserial);
    serial->enabled = false;
}

/*********************************** SERIAL TX ************************************/
void rhs_hal_serial_tx(RHSHalSerial* serial, const uint8_t* buffer, uint16_t buffer_size)
{
    rhs_assert(serial->enabled == true);
    if (LL_USART_IsEnabled(serial->rserial.uart.Instance) == 0)
        return;
#if defined(BMPLC_M)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
#endif
    while (buffer_size > 0)
    {
        while (!LL_USART_IsActiveFlag_TXE(serial->rserial.uart.Instance))
            ;
        LL_USART_TransmitData8(serial->rserial.uart.Instance, *buffer);

        buffer++;
        buffer_size--;
    }
#if defined(BMPLC_M)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
#endif
}

void rhs_hal_serial_async_tx_dma_configure(RHSHalSerial* serial)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_interrupt_set_isr(RHS_DMA_TX_RS232, rhs_hal_rs232_tx_irq_callback, serial);
        rhs_hal_rs232_async_tx_dma_configure();
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_interrupt_set_isr(RHS_DMA_TX_RS485, rhs_hal_rs485_tx_irq_callback, serial);
        rhs_hal_rs485_async_tx_dma_configure();
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_async_tx_dma_start(RHSHalSerial*             serial,
                                       RHSHalSerialDMATxCallback callback,
                                       void*                     context,
                                       const uint8_t*            buffer,
                                       uint16_t                  buffer_size)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    serial->tx_dma_callback  = callback;
    serial->tx_byte_callback = NULL;
    serial->context          = context;
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_rs232_async_tx_dma_start(buffer, buffer_size);
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_rs485_async_tx_dma_start(buffer, buffer_size);
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

/*********************************** SERIAL RX ************************************/

void rhs_hal_serial_async_rx_start(RHSHalSerial* serial, RHSHalSerialAsyncRxCallback callback, void* context)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    rhs_assert(serial->enabled == true);
    rhs_assert(callback);

    serial->rx_byte_callback = callback;
    serial->rx_dma_callback  = NULL;
    serial->context          = context;

    switch (id)
    {
    case RHSHalSerialIdRS232:
        LL_USART_EnableIT_RXNE(RHS_INTERFACE_RS232);
        rhs_hal_interrupt_set_isr(RHS_INTERRUPT_RS232, rhs_hal_rs232_irq_callback, serial);
        break;
    case RHSHalSerialIdRS485:
        LL_USART_EnableIT_RXNE(RHS_INTERFACE_RS485);
        rhs_hal_interrupt_set_isr(RHS_INTERRUPT_RS485, rhs_hal_rs485_rx_irq_callback, serial);
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

uint8_t rhs_hal_serial_async_rx(RHSHalSerial* serial)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    rhs_assert(serial->enabled == true);
    rhs_assert(RHS_IS_IRQ_MODE());

    switch (id)
    {
    case RHSHalSerialIdRS232:
        return LL_USART_ReceiveData8(serial->rserial.uart.Instance);
    case RHSHalSerialIdRS485:
        return LL_USART_ReceiveData8(serial->rserial.uart.Instance);
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_async_rx_dma_configure(RHSHalSerial* serial, RHSHalSerialDmaRxCallback callback, void* context)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    rhs_assert(serial->enabled == true);
    rhs_assert(callback);
    serial->buffer_rx_ptr         = NULL;
    serial->buffer_rx_index_write = 0;
    serial->buffer_rx_index_read  = 0;

    serial->rx_byte_callback = NULL;
    serial->rx_dma_callback  = callback;
    serial->context          = context;

    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_crash("Not implemented RHSHalSerialIdRS232");
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_interrupt_set_isr(RHS_INTERRUPT_RS485, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHS_DMA_RX_RS485, rhs_hal_rs485_rx_irq_callback, serial);
        rhs_hal_rs485_async_rx_dma_configure();
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_async_rx_dma_start(RHSHalSerial* serial, uint8_t* buffer, uint16_t buffer_size)
{
    RHSHalSerialId id = rhs_hal_serial_get_id(serial);
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_crash("Not implemented RHSHalSerialIdRS232");
        break;
    case RHSHalSerialIdRS485:
        serial->buffer_rx_ptr = buffer;
        rhs_hal_rs485_async_rx_dma_start(buffer, buffer_size);
        break;
#if !defined(BMPLC_XL)
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
#endif
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}
