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

HAL_StatusTypeDef W25Q_Init(W25Q_HandleTypeDef *hflash) {
  // Initialize the QSPI interface
  // Typically this will be already initialized in your HAL configuration
  // but you may want to configure or verify settings here if necessary

  // You can optionally perform a reset of the flash memory
  return W25Q_Reset(hflash);
}

HAL_StatusTypeDef W25Q_Reset(W25Q_HandleTypeDef *hflash) {
  uint8_t cmd[2] = { W25Q_CMD_RESET_ENABLE, W25Q_CMD_RESET_MEMORY };

  HAL_StatusTypeDef status = HAL_QSPI_Command(hflash->hqspi, &cmd[0], HAL_QPSI_TIMEOUT_DEFAULT);
  if (status != HAL_OK) {
    return status;
  }

  HAL_Delay(1);  // Ensure 30us wait time after reset command as per datasheet

  return HAL_QSPI_Command(hflash->hqspi, &cmd[1], HAL_QPSI_TIMEOUT_DEFAULT);
}

HAL_StatusTypeDef W25Q_ReadData(W25Q_HandleTypeDef *hflash, uint8_t *pData, uint32_t ReadAddr, uint32_t Size) {
  QSPI_CommandTypeDef sCommand;

  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_FAST_READ;
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = ReadAddr;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 8;
  sCommand.NbData            = Size;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send command
  if (HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT) != HAL_OK) {
    return HAL_ERROR;
  }

  // Receive data
  return HAL_QSPI_Receive(hflash->hqspi, pData, W25Q_TIMEOUT_DEFAULT);
}

HAL_StatusTypeDef W25Q_WriteData(W25Q_HandleTypeDef *hflash, const uint8_t *pData, uint32_t WriteAddr, uint32_t Size) {
  QSPI_CommandTypeDef sCommand;
  HAL_StatusTypeDef status;
  uint32_t currentAddr = WriteAddr;
  uint32_t currentSize = Size;
  uint32_t endAddr = WriteAddr + Size;

  while (currentAddr < endAddr) {
    // Calculate the current page size
    uint32_t pageSize = W25Q_PAGE_SIZE - (currentAddr % W25Q_PAGE_SIZE);
    if (currentSize < pageSize) {
      pageSize = currentSize;
    }

    // Enable write operations
    status = W25Q_WriteEnable(hflash);
    if (status != HAL_OK) {
      return status;
    }

    // Set up the QSPI command
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = W25Q_CMD_PAGE_PROGRAM;
    sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.Address           = currentAddr;
    sCommand.DataMode          = QSPI_DATA_1_LINE;
    sCommand.NbData            = pageSize;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    // Send the command
    status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT);
    if (status != HAL_OK) {
      return status;
    }

    // Transmit the data
    status = HAL_QSPI_Transmit(hflash->hqspi, (uint8_t *)pData, W25Q_TIMEOUT_DEFAULT);
    if (status != HAL_OK) {
      return status;
    }

    // Wait until the memory is no longer busy
    while (W25Q_GetStatus(hflash, &status) != HAL_OK) {
      HAL_Delay(1);
    }

    // Move to the next address
    currentAddr += pageSize;
    pData += pageSize;
    currentSize -= pageSize;
  }

  return HAL_OK;
}

HAL_StatusTypeDef W25Q_EraseSector(W25Q_HandleTypeDef *hflash, uint32_t SectorAddr) {
  QSPI_CommandTypeDef sCommand;
  HAL_StatusTypeDef status;

  // Enable write operations
  status = W25Q_WriteEnable(hflash);
  if (status != HAL_OK) {
    return status;
  }

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_SECTOR_ERASE;
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = SectorAddr;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_SECTOR_ERASE_TIMEOUT);
  if (status != HAL_OK) {
    return status;
  }

  // Wait until the memory is no longer busy
  while (W25Q_GetStatus(hflash, &status) != HAL_OK) {
    HAL_Delay(1);
  }

  return HAL_OK;
}

HAL_StatusTypeDef W25Q_EraseChip(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand;
  HAL_StatusTypeDef status;

  // Enable write operations
  status = W25Q_WriteEnable(hflash);
  if (status != HAL_OK) {
    return status;
  }

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_CHIP_ERASE;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AddressSize       = 0;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  status = HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_CHIP_ERASE_TIMEOUT);
  if (status != HAL_OK) {
    return status;
  }

  // Wait until the memory is no longer busy
  while (W25Q_GetStatus(hflash, &status) != HAL_OK) {
    HAL_Delay(1);
  }

  return HAL_OK;
}

HAL_StatusTypeDef W25Q_WriteEnable(W25Q_HandleTypeDef *hflash) {
  QSPI_CommandTypeDef sCommand;
  uint8_t reg;

  // Set up the QSPI command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q_CMD_WRITE_ENABLE;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  // Send the command
  if (HAL_QSPI_Command(hflash->hqspi, &sCommand, W25Q_TIMEOUT_DEFAULT) != HAL_OK) {
    return HAL_ERROR;
  }

  // Wait until the WEL bit is set
  do {
    if (W25Q_GetStatus(hflash, &reg) != HAL_OK) {
      return HAL_ERROR;
    }
  } while (!(reg & W25Q_SR_WEL));

  return HAL_OK;
}

//HAL_StatusTypeDef W25Q_GetStatus(W25Q_HandleTypeDef *hflash, uint8_t *Status) {
//  QSPI_CommandTypeDef sCommand;
//  uint8_t reg;
//
//  // Set up the QSPI command
//  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
//  sCommand.Instruction       = W25Q_CMD_READ_STATUS_REG;
//  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
//  sCommand.DataMode          = QSPI_DATA_1_LINE;
//  sCommand.NbData            = 1;
//  sCommand.DummyCycles       = 0;
//  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
//  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
//
//  // Send the command