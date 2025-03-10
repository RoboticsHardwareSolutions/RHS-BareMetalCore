#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_qspi.h"

extern QSPI_HandleTypeDef hqspi;


/** MT25QL128ABA Timing configuration **/
#define MT25QL128ABA_SECTOR_64K                   (uint32_t)(64 * 256)        /* 256 sectors of 64KBytes     */
#define MT25QL128ABA_SUBSECTOR_32K                (uint32_t)(32 * 256)        /* 512 subsectors of 32KBytes  */
#define MT25QL128ABA_SUBSECTOR_4K                 (uint32_t)(4  * 256)        /* 1024 subsectors of 4KBytes  */

#define MT25QL128ABA_FLASH_SIZE                   (uint32_t)(128*1024*1024/8)  /* 128 Mbits => 16MBytes        */
#define MT25QL128ABA_PAGE_SIZE                    (uint32_t)256                /* 65536 pages of 256 Bytes    */


/** MT25QL128ABA Timing configuration **/
#define MT25QL128ABA_BULK_ERASE_MAX_TIME          460000U
#define MT25QL128ABA_SECTOR_ERASE_MAX_TIME        1000U
#define MT25QL128ABA_SUBSECTOR_32K_ERASE_MAX_TIME 1000U
#define MT25QL128ABA_SUBSECTOR_4K_ERASE_MAX_TIME  400U
#define MT25QL128ABA_RESET_MAX_TIME               1000U                 /* when SWreset during erase operation */
#define MT25QL128ABA_AUTOPOLLING_INTERVAL_TIME    0x10U


/** MT25QL128ABA Error codes **/
#define MT25QL128ABA_OK                           (0)
#define MT25QL128ABA_ERROR                        (-1)

/*********************** Software RESET Operations **********************************/
#define MT25QL128ABA_RESET_ENABLE 0x66U
#define MT25QL128ABA_RESET_MEMORY 0x99U

/*********************** READ ID Operations **********************************/
#define MT25QL128ABA_READ_ID 0x9E // or 0x9FU
#define MT25QL128ABA_MULTIPLE_IO_READ_ID 0xAFU
#define MT25QL128ABA_READ_SERIAL_FLASH_DISCOVERY_PARAMETER 0x5AU

/*********************** READ MEMORY Operations **********************************/
#define MT25QL128ABA_READ           0x03U
#define MT25QL128ABA_FAST_READ           0x0BU
#define MT25QL128ABA_DUAL_OUTPUT_FAST_READ 0x3BU
#define MT25QL128ABA_DUAL_IO_FAST_READ 0xBBU
#define MT25QL128ABA_QUAD_OUTPUT_FAST_READ 0x6BU
#define MT25QL128ABA_QUAD_IO_FAST_READ 0xEBU
#define MT25QL128ABA_DTR_FAST_READ 0x0DU
#define MT25QL128ABA_DTR_DUAL_OUTPUT_FAST_READ 0x3DU
#define MT25QL128ABA_DTR_DUAL_IO_FAST_READ 0xBDU
#define MT25QL128ABA_DTR_QUAD_OUTPUT_FAST_READ 0x6DU
#define MT25QL128ABA_DTR_QUAD_IO_FAST_READ 0xEDU
#define MT25QL128ABA_QUAD_IO_WORD_READ 0xE7U

/*********************** WRITE Operations **********************************/
#define MT25QL128ABA_WRITE_ENABLE 0x06U
#define MT25QL128ABA_WRITE_DISABLE 0x04U

/*********************** READ REGISTER Operations **********************************/
#define MT25QL128ABA_READ_STATUS_REGISTER 0x05U
#define MT25QL128ABA_READ_FLAG_STATUS_REGISTER 0x70U
#define MT25QL128ABA_READ_NONVOLATILE_CONFIGURATION_REGISTER 0xB5U
#define MT25QL128ABA_READ_VOLATILE_CONFIGURATION_REGISTER 0x85U
#define MT25QL128ABA_READ_ENHANCED_VOLATILE_CONFIGURATION_REGISTER 0x65U
#define MT25QL128ABA_READ_GENERAL_PURPOSE_READ_REGISTER 0x96U

