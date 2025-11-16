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
#include <stdint.h>

  // TODO [DFT-22](https://www.notion.so/sparkles-feat-MEMORY-static-filesystem-FAT16-header-and-local-addresses-table-2ac109abe356807fa00de8f06ab31c1c?v=2a7109abe35680779b2b000ca621d753&source=copy_link)

// FAT12 boot sector
#define FAT12_SECTOR_SIZE       (512)
#define FAT12_SECTORS           (9)
#define FAT12_BOOT_SECTOR_SIZE  (FAT12_SECTOR_SIZE * FAT12_SECTORS)

// Settings file
#define SETTINGS_FILE_NAME      "settings.bin"
#define SETTINGS_DATA_SIZE      (0x80)    // 256B, settings data size is page size
#define SETTINGS_FILE_SIZE      (0x1000)  // 4KB - 1 erasable sector
#define SETTINGS_FILE_ADDR      (FAT12_BOOT_SECTOR_SIZE + 1)

// Log file
#define INITIAL_LOG_START_ADDR  (FAT12_BOOT_SECTOR_SIZE + SETTINGS_FILE_SIZE + 1)

#ifdef __cplusplus
}
#endif

#endif //FS_STATIC_H