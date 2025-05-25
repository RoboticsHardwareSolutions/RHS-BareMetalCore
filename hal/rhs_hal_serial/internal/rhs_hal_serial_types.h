#pragma once
#include "hard_defs.h"
#include "rserial.h"

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
