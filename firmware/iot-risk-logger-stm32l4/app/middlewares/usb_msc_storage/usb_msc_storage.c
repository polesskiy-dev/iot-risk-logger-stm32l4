/*!
 * @file usb_msc_storage.c
 * @brief implementation of usb_msc_storage
 *
 * Detailed description of the implementation file.
 *
 * @date 02/09/2024
 * @author artempolisskyi
 */

#include "usb_msc_storage.h"

extern W25Q_HandleTypeDef MEMORY_W25QHandle;

int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
  uint8_t status = HAL_OK;
  // TODO
  return (status);
};

int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
  // Calculate the flash address
  uint32_t address = blk_addr * STORAGE_BLOCK_SIZE; // Convert block address to byte address
  uint8_t ioStatus = HAL_OK;

  // wake up the memory
  ioStatus = W25Q_WakeUp(&MEMORY_W25QHandle);

  for (uint16_t blockNumber = 0; blockNumber < blk_len; blockNumber++) {
    uint32_t bufferOffset = blockNumber * STORAGE_BLOCK_SIZE;
    ioStatus = ioStatus || W25Q_ReadData(&MEMORY_W25QHandle, &buf[bufferOffset], address, STORAGE_BLOCK_SIZE);
  }

  // put the memory to sleep, not very optimal but it significantly simplifies the flow. Power consumption is not a concern here due to USB powering.
  ioStatus = ioStatus || W25Q_Sleep(&MEMORY_W25QHandle);

  return (ioStatus);
}

int8_t STORAGE_IsReady(uint8_t lun) {
  uint8_t ioStatus = W25Q_isBusy(&MEMORY_W25QHandle);
  uint8_t isBusy = MEMORY_W25QHandle.status.status1RegBits.busy == 1;

  if (ioStatus == HAL_OK && !isBusy) {
    return (HAL_OK);
  }
  return (HAL_ERROR);
}

int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size) {
  *block_num  = STORAGE_BLOCK_NUMBER;
  *block_size = STORAGE_BLOCK_SIZE;
  return (HAL_OK);
}