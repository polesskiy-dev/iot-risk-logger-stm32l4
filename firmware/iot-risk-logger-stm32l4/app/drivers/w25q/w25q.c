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

/**
 * @brief Fast 4 lines read data from the flash
 *
 * @note The entire memory can be accessed with a single instruction as long as the clock continues
 * @note The Quad Enable bit (QE) of Status Register-2 must be set, it's factory default value is 1
 *
 * @param {W25Q_HandleTypeDef} hflash [in]
 * @param dataBuffer [out] buffer to store the data
 * @param address [in]
 * @param size [in]
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_ReadData(W25Q_HandleTypeDef *hflash, uint8_t *const dataBuffer, uint32_t address, size_t size) {
  QSPI_CommandTypeDef sCommand = {};

  HAL_StatusTypeDef status = HAL_OK;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_FAST_READ;

  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = address;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 6;
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

/**
 * @brief Write data to the flash page
 *
 * @description The Page Program instruction allows from 1 to 256 bytes of data to be programmed into the memory
 *
 * @note Single-line mode was selected due to the datasheet note about the lack of feasible performance improvement in the 4-line mode
 *
 * @param {W25Q_HandleTypeDef} hflash [in]
 * @param dataBuffer [in] buffer to write the data from
 * @param address [in]
 * @param size [in]
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_WritePageData(W25Q_HandleTypeDef *hflash, const uint8_t *dataBuffer, uint32_t address, size_t size) {
  QSPI_CommandTypeDef sCommand = {};

  HAL_StatusTypeDef status = HAL_OK;

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

  status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  return status;
}

/**
 * @brief Write any amount of data to the flash
 *
 * @description Uses W25Q_WritePageData to write the data page by page, address should be aligned to the page start
 *
 * @warning Place to write should be erased (0xFF) before writing
 *
 * @param {W25Q_HandleTypeDef} hflash [in]
 * @param dataBuffer [in] buffer to write the data from
 * @param address [in]
 * @param size [in]
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_WriteData(W25Q_HandleTypeDef *hflash, const uint8_t *dataBuffer, uint32_t address, size_t size) {
  HAL_StatusTypeDef status = HAL_OK;

  size_t pageSize = hflash->geometry.pageSize;

  // Write the data page by page
  for (size_t addressOffset = 0; addressOffset < size; addressOffset += pageSize) {
    status = W25Q_WritePageData(hflash, &dataBuffer[addressOffset], address + addressOffset, pageSize);

    if (status != HAL_OK)
      return status;
  }

  return status;
}

/**
 * @brief Erase a single 4KB sector
 *
 * @description The Sector Erase instruction sets all memory within a specified sector (4KB) to the erased state of all 1s (FFh)
 *
 * @param {W25Q_HandleTypeDef} hflash [in]
 * @param address [in] - address in memory, it wil be aligned to the sector start address internally
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_EraseSector(W25Q_HandleTypeDef *hflash, uint32_t address) {
  QSPI_CommandTypeDef sCommand = {};
  HAL_StatusTypeDef status = HAL_OK;

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

  sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  return status;
}

/**
 * @brief Erase the entire chip
 *
 * @description The Chip Erase instruction sets all memory within the device to the erased state of all 1s (FFh)
 *
 * @param {W25Q_HandleTypeDef} hflash [in]
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_EraseChip(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand = {};
  HAL_StatusTypeDef status = HAL_OK;

  status = W25Q_EnableWright(hflash);
  if (status != HAL_OK)
    return status;

  // Set up the QSPI command
  sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction = W25Q_CMD_CHIP_ERASE;

  sCommand.AddressMode = QSPI_ADDRESS_NONE;

  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode = QSPI_DATA_NONE;

  sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status != HAL_OK)
    return status;

  // wait for the busy flag to be cleared
  status = W25Q_WaitBusy(hflash);
  if (status != HAL_OK)
    return status;

  return status;
}

HAL_StatusTypeDef W25Q_Sleep(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand = {};
  HAL_StatusTypeDef status = HAL_OK;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_POWER_DOWN;

  sCommand.AddressMode       = QSPI_ADDRESS_NONE;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.AlternateBytes     = QSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;

  sCommand.DataMode          = QSPI_DATA_NONE;

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
 * @brief Wake up the W25Q device from the sleep mode
 *
 * @param hflash [in]
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_WakeUp(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand = {};

  HAL_StatusTypeDef status = HAL_OK;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_RELEASE_POWER_DOWN;

  sCommand.AddressMode       = QSPI_ADDRESS_NONE;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode = QSPI_DATA_NONE;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT);
  if (status != HAL_OK)
    return status;

  return status;
}

/**
 * @brief Read the W25Q Manufacturer & Device ID
 *
 * The instruction is initiated by driving the /CS pin low and shifting the instruction code “94h” followed by a
 * four clock dummy cycles and then a 24-bit address (A23-A0) of 000000h, but with the capability to input the
 * Address bits four bits per clock. After which, the Manufacturer ID for Winbond (EFh) and the Device ID (0x16) are shifted out
 *
 * @param hflash [in]
 * @param ID [out]
 *
 * @return {HAL_StatusTypeDef} execution status
 */
HAL_StatusTypeDef W25Q_ReadID(W25Q_HandleTypeDef *hflash, uint8_t ID[W25Q_ID_SIZE]) {
  QSPI_CommandTypeDef sCommand = {};

  HAL_StatusTypeDef status = HAL_OK;

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
  QSPI_CommandTypeDef sCommand = {};
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

/**
 * @brief Check if the flash is busy
 *
 * @description BUSY is a read only bit in the status register (S0) that is set to a 1 state when the device is executing a
 * *Page Program, Quad Page Program, Sector Erase, Block Erase, Chip Erase, Write Status Register or
 * Erase/Program Security Register* instruction. During this time the device will ignore further instructions
 * except for the Read Status Register and Erase/Program Suspend instruction
 *
 * @param {W25Q_HandleTypeDef} hflash [in]
 * @return {HAL_StatusTypeDef} execution status
 */
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
  QSPI_CommandTypeDef sCommand = {};

  HAL_StatusTypeDef status = HAL_OK;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_WRITE_ENABLE;

  sCommand.AddressMode       = QSPI_ADDRESS_NONE;

  sCommand.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;

  sCommand.DataMode          = QSPI_DATA_NONE;

  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the enable wright command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

  if (status != HAL_OK)
    return status;

  return status;
}

static HAL_StatusTypeDef W25Q_WaitBusy(W25Q_HandleTypeDef *hflash) {
  hflash->busyWaitCycles = FLASH_BUSY_WAIT_CYCLES; // refresh the counter
  HAL_StatusTypeDef status = HAL_OK;

  do {
    status = W25Q_isBusy(hflash);
  } while (status == HAL_BUSY && hflash->busyWaitCycles-- > NO_FLASH_BUSY_WAIT_CYCLES_LEFT);

  if (hflash->busyWaitCycles <= NO_FLASH_BUSY_WAIT_CYCLES_LEFT)
    return HAL_TIMEOUT;

  return HAL_OK;
}