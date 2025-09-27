#include "eeprom.h"
#include "rhs.h"

#define TAG "eeprom"

EepromStatus eeprom_init(EepromType* eeprom)
{
    if (!eeprom)
    {
        return EepromStatusInvalidParam;
    }

    // Validate user-filled fields
    if (!eeprom->i2c_handle || eeprom->size == 0 || eeprom->page_size == 0 || eeprom->timeout_ms == 0)
    {
        return EepromStatusInvalidParam;
    }

    // Validate I2C address (7-bit address should be <= 0x7F)
    if (eeprom->i2c_address > 0x7F)
    {
        return EepromStatusInvalidParam;
    }

    return EepromStatusOk;
}

EepromStatus eeprom_read(const EepromType* eeprom, uint16_t address, uint8_t* data, size_t size)
{
    if (!eeprom || !data || size == 0)
    {
        return EepromStatusInvalidParam;
    }

    if (address + size > eeprom->size)
    {
        return EepromStatusInvalidParam;
    }

    // Convert address to big-endian format (MSB first)
    uint8_t addr_buf[2] = {(uint8_t) ((address >> 8) & 0xFF), (uint8_t) (address & 0xFF)};

    bool success = rhs_hal_i2c_trx(eeprom->i2c_handle,
                                   eeprom->i2c_address,
                                   addr_buf,
                                   sizeof(addr_buf),
                                   data,
                                   size,
                                   eeprom->timeout_ms);

    return success ? EepromStatusOk : EepromStatusError;
}

EepromStatus eeprom_write(const EepromType* eeprom, uint16_t address, const uint8_t* data, size_t size)
{
    if (!eeprom || !data || size == 0)
    {
        return EepromStatusInvalidParam;
    }

    // Check if size exceeds page size
    if (size > eeprom->page_size)
    {
        return EepromStatusInvalidParam;
    }

    // Check if address is within EEPROM range
    if (address + size > eeprom->size)
    {
        return EepromStatusInvalidParam;
    }

    // Check if write crosses page boundary
    uint16_t page_start = (address / eeprom->page_size) * eeprom->page_size;
    uint16_t page_end   = page_start + eeprom->page_size;
    if (address + size > page_end)
    {
        return EepromStatusInvalidParam;
    }

    // Prepare address buffer
    uint8_t addr_buf[2] = {(uint8_t) ((address >> 8) & 0xFF), (uint8_t) (address & 0xFF)};

    // Write address first, then data without STOP condition
    bool success = rhs_hal_i2c_tx_ext(eeprom->i2c_handle,
                                      eeprom->i2c_address,
                                      false,
                                      addr_buf,
                                      2,
                                      RHSHalI2cBeginStart,
                                      RHSHalI2cEndPause,
                                      eeprom->timeout_ms);

    if (success)
    {
        // Write data with STOP condition
        success = rhs_hal_i2c_tx_ext(eeprom->i2c_handle,
                                     eeprom->i2c_address,
                                     false,
                                     data,
                                     size,
                                     RHSHalI2cBeginResume,
                                     RHSHalI2cEndStop,
                                     eeprom->timeout_ms);
    }

    if (!success)
    {
        return EepromStatusError;
    }

    // Wait for write completion
    rhs_delay_ms(eeprom->write_delay_ms);

    return EepromStatusOk;
}
