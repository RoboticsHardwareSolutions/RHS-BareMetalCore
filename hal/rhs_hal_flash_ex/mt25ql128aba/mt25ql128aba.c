#include "mt25ql128aba.h"
#include "quadspi.h"

/**
 * @brief  Flash reset enable command
 *         SPI/QPI; 1-0-0, 4-0-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int mt25ql128aba_ResetEnable(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the reset enable command */
    s_command.InstructionMode   = (Mode == MT25QL128ABA_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_RESET_ENABLE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Flash reset memory command
 *         SPI/QPI; 1-0-0, 4-0-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int mt25ql128aba_ResetMemory(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the reset enable command */
    s_command.InstructionMode   = (Mode == MT25QL128ABA_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_RESET_MEMORY;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  This function set the QSPI memory in 4-byte address mode
 *          SPI/QPI; 1-0-1/4-0-4
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int mt25ql128aba_Enter4BytesAddressMode(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the command */
    s_command.InstructionMode   = (Mode == MT25QL128ABA_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_ENTER_4_BYTE_ADDR_MODE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /*write enable */
    if (mt25ql128aba_write_enable(Ctx, Mode) != 0)
    {
        return -1;
    }
    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    /* Configure automatic polling mode to wait the memory is ready */
    else if (mt25ql128aba_auto_polling_mem_ready(Ctx, Mode, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  This function configure the dummy cycles on memory side.
 *         Dummy cycle bit locate in Configuration Register[7:6]
 * @param  Instance  QSPI instance
 * @retval BSP status
 */
static int QSPI_DummyCyclesCfg(mt25ql128aba_interface_t Mode)
{
    int                 ret = HAL_OK;
    QSPI_CommandTypeDef s_command;
    uint16_t            reg = 0;

    /* Initialize the read volatile configuration register command */
    s_command.InstructionMode   = (Mode == MT25QL128ABA_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_READ_VOLATILE_CONFIGURATION_REGISTER;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = (Mode == MT25QL128ABA_QPI_MODE) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 2;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, (uint8_t*) (&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Enable write operations */
    if (mt25ql128aba_write_enable(&hqspi, Mode) != 0)
    {
        return HAL_ERROR;
    }

    /* Update volatile configuration register (with new dummy cycles) */
    s_command.Instruction = MT25QL128ABA_WRITE_VOLATILE_CONFIGURATION_REGISTER;
    MODIFY_REG(reg, 0xF0F0, ((MT25QL128ABA_DUMMY_CYCLES_READ_QUAD << 4) | (MT25QL128ABA_DUMMY_CYCLES_READ_QUAD << 12)));

    /* Configure the write volatile configuration register command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(&hqspi, (uint8_t*) (&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Return BSP status */
    return ret;
}

/**
 * @brief  This function put QSPI memory in QPI mode (Quad I/O) from SPI mode.
 *         SPI -> QPI; 1-x-x -> 4-4-4
 *         SPI; 1-0-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int MT25QL128ABA_EnterQPIMode(QSPI_HandleTypeDef* Ctx)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_ENTER_QUAD_IO_MODE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Set Flash to desired Interface mode. And this instance becomes current instance.
 *         If current instance running at MMP mode then this function isn't work.
 *         Indirect -> Indirect
 * @param  Instance  QSPI instance
 * @param  Mode      QSPI mode
 * @param  Rate      QSPI transfer rate
 * @retval BSP status
 */

/**
 * @brief  This function put QSPI memory in SPI mode (Single I/O) from QPI mode.
 *         QPI -> SPI; 4-4-4 -> 1-x-x
 *         QPI; 4-0-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int MT25QL128ABA_ExitQPIMode(QSPI_HandleTypeDef* Ctx)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_RESET_QUAD_IO_MODE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

int QSPI_ConfigFlash(uint32_t Instance, mt25ql128aba_interface_t Mode, mt25ql128aba_transfer_t Rate)
{
    int32_t ret = HAL_OK;

    /* Setup MCU transfer rate setting ***************************************************/
    hqspi.Init.SampleShifting =
        (Rate == MT25QL128ABA_STR_TRANSFER) ? QSPI_SAMPLE_SHIFTING_HALFCYCLE : QSPI_SAMPLE_SHIFTING_NONE;

    if (Mode != MT25QL128ABA_QPI_MODE)
    {
        if (MT25QL128ABA_ExitQPIMode(&hqspi) != 0)
        {
            return -1;
        }
    }

    /* Return BSP status */
    return 0;
}

int mt25ql128aba_init(QSPI_HandleTypeDef* Ctx)
{ /* QSPI memory reset */
    /* Send RESET ENABLE command in QPI mode (QUAD I/Os, 4-4-4) */
    if (mt25ql128aba_ResetEnable(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        return -1;
    } /* Send RESET memory command in QPI mode (QUAD I/Os, 4-4-4) */
    else if (mt25ql128aba_ResetMemory(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        return -1;
    } /* Wait Flash ready */
    else if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, MT25QL128ABA_RESET_MAX_TIME) != 0)
    {
        return -1;
    } /* Send RESET ENABLE command in SPI mode (1-1-1) */
    //    else if(MT25TL01G_ResetEnable(&hqspi, QSPI_SPI_MODE) != MT25TL01G_OK)
    //    {
    //    return -1;
    //    }/* Send RESET memory command in SPI mode (1-1-1) */
    //    else if(MT25TL01G_ResetMemory(&hqspi, QSPI_SPI_MODE) != MT25TL01G_OK)
    //    {
    //    return -1;
    //    }
    /* Force Flash enter 4 Byte address mode */
    else if (mt25ql128aba_Enter4BytesAddressMode(&hqspi, MT25QL128ABA_QPI_MODE) != 0)
    {
        return -1;
    }

    /* Configuration of the dummy cycles on QSPI memory side */
    else if (mt25ql128aba_auto_polling_mem_ready(&hqspi, MT25QL128ABA_QPI_MODE, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != 0)
    {
        return -1;
    }
    else if (QSPI_DummyCyclesCfg(MT25QL128ABA_QPI_MODE) != HAL_OK)
    {
        return -1;
    }
    else
    {
        /* Configure Flash to desired mode */
        if (QSPI_ConfigFlash(MT25QL128ABA_QPI_MODE, MT25QL128ABA_QPI_MODE, MT25QL128ABA_STR_TRANSFER) != HAL_OK)
        {
            return -1;
        }
    }
    return 0;
}

int mt25ql128aba_read(QSPI_HandleTypeDef*      Ctx,
                      mt25ql128aba_interface_t Mode,
                      uint8_t*                 pData,
                      uint32_t                 ReadAddr,
                      uint32_t                 Size)
{
    QSPI_CommandTypeDef s_command;
    switch (Mode)
    {
    case MT25QL128ABA_SPI_MODE: /* 1-1-1 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_FAST_READ;
        s_command.AddressMode     = QSPI_ADDRESS_1_LINE;
        s_command.DataMode        = QSPI_DATA_1_LINE;
        break;
    case MT25QL128ABA_SPI_2IO_MODE: /* 1-2-2 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_DUAL_IO_FAST_READ;
        s_command.AddressMode     = QSPI_ADDRESS_2_LINES;
        s_command.DataMode        = QSPI_DATA_2_LINES;
        break;
    case MT25QL128ABA_SPI_4IO_MODE: /* 1-4-4 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_QUAD_IO_FAST_READ;
        s_command.AddressMode     = QSPI_ADDRESS_4_LINES;
        s_command.DataMode        = QSPI_DATA_4_LINES;
        break;
    case MT25QL128ABA_QPI_MODE: /* 1-1-4 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_QUAD_OUTPUT_FAST_READ;
        s_command.AddressMode     = QSPI_ADDRESS_1_LINE;
        s_command.DataMode        = QSPI_DATA_4_LINES;
        break;
    }
    /* Initialize the read command */
    s_command.DummyCycles       = MT25QL128ABA_DUMMY_CYCLES_READ;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = ReadAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.NbData            = Size;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  This function send a Write Enable and wait it is effective.
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int mt25ql128aba_write_enable(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Enable write operations */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;

    s_command.Instruction       = MT25QL128ABA_WRITE_ENABLE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    /* Configure automatic polling mode to wait for write enabling */
    s_config.Match           = (MT25QL128ABA_SR_WREN << 0x08) | MT25QL128ABA_SR_WREN;
    s_config.Mask            = (MT25QL128ABA_SR_WREN << 0x08) | MT25QL128ABA_SR_WREN;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 2;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction = MT25QL128ABA_READ_STATUS_REGISTER;
    s_command.DataMode    = QSPI_DATA_1_LINE;

    if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  This function reset the (WEL) Write Enable Latch bit.
 *         SPI/QPI; 1-0-0/4-0-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int mt25ql128aba_write_disable(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode)
{
    QSPI_CommandTypeDef s_command;
    /* Enable write operations */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_WRITE_DISABLE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief  Writes an amount of data to the QSPI memory.
 *         SPI/QPI; 1-1-1/1-2-2/1-4-4/4-4-4
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @param  pData Pointer to data to be written
 * @param  WriteAddr Write start address
 * @param  Size Size of data to write. Range 1 ~ 256
 * @retval QSPI memory status
 */

int mt25ql128aba_page_program(QSPI_HandleTypeDef*      Ctx,
                              mt25ql128aba_interface_t Mode,
                              uint8_t*                 pData,
                              uint32_t                 WriteAddr,
                              uint32_t                 Size)
{
    QSPI_CommandTypeDef s_command;
    switch (Mode)
    {
    case MT25QL128ABA_SPI_MODE: /* 1-1-1 commands, Power on H/W default setting */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_PAGE_PROGRAM;
        s_command.AddressMode     = QSPI_ADDRESS_1_LINE;
        s_command.DataMode        = QSPI_DATA_1_LINE;
        break;

    case MT25QL128ABA_SPI_2IO_MODE: /*  1-2-2 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_EXTENDED_DUAL_INPUT_FAST_PROGRAM;
        s_command.AddressMode     = QSPI_ADDRESS_2_LINES;
        s_command.DataMode        = QSPI_DATA_2_LINES;
        break;

    case MT25QL128ABA_SPI_4IO_MODE: /* 1-4-4 program commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_EXTENDED_QUAD_INPUT_FAST_PROGRAM;
        s_command.AddressMode     = QSPI_ADDRESS_4_LINES;
        s_command.DataMode        = QSPI_DATA_4_LINES;
        break;

    case MT25QL128ABA_QPI_MODE: /* 1-1-4 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction     = MT25QL128ABA_QUAD_INPUT_FAST_PROGRAM;
        s_command.AddressMode     = QSPI_ADDRESS_1_LINE;
        s_command.DataMode        = QSPI_DATA_4_LINES;
        break;
    }

    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = WriteAddr;
    s_command.NbData            = Size;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }
    if (HAL_QSPI_Transmit(Ctx, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief  Polling WIP(Write In Progress) bit become to 0
 *         SPI/QPI;4-0-4
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */
int mt25ql128aba_auto_polling_mem_ready(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode, uint32_t Timeout)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Configure automatic polling mode to wait for memory ready */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_READ_STATUS_REGISTER;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    s_config.Match           = 0;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;
    s_config.Mask            = MT25QL128ABA_SR_WIP | (MT25QL128ABA_SR_WIP << 8);
    s_config.StatusBytesSize = 2;

    if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, Timeout) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Whole chip erase.
 *         SPI/QPI; 1-0-0/4-0-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @retval QSPI memory status
 */

int mt25ql128aba_chip_erase(QSPI_HandleTypeDef* Ctx, mt25ql128aba_interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = MT25QL128ABA_BULK_ERASE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief  Erases the specified block of the QSPI memory.
 *         MT25QL128ABA support 4K, 32K, 64K size block erase commands.
 *         SPI/QPI; 1-1-0/4-4-0
 * @param  Ctx Component object pointer
 * @param  Mode Interface mode
 * @param  BlockAddress Block address to erase
 * @retval QSPI memory status
 */

int mt25ql128aba_block_erase(QSPI_HandleTypeDef*      Ctx,
                             mt25ql128aba_interface_t Mode,
                             uint32_t                 BlockAddress,
                             mt25ql128aba_erase_t     BlockSize)
{
    QSPI_CommandTypeDef s_command;
    switch (BlockSize)
    {
    case MT25QL128ABA_ERASE_4K:
        s_command.Instruction = MT25QL128ABA_4KB_SUBSECTOR_ERASE;
        break;

    case MT25QL128ABA_ERASE_32K:
        s_command.Instruction = MT25QL128ABA_32KB_SUBSECTOR_ERASE;
        break;

    case MT25QL128ABA_ERASE_SECTOR:
        s_command.Instruction = MT25QL128ABA_SECTOR_ERASE;
        break;

    case MT25QL128ABA_ERASE_BULK:
        return mt25ql128aba_chip_erase(Ctx, Mode);
    }
    /* Initialize the erase command */

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = BlockAddress;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return -1;
    }
    return 0;
}
