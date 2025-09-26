#include <rhs_hal_i2c.h>
#include "rhs.h"
#include "rhs_hal_cortex.h"

#if defined(STM32F103xE)
#    include <stm32f1xx_ll_gpio.h>
#elif defined(STM32F407xx)
#    include <stm32f4xx_ll_gpio.h>
#elif defined(STM32F765xx)
#    include <stm32f7xx_ll_gpio.h>
#else
#    error "Unsupported platform"
#endif

#define TAG "rhs_hal_i2c"

void rhs_hal_i2c_init(RHSHalI2cBus* bus)
{
    bus->callback(bus, RHSHalI2cBusEventInit);
}

void rhs_hal_i2c_deinit(RHSHalI2cBus* bus)
{
    bus->callback(bus, RHSHalI2cBusEventDeinit);
}

void rhs_hal_i2c_acquire(const RHSHalI2cBusHandle* handle)
{
    // Lock bus access
    handle->bus->callback(handle->bus, RHSHalI2cBusEventLock);
    // Ensure that no active handle set
    rhs_assert(handle->bus->current_handle == NULL);
    // Set current handle
    handle->bus->current_handle = handle;
    // Activate bus
    handle->bus->callback(handle->bus, RHSHalI2cBusEventActivate);
    // Activate handle
    handle->callback(handle, RHSHalI2cBusHandleEventActivate);
}

void rhs_hal_i2c_release(const RHSHalI2cBusHandle* handle)
{
    // Ensure that current handle is our handle
    rhs_assert(handle->bus->current_handle == handle);
    // Deactivate handle
    handle->callback(handle, RHSHalI2cBusHandleEventDeactivate);
    // Deactivate bus
    handle->bus->callback(handle->bus, RHSHalI2cBusEventDeactivate);
    // Reset current handle
    handle->bus->current_handle = NULL;
    // Unlock bus
    handle->bus->callback(handle->bus, RHSHalI2cBusEventUnlock);
}

static bool rhs_hal_i2c_wait_for_idle(I2C_TypeDef* i2c, RHSHalI2cBegin begin, RHSHalCortexTimer timer)
{
    do
    {
        if (rhs_hal_cortex_timer_is_expired(timer))
        {
            return false;
        }
    } while (begin == RHSHalI2cBeginStart && LL_I2C_IsActiveFlag_BUSY(i2c));
    // Only check if the bus is busy if starting a new transaction, if not we already control the bus

    return true;
}

static bool rhs_hal_i2c_wait_for_end(I2C_TypeDef* i2c, RHSHalI2cEnd end, RHSHalCortexTimer timer)
{
#if defined(STM32F765xx)
    // STM32F7 has newer I2C architecture with ISR register
    bool     wait_for_stop = end == RHSHalI2cEndStop;
    uint32_t end_mask      = (wait_for_stop) ? I2C_ISR_STOPF : (I2C_ISR_TC | I2C_ISR_TCR);

    while ((i2c->ISR & end_mask) == 0)
    {
        if (rhs_hal_cortex_timer_is_expired(timer))
        {
            return false;
        }
    }
#else
    // STM32F1/F4 use similar I2C architecture with SR1/SR2 registers
    bool wait_for_stop = end == RHSHalI2cEndStop;

    if (wait_for_stop)
    {
        while (!LL_I2C_IsActiveFlag_STOP(i2c))
        {
            if (rhs_hal_cortex_timer_is_expired(timer))
            {
                return false;
            }
        }
    }
    else
    {
        // Wait for BTF (Byte Transfer Finished) for transfer complete
        while (!LL_I2C_IsActiveFlag_BTF(i2c))
        {
            if (rhs_hal_cortex_timer_is_expired(timer))
            {
                return false;
            }
        }
    }
#endif

    return true;
}

