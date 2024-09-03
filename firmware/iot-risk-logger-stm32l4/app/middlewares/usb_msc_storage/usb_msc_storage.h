/*!
 * @file usb_msc_storage.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 02/09/2024
 * @author artempolisskyi
 */

#ifndef USB_MSC_STORAGE_H
#define USB_MSC_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "w25q.h"

#define STORAGE_BLOCK_NUMBER                  (0x4000)  // 8MBytes (0x200 * 0x4000 === 0x800000)
#define STORAGE_BLOCK_SIZE                    (0x200)   // 512 bytes, standard FS block size

int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_IsReady(uint8_t lun);
int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size);

#ifdef __cplusplus
}
#endif

#endif //USB_MSC_STORAGE_H