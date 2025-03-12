#pragma once
#include "stdint.h"

/**
 * @brief init flash
 * @return 0 - if success
 */
int rhs_hal_flash_ex_init(void);

/**
 * @brief read data
 * @param addr
 * @param p_data
 * @param size
 * @return 0 - if success
 */
int rhs_hal_flash_ex_read(uint32_t addr, uint8_t* p_data, uint32_t size);

/**
 * @brief write data
 * @param addr
 * @param p_data
 * @param size
 * @return 0 - if success
 */
int rhs_hal_flash_ex_write(uint32_t addr, uint8_t* p_data, uint32_t size);

/**
 * @brief erase full flash
 * @return 0 - if success
 */
int rhs_hal_flash_ex_erase_chip(void);

/**
 * @brief erase block flash
 * @param addr
 * @param size
 * @return 0 - if success
 */
int rhs_hal_flash_ex_block_erase(uint32_t addr, uint32_t size);