static uint32_t rhs_hal_i2c_get_start_signal(RHSHalI2cBegin begin, bool ten_bit_address, bool read)
{
#if defined(STM32F765xx)
    // STM32F7 original implementation
    switch (begin)
    {
    case RHSHalI2cBeginRestart:
        if (read)
        {
            return ten_bit_address ? LL_I2C_GENERATE_RESTART_10BIT_READ : LL_I2C_GENERATE_RESTART_7BIT_READ;
        }
        else
        {
            return ten_bit_address ? LL_I2C_GENERATE_RESTART_10BIT_WRITE : LL_I2C_GENERATE_RESTART_7BIT_WRITE;
        }
    case RHSHalI2cBeginResume:
        return LL_I2C_GENERATE_NOSTARTSTOP;
    case RHSHalI2cBeginStart:
    default:
        return read ? LL_I2C_GENERATE_START_READ : LL_I2C_GENERATE_START_WRITE;
    }
#else
    // STM32F1/F4 don't use these constants - handle start conditions manually
    // Return simple enum values that we can interpret later
    switch (begin)
    {
    case RHSHalI2cBeginRestart:
        return read ? 2 : 3;  // 2 = restart_read, 3 = restart_write
    case RHSHalI2cBeginResume:
        return 4;  // 4 = no_start_stop
    case RHSHalI2cBeginStart:
    default:
        return read ? 0 : 1;  // 0 = start_read, 1 = start_write
    }
#endif
}

static uint32_t rhs_hal_i2c_get_end_signal(RHSHalI2cEnd end)
{
#if defined(STM32F765xx)
    // STM32F7 original implementation
    switch (end)
    {
    case RHSHalI2cEndAwaitRestart:
        return LL_I2C_MODE_SOFTEND;
    case RHSHalI2cEndPause:
        return LL_I2C_MODE_RELOAD;
    case RHSHalI2cEndStop:
    default:
        return LL_I2C_MODE_AUTOEND;
    }
#else
    // STM32F1/F4 don't use these constants - return simple enum values
    switch (end)
    {
    case RHSHalI2cEndAwaitRestart:
        return 0;  // soft_end
    case RHSHalI2cEndPause:
        return 1;  // reload/pause
    case RHSHalI2cEndStop:
    default:
        return 2;  // auto_end/stop
    }
#endif
}

static bool rhs_hal_i2c_transfer_is_aborted(I2C_TypeDef* i2c)
{
#if defined(STM32F765xx)
    // STM32F7 original implementation
    return LL_I2C_IsActiveFlag_STOP(i2c) && !(LL_I2C_IsActiveFlag_TC(i2c) || LL_I2C_IsActiveFlag_TCR(i2c));
#else
    // STM32F1/F4: Check for STOP and ensure no successful completion flags
    return LL_I2C_IsActiveFlag_STOP(i2c) && !LL_I2C_IsActiveFlag_BTF(i2c);
#endif
}

static bool rhs_hal_i2c_transfer(I2C_TypeDef*      i2c,
                                 uint8_t*          data,
                                 uint32_t          size,
                                 RHSHalI2cEnd      end,
                                 bool              read,
                                 RHSHalCortexTimer timer)
{
    bool ret = true;

#if defined(STM32F765xx)
    // STM32F7 original implementation
    while (size > 0)
    {
        bool should_stop = rhs_hal_cortex_timer_is_expired(timer) || rhs_hal_i2c_transfer_is_aborted(i2c);

        // Modifying the data pointer's data is UB if read is true
        if (read && LL_I2C_IsActiveFlag_RXNE(i2c))
        {
            *data = LL_I2C_ReceiveData8(i2c);
            data++;
            size--;
        }
        else if (!read && LL_I2C_IsActiveFlag_TXIS(i2c))
        {
            LL_I2C_TransmitData8(i2c, *data);
            data++;
            size--;
        }

        // Exit on timeout or premature stop, probably caused by a nacked address or byte
        if (should_stop)
        {
            ret = size == 0;  // If the transfer was over, still a success
            break;
        }
    }

    if (ret)
    {
        ret = rhs_hal_i2c_wait_for_end(i2c, end, timer);
    }

    LL_I2C_ClearFlag_STOP(i2c);
#else
    // STM32F1/F4 implementation
    while (size > 0)
    {
        bool should_stop = rhs_hal_cortex_timer_is_expired(timer) || rhs_hal_i2c_transfer_is_aborted(i2c);

        if (read && LL_I2C_IsActiveFlag_RXNE(i2c))
        {
            *data = LL_I2C_ReceiveData8(i2c);
            data++;
            size--;
        }
        else if (!read && LL_I2C_IsActiveFlag_TXE(i2c))  // STM32F1/F4 use TXE instead of TXIS
        {
            LL_I2C_TransmitData8(i2c, *data);
            data++;
            size--;
        }

        // Exit on timeout or premature stop, probably caused by a nacked address or byte
        if (should_stop)
        {
            ret = size == 0;  // If the transfer was over, still a success
            break;
        }
    }

    if (ret)
    {
        ret = rhs_hal_i2c_wait_for_end(i2c, end, timer);
    }

    LL_I2C_ClearFlag_STOP(i2c);
#endif

    return ret;
}

