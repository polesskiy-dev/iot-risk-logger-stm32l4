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
#include <stdbool.h>

#include "stm32l4xx_hal.h"

/* W25Q Commands */
#define W25Q_CMD_WRITE_ENABLE           (0x06)
#define W25Q_CMD_WRITE_DISABLE          0x04
#define W25Q_CMD_READ_STATUS_REG1       (0x05)
#define W25Q_CMD_WRITE_STATUS_REG1      (0x01)
#define W25Q_CMD_READ_DATA              0x03
#define W25Q_CMD_FAST_READ              (0xEB) ///< Fast Read Quad I/O
#define W25Q_CMD_PAGE_PROGRAM           (0x02)
#define W25Q_CMD_SECTOR_ERASE           (0x20)
#define W25Q_CMD_BLOCK_ERASE_32K        0x52
#define W25Q_CMD_BLOCK_ERASE_64K        0xD8
#define W25Q_CMD_CHIP_ERASE             0xC7
#define W25Q_CMD_POWER_DOWN             (0xB9)
#define W25Q_CMD_RELEASE_POWER_DOWN     0xAB
#define W25Q_CMD_READ_ID                (0x94) ///< Read MF ID and Dev ID
#define W25Q_CMD_READ_JEDEC_ID          0x9F
#define W25Q_CMD_RESET_ENABLE           0x66
#define W25Q_CMD_RESET_MEMORY           0x99

/* W25Q Status Register Bits */
#define W25Q_SR_BUSY                    0x01  /* Busy flag */
#define W25Q_SR_WEL                     0x02  /* Write enable latch */
#define W25Q_SR_BP0                     0x04  /* Block protect bit 0 */
#define W25Q_SR_BP1                     0x08  /* Block protect bit 1 */
#define W25Q_SR_BP2                     0x10  /* Block protect bit 2 */
#define W25Q_SR_TB                      0x20  /* Top/Bottom protect */
#define W25Q_SR_SEC                     0x40  /* Sector protect */
#define W25Q_SR_SRP0                    0x80  /* Status register protect 0 */

/* W25Q Timing Definitions */
#define W25Q_TIMEOUT_DEFAULT            1000   /* Default timeout in ms */
#define W25Q_PAGE_PROG_TIMEOUT          3      /* Page program timeout in ms */
#define W25Q_SECTOR_ERASE_TIMEOUT       300    /* Sector erase timeout in ms */
#define W25Q_BLOCK_ERASE_32K_TIMEOUT    1200   /* 32K block erase timeout in ms */
#define W25Q_BLOCK_ERASE_64K_TIMEOUT    2000   /* 64K block erase timeout in ms */
#define W25Q_CHIP_ERASE_TIMEOUT         10000  /* Chip erase timeout in ms */

#define W25Q_ID_SIZE                    (2)

#define FLASH_BUSY_WAIT_CYCLES          (5)        /* 5 ms */
#define NO_FLASH_BUSY_WAIT_CYCLES_LEFT  (0)        /* 5 ms */

/* W25Q Handle Structure */
typedef struct
{
  QSPI_HandleTypeDef *hqspi;      ///> Pointer to the QSPI handle
  uint8_t ChipSelectPin;          ///> GPIO pin for Chip Select
  GPIO_TypeDef *ChipSelectPort;   ///> GPIO port for Chip Select

  struct {
    uint32_t flashSize;           ///> Flash size in bytes
    uint32_t sectorSize;          ///> Sector size in bytes
    uint32_t subSectorSize;       ///> Subsector size in bytes
    uint32_t pageSize;            ///> Page size in bytes
    uint32_t blockSize32K;        ///> 32K block size in bytes
    uint32_t blockSize64K;        ///> 64K block size in bytes
  } geometry;

  union {
    uint8_t status1Reg;             ///> Status register 1, least significant bit (LSB) is stored first (S0)
    struct {
      uint8_t busy:1;               ///> Busy flag
      uint8_t wel:1;                ///> Write enable latch
      uint8_t bp0:1;                ///> Block protect bit 0
      uint8_t bp1:1;                ///> Block protect bit 1
      uint8_t bp2:1;                ///> Block protect bit 2
      uint8_t tb:1;                 ///> Top/Bottom protect
      uint8_t sec:1;                ///> Sector protect
      uint8_t srp:1;                ///> Status register protect
    } status1RegBits;               ///> Status register 1 bits
  } status;

  uint8_t busyWaitCycles;           ///> Number of cycles to wait for the memory to become not busy, error on depletion
} W25Q_HandleTypeDef;

/* Function Prototypes */
HAL_StatusTypeDef W25Q_Init(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_ReadData(W25Q_HandleTypeDef *hflash, uint8_t *dataBuffer, uint32_t address, uint32_t size);
HAL_StatusTypeDef W25Q_WriteData(W25Q_HandleTypeDef *hflash, const uint8_t *dataBuffer, uint32_t address, uint32_t size);
HAL_StatusTypeDef W25Q_EraseSector(W25Q_HandleTypeDef *hflash, uint32_t address);
HAL_StatusTypeDef W25Q_EraseChip(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_ReadStatusReg(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_ReadID(W25Q_HandleTypeDef *hflash, uint8_t ID[W25Q_ID_SIZE]);
HAL_StatusTypeDef W25Q_WriteEnable(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_Sleep(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_WakeUp(W25Q_HandleTypeDef *hflash);
HAL_StatusTypeDef W25Q_isBusy(W25Q_HandleTypeDef *hflash);

#ifdef __cplusplus
}
#endif

#endif //W25Q_H