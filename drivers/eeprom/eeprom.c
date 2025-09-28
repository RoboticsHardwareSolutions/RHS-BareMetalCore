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

    // Validate address size (must be 1 or 2 bytes)
    if (eeprom->address_size != 1 && eeprom->address_size != 2)
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

    // Prepare address buffer according to address size
    uint8_t addr_buf[2];
    size_t addr_buf_size;
    
    if (eeprom->address_size == 1)
    {
        addr_buf[0] = (uint8_t)(address & 0xFF);
        addr_buf_size = 1;
    }
    else // address_size == 2
    {
        addr_buf[0] = (uint8_t)((address >> 8) & 0xFF);  // MSB first
        addr_buf[1] = (uint8_t)(address & 0xFF);
        addr_buf_size = 2;
    }

    bool success = rhs_hal_i2c_trx(eeprom->i2c_handle,
                                   eeprom->i2c_address,
                                   addr_buf,
                                   addr_buf_size,
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

    // Check if write would exceed EEPROM memory bounds
    if (address + size > eeprom->size)
    {
        return EepromStatusInvalidParam;
    }

    size_t remaining = size;
    uint16_t current_address = address;
    const uint8_t* current_data = data;

    while (remaining > 0)
    {
        // Calculate how much we can write in current page
        uint16_t page_start = (current_address / eeprom->page_size) * eeprom->page_size;
        uint16_t page_end = page_start + eeprom->page_size;
        uint16_t bytes_to_page_end = page_end - current_address;
        
        // Write only what fits in current page
        size_t bytes_to_write = (remaining < bytes_to_page_end) ? remaining : bytes_to_page_end;

        // Prepare address buffer according to address size
        uint8_t addr_buf[2];
        size_t addr_buf_size;
        
        if (eeprom->address_size == 1)
        {
            addr_buf[0] = (uint8_t)(current_address & 0xFF);
            addr_buf_size = 1;
        }
        else // address_size == 2
        {
            addr_buf[0] = (uint8_t)((current_address >> 8) & 0xFF);  // MSB first
            addr_buf[1] = (uint8_t)(current_address & 0xFF);
            addr_buf_size = 2;
        }

        // Write address first, then data without STOP condition
        bool success = rhs_hal_i2c_tx_ext(eeprom->i2c_handle,
                                          eeprom->i2c_address,
                                          false,
                                          addr_buf,
                                          addr_buf_size,
                                          RHSHalI2cBeginStart,
                                          RHSHalI2cEndPause,
                                          eeprom->timeout_ms);

        if (success)
        {
            // Write data with STOP condition
            success = rhs_hal_i2c_tx_ext(eeprom->i2c_handle,
                                         eeprom->i2c_address,
                                         false,
                                         current_data,
                                         bytes_to_write,
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

        // Update pointers and counters for next page
        remaining -= bytes_to_write;
        current_address += bytes_to_write;
        current_data += bytes_to_write;
    }

    return EepromStatusOk;
}