static bool rhs_hal_i2c_transaction(I2C_TypeDef*      i2c,
                                    uint16_t          address,
                                    bool              ten_bit,
                                    uint8_t*          data,
                                    size_t            size,
                                    RHSHalI2cBegin    begin,
                                    RHSHalI2cEnd      end,
                                    bool              read,
                                    RHSHalCortexTimer timer)
{
#if defined(STM32F765xx)
    // STM32F7 original implementation
    uint32_t addr_size    = ten_bit ? LL_I2C_ADDRSLAVE_10BIT : LL_I2C_ADDRSLAVE_7BIT;
    uint32_t start_signal = rhs_hal_i2c_get_start_signal(begin, ten_bit, read);

    if (!rhs_hal_i2c_wait_for_idle(i2c, begin, timer))
    {
        return false;
    }

    do
    {
        uint8_t      transfer_size = size;
        RHSHalI2cEnd transfer_end  = end;

        if (size > 255)
        {
            transfer_size = 255;
            transfer_end  = RHSHalI2cEndPause;
        }

        uint32_t end_signal = rhs_hal_i2c_get_end_signal(transfer_end);

        LL_I2C_HandleTransfer(i2c, address, addr_size, transfer_size, end_signal, start_signal);

        if (!rhs_hal_i2c_transfer(i2c, data, transfer_size, transfer_end, read, timer))
        {
            return false;
        }

        size -= transfer_size;
        data += transfer_size;
        start_signal = LL_I2C_GENERATE_NOSTARTSTOP;
    } while (size > 0);

    return true;

#else
    // STM32F1/F4 implementation - manual I2C handling
    uint32_t start_signal = rhs_hal_i2c_get_start_signal(begin, ten_bit, read);

    if (!rhs_hal_i2c_wait_for_idle(i2c, begin, timer))
    {
        return false;
    }

    // Generate START condition if needed
    if (start_signal == 0 || start_signal == 1 || start_signal == 2 || start_signal == 3)
    {  // Any start type
        LL_I2C_GenerateStartCondition(i2c);

        // Wait for START condition to be sent
        while (!LL_I2C_IsActiveFlag_SB(i2c))
        {
            if (rhs_hal_cortex_timer_is_expired(timer))
            {
                return false;
            }
        }

        // Send address
        if (ten_bit)
        {
            // TODO: Implement 10-bit addressing if needed
            return false;  // Not implemented for now
        }
        else
        {
            // 7-bit addressing
            uint8_t addr_byte = (address << 1) | (read ? 1 : 0);
            LL_I2C_TransmitData8(i2c, addr_byte);
        }

        // Wait for address ACK
        while (!LL_I2C_IsActiveFlag_ADDR(i2c))
        {
            if (LL_I2C_IsActiveFlag_AF(i2c) || rhs_hal_cortex_timer_is_expired(timer))
            {
                LL_I2C_ClearFlag_AF(i2c);
                return false;
            }
        }

        // Clear ADDR flag by reading SR1 and SR2
        LL_I2C_ClearFlag_ADDR(i2c);
    }

    // Perform data transfer
    if (!rhs_hal_i2c_transfer(i2c, data, size, end, read, timer))
    {
        return false;
    }

    // Generate STOP condition if needed
    if (end == RHSHalI2cEndStop)
    {
        LL_I2C_GenerateStopCondition(i2c);
    }

    return true;
#endif
}

