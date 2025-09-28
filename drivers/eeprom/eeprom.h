#pragma once

#include "rhs.h"
#include "rhs_hal_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EEPROM instance structure
 */
typedef struct
{
    const RHSHalI2cBusHandle* i2c_handle;      ///< I2C bus handle
    uint8_t                   i2c_address;     ///< I2C address of EEPROM (7-bit)
    uint32_t                  size;            ///< Total EEPROM size in bytes
    uint16_t                  page_size;       ///< Page size in bytes
    uint8_t                   address_size;    ///< Address size in bytes (1 or 2)
    uint8_t                   write_delay_ms;  ///< Write delay after page write (ms)
    uint32_t                  timeout_ms;      ///< I2C timeout in milliseconds
} EepromType;

/**
 * @brief EEPROM status codes
 */
typedef enum
{
    EepromStatusOk = 0,
    EepromStatusError,
    EepromStatusTimeout,
    EepromStatusInvalidParam,
    EepromStatusDeviceNotReady
} EepromStatus;

/**
 * @brief Initialize EEPROM instance (validates user-filled structure)
 *
 * @param eeprom Pointer to EEPROM instance with user-filled fields
 * @return EepromStatus
 */
EepromStatus eeprom_init(EepromType* eeprom);

/**
 * @brief Read data from EEPROM
 *
 * @param eeprom Pointer to EEPROM instance
 * @param address Memory address to read from
 * @param data Buffer to store read data
 * @param size Number of bytes to read
 * @return EepromStatus
 */
EepromStatus eeprom_read(const EepromType* eeprom, uint16_t address, uint8_t* data, size_t size);

/**
 * @brief Write data to EEPROM (max page size, must not cross page boundaries)
 *
 * @param eeprom Pointer to EEPROM instance
 * @param address Memory address to write to
 * @param data Data to write
 * @param size Number of bytes to write (must not exceed page size)
 * @return EepromStatus
 */
EepromStatus eeprom_write(const EepromType* eeprom, uint16_t address, const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif