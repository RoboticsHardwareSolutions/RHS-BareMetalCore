#pragma once
#include "stdint.h"

void rhs_hal_rs232_init(void);

void rhs_hal_rs232_irq_callback(void* context);

void rhs_hal_rs232_tx_irq_callback(void* context);

void rhs_hal_rs232_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size);

void rhs_hal_rs232_async_tx_dma_configure(void);