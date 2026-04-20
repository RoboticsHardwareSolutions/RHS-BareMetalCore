#pragma once
#include "stdint.h"
#include "stdbool.h"
#include "rhs_hal_serial_types.h"

#define RHS_HAL_SERIAL_DMA_BUFFER_SIZE (1024u)

/**
 * UART channels
 */
typedef enum
{
    RHSHalSerialIdRS232,
    RHSHalSerialIdRS485,
#if !defined(BMPLC_XL)
    RHSHalSerialIdRS422,
#endif
    RHSHalSerialIdMax,
} RHSHalSerialId;

RHSHalSerial* rhs_hal_serial_init(RHSHalSerialId id, uint32_t baud);

void rhs_hal_serial_deinit(RHSHalSerial* serial);

void rhs_hal_serial_tx(RHSHalSerial* serial, const uint8_t* buffer, uint16_t buffer_size);
/** Receive callback
 *
 * @warning    Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 * @param      serial   Serial handle
 * @param      event    RHSHalSerialRxEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalSerialAsyncTxCallback)(RHSHalSerial* serial, void* context);

void rhs_hal_serial_async_tx_dma_configure(RHSHalSerial* serial);

/** Transmits data in semi-blocking mode
 *
 * Fills transmission pipe with data, returns as soon as all bytes from buffer
 * are in the pipe.
 *
 * Real transmission will be completed later. Use
 * `rhs_hal_serial_async_tx_wait_complete` to wait for completion if you need it.
 *
 * @param      serial       Serial handle
 * @param      buffer       data
 * @param      buffer_size  data size (in bytes)
 */
typedef void (*RHSHalSerialDMATxCallback)(RHSHalSerial* serial, void* context);

void rhs_hal_serial_async_tx_dma_start(RHSHalSerial*             serial,
                                       RHSHalSerialDMATxCallback callback,
                                       void*                     context,
                                       const uint8_t*            buffer,
                                       uint16_t                  buffer_size);

/** Serial RX events */
typedef enum
{
    RHSHalSerialRxEventData         = (1 << 0), /**< Data: new data available */
    RHSHalSerialRxEventIdle         = (1 << 1), /**< Idle: bus idle detected */
    RHSHalSerialRxEventFrameError   = (1 << 2), /**< Framing Error: incorrect frame detected */
    RHSHalSerialRxEventNoiseError   = (1 << 3), /**< Noise Error: noise on the line detected */
    RHSHalSerialRxEventOverrunError = (1 << 4), /**< Overrun Error: no space for received data */
} RHSHalSerialRxEvent;

/** Receive callback
 *
 * @warning    Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 * @param      serial   Serial handle
 * @param      event    RHSHalSerialRxEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalSerialAsyncRxCallback)(RHSHalSerial* serial, RHSHalSerialRxEvent event, void* context);

/** Receive DMA callback
 *
 * @warning    DMA Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 *
 * @param      serial    Serial handle
 * @param      event     RHSHalSerialDmaRxEvent
 * @param      data_len  Received data
 * @param      context   Callback context provided earlier
 */
typedef void (*RHSHalSerialDmaRxCallback)(RHSHalSerial* serial, RHSHalSerialRxEvent event, uint16_t data_len, void* context);

void rhs_hal_serial_async_rx_start(RHSHalSerial* serial, RHSHalSerialAsyncRxCallback callback, void* context);

uint8_t rhs_hal_serial_async_rx(RHSHalSerial* serial);

void rhs_hal_serial_async_rx_dma_configure(RHSHalSerial* serial, RHSHalSerialDmaRxCallback callback, void* context);
void rhs_hal_serial_async_rx_dma_start(RHSHalSerial* serial, uint8_t* buffer, uint16_t buffer_size);
