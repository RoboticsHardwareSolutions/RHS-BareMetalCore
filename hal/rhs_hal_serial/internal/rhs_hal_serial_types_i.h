#pragma once
#include "rserial.h"

struct RHSHalSerial
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

    void* rx_context;
    void* tx_context;
};
