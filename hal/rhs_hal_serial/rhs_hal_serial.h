#pragma once
#include "stdint.h"
#include "stdbool.h"

#define RHS_HAL_SERIAL_DMA_BUFFER_SIZE (1024u)

/**
 * UART channels
 */
typedef enum
{
    RHSHalSerialIdRS232,
    RHSHalSerialIdRS485,
#if !defined(RPLC_XL)
    RHSHalSerialIdRS422,
#endif
    RHSHalSerialIdMax,
} RHSHalSerialId;

void rhs_hal_serial_init(RHSHalSerialId id, uint32_t baud);

void rhs_hal_serial_deinit(RHSHalSerialId id);

void rhs_hal_serial_tx(RHSHalSerialId id, const uint8_t* buffer, uint16_t buffer_size);
/** Receive callback
 *
 * @warning    Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 * @param      handle   Serial handle
 * @param      event    RHSHalSerialRxEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalSerialAsyncTxCallback)(void* context);

void rhs_hal_serial_async_tx_dma_configure(RHSHalSerialId id);

/** Transmits data in semi-blocking mode
 *
 * Fills transmission pipe with data, returns as soon as all bytes from buffer
 * are in the pipe.
 *
 * Real transmission will be completed later. Use
 * `rhs_hal_serial_async_tx_wait_complete` to wait for completion if you need it.
 *
 * @param      handle       Serial handle
 * @param      buffer       data
 * @param      buffer_size  data size (in bytes)
 */
typedef void (*RHSHalSerialDMATxCallback)(void* context);

void rhs_hal_serial_async_tx_dma_start(RHSHalSerialId            id,
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
 * @param      handle   Serial handle
 * @param      event    RHSHalSerialRxEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalSerialAsyncRxCallback)(RHSHalSerialRxEvent event, void* context);

/** Receive DMA callback
 *
 * @warning    DMA Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 *
 * @param      handle    Serial handle
 * @param      event     RHSHalSerialDmaRxEvent
 * @param      data_len  Received data
 * @param      context   Callback context provided earlier
 */
typedef void (*RHSHalSerialDmaRxCallback)(RHSHalSerialRxEvent event, uint16_t data_len, void* context);

void rhs_hal_serial_async_rx_start(RHSHalSerialId id, RHSHalSerialAsyncRxCallback callback, void* context);

uint8_t rhs_hal_serial_async_rx(RHSHalSerialId id);

void rhs_hal_serial_async_rx_dma_configure(RHSHalSerialId id, RHSHalSerialDmaRxCallback callback, void* context);
void rhs_hal_serial_async_rx_dma_start(RHSHalSerialId id, uint8_t* buffer, uint16_t buffer_size);
