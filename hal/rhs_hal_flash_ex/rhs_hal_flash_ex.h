#pragma once
#include "stdint.h"

/**
 * @brief Flash operation return codes
 */
#define RHS_FLASH_EX_OK              0   /**< Success */
#define RHS_FLASH_EX_ERROR          -1   /**< General error */
#define RHS_FLASH_EX_BUSY           -2   /**< Flash is busy (when called from ISR) */

/**
 * @brief init flash
 * @return RHS_FLASH_EX_OK if success, RHS_FLASH_EX_ERROR otherwise
 */
int rhs_hal_flash_ex_init(void);

/**
 * @brief read data
 * @param addr Flash address to read from
 * @param p_data Pointer to buffer to store data
 * @param size Number of bytes to read
 * @return RHS_FLASH_EX_OK if success, RHS_FLASH_EX_BUSY if called from ISR and mutex unavailable, RHS_FLASH_EX_ERROR otherwise
 */
int rhs_hal_flash_ex_read(uint32_t addr, uint8_t* p_data, uint32_t size);

/**
 * @brief write data
 * @param addr Flash address to write to
 * @param p_data Pointer to data to write
 * @param size Number of bytes to write
 * @return RHS_FLASH_EX_OK if success, RHS_FLASH_EX_BUSY if called from ISR and mutex unavailable, RHS_FLASH_EX_ERROR otherwise
 */
int rhs_hal_flash_ex_write(uint32_t addr, uint8_t* p_data, uint32_t size);

/**
 * @brief erase full flash
 * @return RHS_FLASH_EX_OK if success, RHS_FLASH_EX_BUSY if called from ISR and mutex unavailable, RHS_FLASH_EX_ERROR otherwise
 */
int rhs_hal_flash_ex_erase_chip(void);

/**
 * @brief erase block flash
 * @param addr Block address to start erasing from
 * @param size Size of area to erase
 * @return RHS_FLASH_EX_OK if success, RHS_FLASH_EX_BUSY if called from ISR and mutex unavailable, RHS_FLASH_EX_ERROR otherwise
 */
int rhs_hal_flash_ex_block_erase(uint32_t addr, uint32_t size);