/*********************** WRITE REGISTER Operations **********************************/
#define MT25QL128ABA_WRITE_STATUS_REGISTER 0x01U
#define MT25QL128ABA_WRITE_NONVOLATILE_CONFIGURATION_REGISTER 0xB1U
#define MT25QL128ABA_WRITE_VOLATILE_CONFIGURATION_REGISTER 0x81U
#define MT25QL128ABA_WRITE_ENHANCED_VOLATILE_CONFIGURATION_REGISTER 0x61U

/*********************** CLEAR FLAG STATUS REGISTER Operation **********************************/
#define MT25QL128ABA_CLEAR_FLAG_STATUS_REGISTER 0x50U

/*********************** PROGRAM Operations **********************************/
#define MT25QL128ABA_PAGE_PROGRAM 0x02U
#define MT25QL128ABA_DUAL_INPUT_FAST_PROGRAM 0xA2U
#define MT25QL128ABA_EXTENDED_DUAL_INPUT_FAST_PROGRAM 0xD2U
#define MT25QL128ABA_QUAD_INPUT_FAST_PROGRAM 0x32U
#define MT25QL128ABA_EXTENDED_QUAD_INPUT_FAST_PROGRAM 0x38U

/*********************** ERASE Operations **********************************/
#define MT25QL128ABA_32KB_SUBSECTOR_ERASE 0x52U
#define MT25QL128ABA_4KB_SUBSECTOR_ERASE 0x20U
#define MT25QL128ABA_SECTOR_ERASE 0xD8U
#define MT25QL128ABA_BULK_ERASE 0xC7U // or 0x60U

/*********************** SUSPEND/RESUME Operations **********************************/
#define MT25QL128ABA_PROGRAM_OR_ERASE_SUSPEND 0x75U
#define MT25QL128ABA_PROGRAM_OR_ERASE_RESUME 0x7AU

/*********************** ONE-TIME PROGRAMMABLE (OTP) Operations **********************************/
#define MT25QL128ABA_READ_OTP_ARRAY 0x4BU
#define MT25QL128ABA_PROGRAM_OTP_ARRAY 0x42U

/*********************** QUAD PROTOCOL Operations **********************************/
#define MT25QL128ABA_ENTER_QUAD_IO_MODE 0x35U
#define MT25QL128ABA_RESET_QUAD_IO_MODE 0xF5U

/*********************** DEEP POWER-DOWN Operations **********************************/
#define MT25QL128ABA_ENTER_DEEP_POWER_DOWN 0xB9U
#define MT25QL128ABA_RELEASE_FROM_DEEP_POWERDOWN 0xABU

/*********************** ADVANCED SECTOR PROTECTION Operations **********************************/
#define MT25QL128ABA_READ_SECTOR_PROTECTION 0x2DU
#define MT25QL128ABA_PROGRAM_SECTOR_PROTECTION 0x2CU
#define MT25QL128ABA_READ_VOLATILE_LOCK_BITS 0xE8U
#define MT25QL128ABA_WRITE_VOLATILE_LOCK_BITS 0xE5U
#define MT25QL128ABA_READ_NONVOLATILE_LOCK_BITS 0xE2U
#define MT25QL128ABA_WRITE_NONVOLATILE_LOCK_BITS 0xE3U
#define MT25QL128ABA_ERASE_NONVOLATILE_LOCK_BITS 0xE4U
#define MT25QL128ABA_READ_GLOBAL_FREEZE_BIT 0xA7U
#define MT25QL128ABA_WRITE_GLOBAL_FREEZE_BIT 0xA6U
#define MT25QL128ABA_READ_PASSWORD 0x27U
#define MT25QL128ABA_WRITE_PASSWORD 0x28U
#define MT25QL128ABA_UNLOCK_PASSWORD 0x29U

