/*!
 * @file w25q.c
 * @brief implementation of w25q
 *
 * Detailed description of the implementation file.
 *
 * @date 19/08/2024
 * @author artempolisskyi
 */

#include "w25q.h"
#include "cmsis_os2.h"

static HAL_StatusTypeDef W25Q_WaitBusy(W25Q_HandleTypeDef *hflash);

HAL_StatusTypeDef W25Q_Init(W25Q_HandleTypeDef *hflash) {
  // TODO implement if needed

  return osOK;
}

/**
 * @brief
 *
 * @note size should be less than page size (256 bytes)
 *
 * @param hflash
 * @param dataBuffer [out]
 * @param address [in]
 * @param size [in]
 * @return
 */
HAL_StatusTypeDef W25Q_ReadData(W25Q_HandleTypeDef *hflash, uint8_t *const dataBuffer, uint32_t address, uint32_t size) {
  QSPI_CommandTypeDef sCommand;

  HAL_StatusTypeDef status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_FAST_READ;

  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = address;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 8;
  sCommand.NbData            = size;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // TODO check: The Quad Enable bit (QE) of Status Register-2 must be set to enable the Fast Read Quad I/O Instruction.

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT);
  if (status != HAL_OK)
    return status;

  // Receive the data
  status = HAL_QSPI_Receive(hflash->hqspi, dataBuffer, W25Q_TIMEOUT_DEFAULT);
  if (status != HAL_OK)
    return status;

  return status;
}

HAL_StatusTypeDef W25Q_WriteData(W25Q_HandleTypeDef *hflash, const uint8_t *dataBuffer, uint32_t address, uint32_t size) {
  QSPI_CommandTypeDef sCommand;

  HAL_StatusTypeDef status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  status = W25Q_EnableWright(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_PAGE_PROGRAM;

  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = address;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = size;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the page program command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  // Transmit the data
  status = HAL_QSPI_Transmit(hflash->hqspi, (uint8_t *)dataBuffer, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  return status;
}

HAL_StatusTypeDef W25Q_EraseSector(W25Q_HandleTypeDef *hflash, uint32_t address) {
  QSPI_CommandTypeDef sCommand;

  HAL_StatusTypeDef status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  status = W25Q_EnableWright(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction = W25Q_CMD_SECTOR_ERASE;

  sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
  sCommand.Address = address;

  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode = QSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.NbData = 0;

  sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  return status;
}

HAL_StatusTypeDef W25Q_EraseChip(W25Q_HandleTypeDef *hflash);

HAL_StatusTypeDef W25Q_Sleep(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand;

  HAL_StatusTypeDef status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_POWER_DOWN;

  sCommand.AddressMode       = QSPI_ADDRESS_NONE;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.AlternateBytes     = QSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;

  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 0;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT);
  if (status != HAL_OK)
    return status;

  return HAL_OK;
}

/**
 * @brief Read the W25Q device ID
 *
 * The instruction is initiated by driving the /CS pin low and shifting the instruction code “94h” followed by a
 * four clock dummy cycles and then a 24-bit address (A23-A0) of 000000h, but with the capability to input the
 * Address bits four bits per clock. After which, the Manufacturer ID for Winbond (EFh) and the Device ID (0x16) are shifted out
 *
 * @param hflash [in]
 * @param ID [out]
 */
HAL_StatusTypeDef W25Q_ReadID(W25Q_HandleTypeDef *hflash, uint8_t ID[W25Q_ID_SIZE]) {
  QSPI_CommandTypeDef sCommand;

  HAL_StatusTypeDef status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_READ_ID;

  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = 0x000000;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 6; // 2 cycles per byte
  sCommand.NbData            = W25Q_ID_SIZE;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT);
  if (status != HAL_OK)
    return status;

  // Receive the data
  status = HAL_QSPI_Receive(hflash->hqspi, ID, W25Q_TIMEOUT_DEFAULT);
  if (status != HAL_OK)
    return status;

  return status;
}

HAL_StatusTypeDef W25Q_ReadStatusReg(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand;
  HAL_StatusTypeDef status;

  // Initialize the QSPI command structure
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;   // Instruction sent on 1 line
  sCommand.Instruction       = W25Q_CMD_READ_STATUS_REG1; // Read status register 1

  sCommand.AddressMode       = QSPI_ADDRESS_NONE;        // No address needed for this command
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;// No alternate bytes
  sCommand.DataMode          = QSPI_DATA_1_LINE;         // Data received on 1 line
  sCommand.DummyCycles       = 0;                        // No dummy cycles needed
  sCommand.NbData            = 1;                        // We expect to receive 1 byte (the status register)

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;    // No DDR mode
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;// No DDR hold
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD; // Send instruction every time

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  // Receive the status register value
  status = HAL_QSPI_Receive(hflash->hqspi, &hflash->status.status1Reg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  return HAL_OK;
}

HAL_StatusTypeDef W25Q_isBusy(W25Q_HandleTypeDef *hflash) {
  // Read the status register
  if (W25Q_ReadStatusReg(hflash) != HAL_OK) {
    return HAL_ERROR;
  }

  // Check the busy flag
  if (hflash->status.status1RegBits.busy) {
    return HAL_BUSY;
  }

  return HAL_OK;
}

HAL_StatusTypeDef W25Q_EnableWright(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand;

  HAL_StatusTypeDef status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_WRITE_ENABLE;

  sCommand.AddressMode       = QSPI_ADDRESS_NONE;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 0;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the enable wright command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

  if (status != HAL_OK)
    return status;

  status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  return status;
}

static HAL_StatusTypeDef W25Q_WaitBusy(W25Q_HandleTypeDef *hflash) {
  hflash->busyWaitCycles = FLASH_BUSY_WAIT_CYCLES; // refresh the counter

  while (W25Q_isBusy(hflash) == HAL_BUSY && hflash->busyWaitCycles-- > NO_FLASH_BUSY_WAIT_CYCLES_LEFT)
    osDelay(1);

  if (hflash->busyWaitCycles <= NO_FLASH_BUSY_WAIT_CYCLES_LEFT)
    return HAL_TIMEOUT;

  return HAL_OK;
}