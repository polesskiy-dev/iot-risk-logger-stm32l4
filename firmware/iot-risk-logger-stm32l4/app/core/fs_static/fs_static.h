/*!
 * @file fs_static.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 29/08/2024
 * @author artempolisskyi
 */

#ifndef FS_STATIC_H
#define FS_STATIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define FAT12_SECTOR_SIZE       (512)
#define FAT12_SECTORS           (9)
#define FAT12_BOOT_SECTOR_SIZE  (FAT12_SECTOR_SIZE * FAT12_SECTORS)

#define SETTINGS_FILE_NAME      "settings.bin"
#define SETTINGS_FILE_SIZE      (0x1000) // 4KB - 1 erasable sector

#define INITIAL_LOG_START_ADDR                        (FAT12_BOOT_SECTOR_SIZE + SETTINGS_FILE_SIZE + 1)

#ifdef __cplusplus
}
#endif

#endif //FS_STATIC_H