/*********************** ADVANCED FUNCTION INTERFACE Operations **********************************/
#define MT25QL128ABA_INTERFACE_ACTIVATION 0x9BU
#define MT25QL128ABA_CYCLIC_REDUNDANCY_CHECK 0x9BU // or 0x27U

/*********************** Status Register **********************************/
#define MT25QL128ABA_SR_WIP                      ((uint8_t)0x01)    /*!< Write in progress */
#define MT25QL128ABA_SR_WREN                     ((uint8_t)0x02)    /*!< Write enable latch */
#define MT25QL128ABA_SR_BLOCKPR                  ((uint8_t)0x5C)    /*!< Block protected against program and erase operations */
#define MT25QL128ABA_SR_PRBOTTOM                 ((uint8_t)0x20)    /*!< Protected memory area defined by BLOCKPR starts from top or bottom */
#define MT25QL128ABA_SR_SRWREN                   ((uint8_t)0x80)    /*!< Status register write enable/disable */



/* Dummy cycles for STR read mode */
#define MT25QL128ABA_DUMMY_CYCLES_READ           8U
#define MT25QL128ABA_DUMMY_CYCLES_READ_QUAD      8U
/* Dummy cycles for DTR read mode */
#define MT25QL128ABA_DUMMY_CYCLES_READ_DTR       6U
#define MT25QL128ABA_DUMMY_CYCLES_READ_QUAD_DTR  8U




/***** 4-BYTE ADDRESS MODE Operations ****************************************/
#define MT25QL128ABA_ENTER_4_BYTE_ADDR_MODE_CMD           0xB7U   /*!< Enter 4-Byte mode (3/4 Byte address commands)           */
#define MT25QL128ABA_EXIT_4_BYTE_ADDR_MODE_CMD            0xE9U   /*!< Exit 4-Byte mode (3/4 Byte address commands)            */


/** @defgroup MT25QL128ABA_exported_eypes MT25QL128ABA Exported Types  */
typedef struct {
    uint32_t flash_size;                         /*!< Size of the flash                             */
    uint32_t erase_sector_size;                  /*!< Size of sectors for the erase operation       */
    uint32_t erase_sectors_number;               /*!< Number of sectors for the erase operation     */
    uint32_t erase_subsector_size;               /*!< Size of subsector for the erase operation     */
    uint32_t erase_subsector_number;             /*!< Number of subsector for the erase operation   */
    uint32_t erase_subsector1_size;              /*!< Size of subsector 1 for the erase operation   */
    uint32_t erase_subsector1_number;            /*!< Number of subsector 1 for the erase operation */
    uint32_t prog_page_size;                     /*!< Size of pages for the program operation       */
    uint32_t prog_pages_number;                  /*!< Number of pages for the program operation     */
} mt25ql128aba_info_t;

typedef enum {
    MT25QL128ABA_SPI_MODE = 0,                 /*!< 1-1-1 commands, Power on H/W default setting  */
    MT25QL128ABA_SPI_1I2O_MODE,                /*!< 1-1-2 commands                                */
    MT25QL128ABA_SPI_2IO_MODE,                 /*!< 1-2-2 commands                                */
    MT25QL128ABA_SPI_1I4O_MODE,                /*!< 1-1-4 commands                                */
    MT25QL128ABA_SPI_4IO_MODE,                 /*!< 1-4-4 commands                                */
    MT25QL128ABA_DPI_MODE,                     /*!< 2-2-2 commands                                */
    MT25QL128ABA_QPI_MODE                      /*!< 4-4-4 commands                                */
} mt25ql128aba_interface_t;

typedef enum {
    MT25QL128ABA_STR_TRANSFER = 0,             /*!< Single Transfer Rate                          */
    MT25QL128ABA_DTR_TRANSFER                  /*!< Double Transfer Rate                          */
} mt25ql128aba_transfer_t;

