/*!
 * @file w25q.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 19/08/2024
 * @author artempolisskyi
 */

#ifndef W25Q_H
#define W25Q_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
  
#include "stm32l4xx_hal.h"

/* W25Q Commands */
#define W25Q_CMD_WRITE_ENABLE        0x06
#define W25Q_CMD_WRITE_DISABLE       0x04
#define W25Q_CMD_READ_STATUS_REG     0x05
#define W25Q_CMD_WRITE_STATUS_REG    0x01
#define W25Q_CMD_READ_DATA           0x03
#define W25Q_CMD_FAST_READ           0x0B
#define W25Q_CMD_PAGE_PROGRAM        0x02
#define W25Q_CMD_SECTOR_ERASE        0x20
#define W25Q_CMD_BLOCK_ERASE_32K     0x52
#define W25Q_CMD_BLOCK_ERASE_64K     0xD8
#define W25Q_CMD_CHIP_ERASE          0xC7
#define W25Q_CMD_POWER_DOWN          0xB9
#define W25Q_CMD_RELEASE_POWER_DOWN  0xAB
#define W25Q_CMD_READ_ID             0x90
#define W25Q_CMD_READ_JEDEC_ID       0x9F
#define W25Q_CMD_RESET_ENABLE        0x66
#define W25Q_CMD_RESET_MEMORY        0x99

/* W25Q Status Register Bits */
#define W25Q_SR_BUSY                 0x01  /* Busy flag */
#define W25Q_SR_WEL                  0x02  /* Write enable latch */
#define W25Q_SR_BP0                  0x04  /* Block protect bit 0 */
#define W25Q_SR_BP1                  0x08  /* Block protect bit 1 */
#define W25Q_SR_BP2                  0x10  /* Block protect bit 2 */
#define W25Q_SR_TB                   0x20  /* Top/Bottom protect */
#define W25Q_SR_SEC                  0x40  /* Sector protect */
#define W25Q_SR_SRP0                 0x80  /* Status register protect 0 */

/* W25Q Timing Definitions */
#define W25Q_TIMEOUT_DEFAULT         1000   /* Default timeout in ms */
#define W25Q_PAGE_PROG_TIMEOUT       3      /* Page program timeout in ms */
#define W25Q_SECTOR_ERASE_TIMEOUT    300    /* Sector erase timeout in ms */
#define W25Q_BLOCK_ERASE_32K_TIMEOUT 1200   /* 32K block erase timeout in ms */
#define W25Q_BLOCK_ERASE_64K_TIMEOUT 2000   /* 64K block erase timeout in ms */
#define W25Q_CHIP_ERASE_TIMEOUT      10000  /* Chip erase timeout in ms */

/* W25Q Handle Structure */
typedef struct
{
  QSPI_HandleTypeDef *hqspi;  /* Pointer to the QSPI handle */
  uint8_t ChipSelectPin;       /* GPIO pin for Chip Select */
  GPIO_TypeDef *ChipSelectPort; /* GPIO port for Chip Select */
} W25Q_HandleTypeDef;

/* Function Prototypes */
HAL_StatusTypeDef W25Q_Init(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_Reset(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_ReadData(W25Q_HandleTypeDef *hflash, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
HAL_StatusTypeDef W25Q_WriteData(W25Q_HandleTypeDef *hflash, const uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
HAL_StatusTypeDef W25Q_EraseSector(W25Q_HandleTypeDef *hflash, uint32_t SectorAddr);
HAL_StatusTypeDef W25Q_EraseChip(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_GetStatus(W25Q_HandleTypeDef *hflash, uint8_t *Status);
HAL_StatusTypeDef W25Q_ReadID(W25Q_HandleTypeDef *hflash, uint8_t *ID);
HAL_StatusTypeDef W25Q_WriteEnable(W25Q_HandleTypeDef *hflash);

#ifdef __cplusplus
}
#endif

#endif //W25Q_H