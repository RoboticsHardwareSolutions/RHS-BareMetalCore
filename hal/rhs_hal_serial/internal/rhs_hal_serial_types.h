#pragma once
#include "rserial.h"

#define RHS_HAL_SERIAL_RS485_DMA_INSTANCE (DMA1)
#define RHS_HAL_SERIAL_RS485_DMA_CHANNEL (LL_DMA_CHANNEL_7)

typedef struct
{
    uint8_t*                    buffer_rx_ptr;
    size_t                      buffer_rx_index_write;
    size_t                      buffer_rx_index_read;
    bool                        enabled;
    rserial                     rserial;
    RHSHalSerialAsyncRxCallback rx_byte_callback;
    RHSHalSerialAsyncTxCallback tx_byte_callback;
    RHSHalSerialDmaRxCallback   rx_dma_callback;
    RHSHalSerialDMATxCallback   tx_dma_callback;

    void* context;
} RHSHalSerial;