typedef enum {
    MT25QL128ABA_DUALFLASH_DISABLE = QSPI_DUALFLASH_DISABLE, /*!<  Single flash mode              */
    MT25QL128ABA_DUALFLASH_ENABLE = QSPI_DUALFLASH_ENABLE    /*!<  Dual flash mode                */
} mt25ql128aba_dual_flash_t;

typedef enum {
    MT25QL128ABA_ERASE_4K = 0,                 /*!< 4K size Sector erase                          */
    MT25QL128ABA_ERASE_32K,                    /*!< 32K size Block erase                          */
    MT25QL128ABA_ERASE_SECTOR,                 /*!< 64K size Block erase                          */
    MT25QL128ABA_ERASE_BULK                    /*!< Whole bulk erase                              */
} mt25ql128aba_erase_t;

typedef enum {
    MT25QL128ABA_BURST_READ_WRAP_16 = 0,       /*!< 16 bytes boundary aligned                     */
    MT25QL128ABA_BURST_READ_WRAP_32,           /*!< 32 bytes boundary aligned                     */
    MT25QL128ABA_BURST_READ_WRAP_64,           /*!< 64 bytes boundary aligned                     */
    MT25QL128ABA_BURST_READ_WRAP_NONE          /*!< Disable wrap function */
} mt25ql128aba_wrap_length_t;

typedef enum {
    MT25QL128ABA_EVCR_ODS_90 = 1,              /*!< Output driver strength 90 ohms                */
    MT25QL128ABA_EVCR_ODS_45 = 3,              /*!< Output driver strength 45 ohms                */
    MT25QL128ABA_EVCR_ODS_20 = 5,              /*!< Output driver strength 20 ohms                */
    MT25QL128ABA_EVCR_ODS_30 = 7               /*!< Output driver strength 30 ohms (default)      */
} mt25ql128aba_ODS_t;

typedef enum {
    MT25QL128ABA_3BYTES_SIZE = 0,              /*!< 3 Bytes address mode                          */
    MT25QL128ABA_4BYTES_SIZE                   /*!< 4 Bytes address mode                          */
} mt25ql128aba_address_size_t;


/**
 * @brief init memory
 * @param Ctx
 * @return 0 - if success
 */
int8_t mt25ql128aba_init(QSPI_HandleTypeDef *Ctx);

/**
 * @brief check memory is ready to write or erase commands
 * @param Ctx
 * @param Mode
 * @return 0 - if success
 */
int8_t mt25ql128aba_auto_polling_mem_ready(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode);

/**
 * @brief write enable command
 * @param Ctx
 * @param Mode
 * @return 0 - if success
 */
int8_t mt25ql128aba_write_enable(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode);

/**
 * @brief write disable command
 * @param Ctx
 * @param Mode
 * @return 0 - if success
 */
int8_t mt25ql128aba_write_disable(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode);

/**
 * @brief read memory
 * @param Ctx
 * @param Mode
 * @param pData
 * @param ReadAddr
 * @param Size
 * @return 0 - if success
 */
int8_t mt25ql128aba_read(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode, uint8_t *pData, uint32_t ReadAddr,
                         uint32_t Size);

/**
 * @brief program memory
 * @param Ctx
 * @param Mode
 * @param pData
 * @param WriteAddr
 * @param Size
 * @return 0 - if success
 */
int8_t mt25ql128aba_page_program(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode,
                                 uint8_t *pData, uint32_t WriteAddr, uint32_t Size);

/**
 * @brief erase memory
 * @param Ctx
 * @param Mode
 * @return 0 - if success
 */
int8_t mt25ql128aba_chip_erase(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode);

/**
 * @brief erase block memory
 * @param Ctx
 * @param Mode
 * @param BlockAddress
 * @param BlockSize
 * @return 0 - if success
 */
int8_t mt25ql128aba_block_erase(QSPI_HandleTypeDef *Ctx, mt25ql128aba_interface_t Mode, uint32_t BlockAddress,
                                mt25ql128aba_erase_t BlockSize);
