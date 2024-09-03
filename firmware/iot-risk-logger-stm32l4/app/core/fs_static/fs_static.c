/*!
 * @file fs_static.c
 * @brief implementation of fs_static
 *
 * Detailed description of the implementation file.
 *
 * @date 29/08/2024
 * @author artempolisskyi
 */

#include "fs_static.h"

// Boot Sector (512 bytes)
const uint8_t boot_sector[512] = {
        0xEB, 0x3C, 0x90,              // Jump instruction
        'M', 'S', 'D', 'O', 'S', '5', '.', '0', // OEM Name
        0x00, 0x02,                    // Bytes per sector (512 bytes)
        0x08,                          // Sectors per cluster (4KB clusters)
        0x01, 0x00,                    // Reserved sectors (1 sector for the boot sector)
        0x02,                          // Number of FATs (2 FATs)
        0x00, 0x20,                    // Max root directory entries (512 entries)
        0x00, 0x00,                    // Total sectors (small volume) (not used)
        0xF8,                          // Media descriptor (Fixed disk)
        0x08, 0x00,                    // Sectors per FAT (calculated based on cluster count)
        0x3F, 0x00,                    // Sectors per track (63)
        0xFF, 0x00,                    // Number of heads (255)
        0x00, 0x00, 0x00, 0x00,        // Hidden sectors (none)
        0x00, 0x40, 0x00, 0x00,        // Total sectors (for large volumes) (16384 sectors = 8MB)
        0x80,                          // Drive number
        0x00,                          // Reserved
        0x29,                          // Extended boot signature
        0xAA, 0x55, 0xAA, 0x55,        // Volume serial number (arbitrary)
        'N', 'O', 'R', ' ', 'D', 'I', 'S', 'K', // Volume label
        'F', 'A', 'T', '1', '6', ' ', ' ', ' ', // File system type
        // Boot code area (usually 0x1B8 - 0x1FE)
        0x00, 0x00, 0x00, 0x00, // Boot code placeholder
        // Fill the rest with zeros
        [510] = 0x55, 0xAA       // Boot sector signature
};

// FAT Table (first two sectors shown)
const uint8_t fat_table[512 * 8] = {
        0xF8, 0xFF, 0xFF, 0xFF,  // Reserved entry
        [4 ... 511] = 0x00,        // All clusters are free initially
};

// Root Directory (initial)
const uint8_t root_dir[512 * 32] = {
        // Entry for "SETTINGS.TXT"
        'S', 'E', 'T', 'T', 'I', 'N', 'G', 'S', 'T', 'X', 'T', // File name "SETTINGS.TXT"
        0x20,  // Attribute (archive)
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, 0x00,  // Time created
        0x00, 0x00,  // Date created
        0x01, 0x00,  // Starting cluster (cluster 1)
        0x00, 0x01,  // File size (256 bytes)
        // Entry for "LOG.HEX"
        'L', 'O', 'G', ' ', ' ', ' ', ' ', ' ', 'H', 'E', 'X', // File name "LOG.HEX"
        0x20,  // Attribute (archive)
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, 0x00,  // Time created
        0x00, 0x00,  // Date created
        0x02, 0x00,  // Starting cluster (cluster 2)
        0x00, 0x80,  // File size (512KB)
        // Entry for "LOG.CSV"
        'L', 'O', 'G', ' ', ' ', ' ', ' ', ' ', 'C', 'S', 'V', // File name "LOG.CSV"
        0x20,  // Attribute (archive)
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, 0x00,  // Time created
        0x00, 0x00,  // Date created
        0x82, 0x00,  // Starting cluster (cluster 130)
        0xFF, 0xFF,  // File size (all remaining space)
        // Fill the rest with zeros
};