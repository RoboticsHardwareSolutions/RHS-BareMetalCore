#pragma once
#include "stdint.h"

void rhs_hal_rs485_init(void);

void rhs_hal_rs485_rx_irq_callback(void* context);

void rhs_hal_rs485_tx_irq_callback(void* context);

void rhs_hal_rs485_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size);

void rhs_hal_rs485_async_tx_dma_stop(void);

void rhs_hal_rs485_async_tx_dma_configure(void);

void rhs_hal_rs485_async_rx_dma_configure(void);

void rhs_hal_rs485_async_rx_dma_start(const uint8_t* buffer, uint16_t buffer_size);

void rhs_hal_rs485_async_rx_dma_stop(void);