bool rhs_hal_i2c_rx_ext(const RHSHalI2cBusHandle* handle,
                        uint16_t                  address,
                        bool                      ten_bit,
                        uint8_t*                  data,
                        size_t                    size,
                        RHSHalI2cBegin            begin,
                        RHSHalI2cEnd              end,
                        uint32_t                  timeout)
{
    rhs_assert(handle->bus->current_handle == handle);

    RHSHalCortexTimer timer = rhs_hal_cortex_timer_get(timeout * 1000);

    return rhs_hal_i2c_transaction(handle->bus->i2c, address, ten_bit, data, size, begin, end, true, timer);
}

bool rhs_hal_i2c_tx_ext(const RHSHalI2cBusHandle* handle,
                        uint16_t                  address,
                        bool                      ten_bit,
                        const uint8_t*            data,
                        size_t                    size,
                        RHSHalI2cBegin            begin,
                        RHSHalI2cEnd              end,
                        uint32_t                  timeout)
{
    rhs_assert(handle->bus->current_handle == handle);

    RHSHalCortexTimer timer = rhs_hal_cortex_timer_get(timeout * 1000);

    return rhs_hal_i2c_transaction(handle->bus->i2c, address, ten_bit, (uint8_t*) data, size, begin, end, false, timer);
}

bool rhs_hal_i2c_tx(const RHSHalI2cBusHandle* handle,
                    uint8_t                   address,
                    const uint8_t*            data,
                    size_t                    size,
                    uint32_t                  timeout)
{
    rhs_assert(timeout > 0);

    return rhs_hal_i2c_tx_ext(handle, address, false, data, size, RHSHalI2cBeginStart, RHSHalI2cEndStop, timeout);
}

bool rhs_hal_i2c_rx(const RHSHalI2cBusHandle* handle, uint8_t address, uint8_t* data, size_t size, uint32_t timeout)
{
    rhs_assert(timeout > 0);

    return rhs_hal_i2c_rx_ext(handle, address, false, data, size, RHSHalI2cBeginStart, RHSHalI2cEndStop, timeout);
}

bool rhs_hal_i2c_trx(const RHSHalI2cBusHandle* handle,
                     uint8_t                   address,
                     const uint8_t*            tx_data,
                     size_t                    tx_size,
                     uint8_t*                  rx_data,
                     size_t                    rx_size,
                     uint32_t                  timeout)
{
    return rhs_hal_i2c_tx_ext(handle,
                              address,
                              false,
                              tx_data,
                              tx_size,
                              RHSHalI2cBeginStart,
                              RHSHalI2cEndStop,
                              timeout) &&
           rhs_hal_i2c_rx_ext(handle, address, false, rx_data, rx_size, RHSHalI2cBeginStart, RHSHalI2cEndStop, timeout);
}

bool rhs_hal_i2c_is_device_ready(const RHSHalI2cBusHandle* handle, uint8_t i2c_addr, uint32_t timeout)
{
    rhs_assert(handle);
    rhs_assert(handle->bus->current_handle == handle);
    rhs_assert(timeout > 0);

    bool              ret   = true;
    RHSHalCortexTimer timer = rhs_hal_cortex_timer_get(timeout * 1000);

    if (!rhs_hal_i2c_wait_for_idle(handle->bus->i2c, RHSHalI2cBeginStart, timer))
    {
        return false;
    }

#if defined(STM32F765xx)
    // STM32F7 original implementation
    LL_I2C_HandleTransfer(handle->bus->i2c,
                          i2c_addr,
                          LL_I2C_ADDRSLAVE_7BIT,
                          0,
                          LL_I2C_MODE_AUTOEND,
                          LL_I2C_GENERATE_START_WRITE);

    if (!rhs_hal_i2c_wait_for_end(handle->bus->i2c, RHSHalI2cEndStop, timer))
    {
        return false;
    }

    ret = !LL_I2C_IsActiveFlag_NACK(handle->bus->i2c);

    LL_I2C_ClearFlag_NACK(handle->bus->i2c);
    LL_I2C_ClearFlag_STOP(handle->bus->i2c);

#else
    // STM32F1/F4 implementation
    // Generate START condition
    LL_I2C_GenerateStartCondition(handle->bus->i2c);

    // Wait for START condition
    while (!LL_I2C_IsActiveFlag_SB(handle->bus->i2c))
    {
        if (rhs_hal_cortex_timer_is_expired(timer))
        {
            return false;
        }
    }

    // Send address for write (device ready check)
    uint8_t addr_byte = (i2c_addr << 1) | 0;  // Write operation
    LL_I2C_TransmitData8(handle->bus->i2c, addr_byte);

    // Wait for address ACK or NACK
    while (!LL_I2C_IsActiveFlag_ADDR(handle->bus->i2c) && !LL_I2C_IsActiveFlag_AF(handle->bus->i2c))
    {
        if (rhs_hal_cortex_timer_is_expired(timer))
        {
            LL_I2C_GenerateStopCondition(handle->bus->i2c);
            return false;
        }
    }

    // Check if we got ACK (device ready) or NACK (device not ready)
    ret = LL_I2C_IsActiveFlag_ADDR(handle->bus->i2c);

    // Clear flags and generate STOP
    if (ret)
    {
        LL_I2C_ClearFlag_ADDR(handle->bus->i2c);
    }
    else
    {
        LL_I2C_ClearFlag_AF(handle->bus->i2c);
    }

    LL_I2C_GenerateStopCondition(handle->bus->i2c);
    LL_I2C_ClearFlag_STOP(handle->bus->i2c);
#endif

    return ret;
}

