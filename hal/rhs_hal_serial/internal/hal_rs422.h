#pragma once
#include "stdint.h"

void rhs_hal_rs422_init(void);

void rhs_hal_rs422_irq_callback(void* context);

void rhs_hal_rs422_tx_irq_callback(void* context);

void rhs_hal_rs422_async_tx_dma_start(const uint8_t* buffer, uint16_t buffer_size);

void rhs_hal_rs422_async_tx_dma_configure(void);
