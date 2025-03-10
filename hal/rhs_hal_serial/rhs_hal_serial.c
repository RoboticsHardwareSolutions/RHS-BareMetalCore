#include "stdbool.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "rserial.h"
#include "rhs.h"
#include "rhs_hal.h"
#include "hal_rs232.h"
#include "hal_rs485.h"

#include "stm32f7xx_ll_usart.h"
#include "rhs_hal_serial_types.h"

static RHSHalSerial rhs_hal_serial[RHSHalSerialIdMax] = {0};

/*********************************** SERIAL INIT ************************************/

void rhs_hal_serial_init(RHSHalSerialId id, uint32_t baud)
{
    rhs_assert(rhs_hal_serial[id].enabled == false);

    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_rs232_init();
        rserial_open(&rhs_hal_serial[RHSHalSerialIdRS232].rserial, "UART3", (int) baud, "8N1", FLOW_CTRL_NONE, 4000);
        rhs_hal_serial[id].enabled = true;
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_rs485_init();
        rserial_open(&rhs_hal_serial[RHSHalSerialIdRS485].rserial, "UART6", (int) baud, "8N1", FLOW_CTRL_DE, 4000);
        rhs_hal_serial[RHSHalSerialIdRS485].enabled = true;
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_deinit(RHSHalSerialId id)
{
    rhs_assert(rhs_hal_serial[id].enabled == true);
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdUsart3, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdDMA1Stream3, NULL, NULL);
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_rs485_async_tx_dma_stop();
        rhs_hal_rs485_async_rx_dma_stop();
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdUsart6, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdDMA2Stream1, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdDMA2Stream6, NULL, NULL);
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
    rserial_close(&rhs_hal_serial[id].rserial);
    rhs_hal_serial[id].enabled = false;
}

/*********************************** SERIAL TX ************************************/

void rhs_hal_serial_async_tx_dma_configure(RHSHalSerialId id)
{
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdDMA1Stream3,
                                  rhs_hal_rs232_tx_irq_callback,
                                  &rhs_hal_serial[RHSHalSerialIdRS232]);
        rhs_hal_rs232_async_tx_dma_configure();
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdDMA2Stream6,
                                  rhs_hal_rs485_tx_irq_callback,
                                  &rhs_hal_serial[RHSHalSerialIdRS485]);
        rhs_hal_rs485_async_tx_dma_configure();
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_async_tx_dma_start(RHSHalSerialId            id,
                                       RHSHalSerialDMATxCallback callback,
                                       void*                     context,
                                       const uint8_t*            buffer,
                                       uint16_t                  buffer_size)
{
    rhs_hal_serial[id].tx_dma_callback  = callback;
    rhs_hal_serial[id].tx_byte_callback = NULL;
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_hal_rs232_async_tx_dma_start(buffer, buffer_size);
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_rs485_async_tx_dma_start(buffer, buffer_size);
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

/*********************************** SERIAL RX ************************************/

void rhs_hal_serial_async_rx_start(RHSHalSerialId id, RHSHalSerialAsyncRxCallback callback, void* context)
{
    rhs_assert(rhs_hal_serial[id].enabled == true);
    rhs_assert(callback);

    rhs_hal_serial[id].rx_byte_callback = callback;
    rhs_hal_serial[id].rx_dma_callback  = NULL;
    rhs_hal_serial[id].context          = context;

    switch (id)
    {
    case RHSHalSerialIdRS232:
        LL_USART_EnableIT_RXNE(USART3);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdUsart3, rhs_hal_rs232_irq_callback, &rhs_hal_serial[id]);
        break;
    case RHSHalSerialIdRS485:
        LL_USART_EnableIT_RXNE(USART6);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdUsart6, rhs_hal_rs485_rx_irq_callback, &rhs_hal_serial[id]);
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

uint8_t rhs_hal_serial_async_rx(RHSHalSerialId id)
{
    rhs_assert(id < RHSHalSerialIdMax);
    rhs_assert(rhs_hal_serial[id].enabled == true);
    rhs_assert(RHS_IS_IRQ_MODE());

    switch (id)
    {
    case RHSHalSerialIdRS232:
        return LL_USART_ReceiveData8(rhs_hal_serial[id].rserial.uart.Instance);
    case RHSHalSerialIdRS485:
        return LL_USART_ReceiveData8(rhs_hal_serial[id].rserial.uart.Instance);
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_async_rx_dma_configure(RHSHalSerialId id, RHSHalSerialDmaRxCallback callback, void* context)
{
    rhs_assert(id < RHSHalSerialIdMax);
    rhs_assert(rhs_hal_serial[id].enabled == true);
    rhs_assert(callback);
    rhs_hal_serial[id].buffer_rx_ptr         = NULL;
    rhs_hal_serial[id].buffer_rx_index_write = 0;
    rhs_hal_serial[id].buffer_rx_index_read  = 0;

    rhs_hal_serial[id].rx_byte_callback = NULL;
    rhs_hal_serial[id].rx_dma_callback  = callback;
    rhs_hal_serial[id].context          = context;

    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_crash("Not implemented RHSHalSerialIdRS232");
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdUsart6, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdDMA2Stream1, rhs_hal_rs485_rx_irq_callback, &rhs_hal_serial[id]);
        rhs_hal_rs485_async_rx_dma_configure();
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}

void rhs_hal_serial_async_rx_dma_start(RHSHalSerialId id, uint8_t* buffer, uint16_t buffer_size)
{
    rhs_assert(id < RHSHalSerialIdMax);
    switch (id)
    {
    case RHSHalSerialIdRS232:
        rhs_crash("Not implemented RHSHalSerialIdRS232");
        break;
    case RHSHalSerialIdRS485:
        rhs_hal_serial[id].buffer_rx_ptr = buffer;
        rhs_hal_rs485_async_rx_dma_start(buffer, buffer_size);
        break;
    case RHSHalSerialIdRS422:
        rhs_crash("Not implemented RHSHalSerialIdRS422");
        break;
    case RHSHalSerialIdMax:
    default:
        rhs_crash("No serial id");
    }
}