bool rhs_hal_i2c_read_reg_8(const RHSHalI2cBusHandle* handle,
                            uint8_t                   i2c_addr,
                            uint8_t                   reg_addr,
                            uint8_t*                  data,
                            uint32_t                  timeout)
{
    rhs_assert(handle);

    return rhs_hal_i2c_trx(handle, i2c_addr, &reg_addr, 1, data, 1, timeout);
}

bool rhs_hal_i2c_read_reg_16(const RHSHalI2cBusHandle* handle,
                             uint8_t                   i2c_addr,
                             uint8_t                   reg_addr,
                             uint16_t*                 data,
                             uint32_t                  timeout)
{
    rhs_assert(handle);

    uint8_t reg_data[2];
    bool    ret = rhs_hal_i2c_trx(handle, i2c_addr, &reg_addr, 1, reg_data, 2, timeout);
    *data       = (reg_data[0] << 8) | (reg_data[1]);

    return ret;
}

bool rhs_hal_i2c_read_mem(const RHSHalI2cBusHandle* handle,
                          uint8_t                   i2c_addr,
                          uint8_t                   mem_addr,
                          uint8_t*                  data,
                          size_t                    len,
                          uint32_t                  timeout)
{
    rhs_assert(handle);

    return rhs_hal_i2c_trx(handle, i2c_addr, &mem_addr, 1, data, len, timeout);
}

bool rhs_hal_i2c_write_reg_8(const RHSHalI2cBusHandle* handle,
                             uint8_t                   i2c_addr,
                             uint8_t                   reg_addr,
                             uint8_t                   data,
                             uint32_t                  timeout)
{
    rhs_assert(handle);

    const uint8_t tx_data[2] = {
        reg_addr,
        data,
    };

    return rhs_hal_i2c_tx(handle, i2c_addr, tx_data, 2, timeout);
}

bool rhs_hal_i2c_write_reg_16(const RHSHalI2cBusHandle* handle,
                              uint8_t                   i2c_addr,
                              uint8_t                   reg_addr,
                              uint16_t                  data,
                              uint32_t                  timeout)
{
    rhs_assert(handle);

    const uint8_t tx_data[3] = {
        reg_addr,
        (data >> 8) & 0xFF,
        data & 0xFF,
    };

    return rhs_hal_i2c_tx(handle, i2c_addr, tx_data, 3, timeout);
}

bool rhs_hal_i2c_write_mem(const RHSHalI2cBusHandle* handle,
                           uint8_t                   i2c_addr,
                           uint8_t                   mem_addr,
                           const uint8_t*            data,
                           size_t                    len,
                           uint32_t                  timeout)
{
    rhs_assert(handle);
    rhs_assert(handle->bus->current_handle == handle);
    rhs_assert(timeout > 0);

    return rhs_hal_i2c_tx_ext(handle, i2c_addr, false, &mem_addr, 1, RHSHalI2cBeginStart, RHSHalI2cEndPause, timeout) &&
           rhs_hal_i2c_tx_ext(handle, i2c_addr, false, data, len, RHSHalI2cBeginResume, RHSHalI2cEndStop, timeout);
}
