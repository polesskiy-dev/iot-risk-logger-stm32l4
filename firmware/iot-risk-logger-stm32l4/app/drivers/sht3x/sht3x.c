/*!
 * @file sht3x.c
 * @brief implementation of sht3x
 *
 * Detailed description of the implementation file.
 *
 * @date 04/08/2024
 * @author artempolisskyi
 */

#include "sht3x.h"
#include "main.h"

// TODO move it to task
SHT3x_IO_t SHT3x_IO = {
        .i2cAddress = 0x00,
        .write = NULL,
        .read = NULL,
        .delayMs = NULL,
        .crc8 = NULL
};

// TODO move to utils
/**
 * CRC-8 lookup table,
 * CRC-8/NRSC-5 Standard: 0x31
 * Polynomial: x^8 + x^5 + x^4 + 1 (0x31)
 */
const uint8_t crc8LookupTable[16 * 16] = {
        0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97,
        0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e,
        0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4,
        0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0x0f, 0x5c, 0x6d,
        0x86, 0xb7, 0xe4, 0xd5, 0x42, 0x73, 0x20, 0x11,
        0x3f, 0x0e, 0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8,
        0xc5, 0xf4, 0xa7, 0x96, 0x01, 0x30, 0x63, 0x52,
        0x7c, 0x4d, 0x1e, 0x2f, 0xb8, 0x89, 0xda, 0xeb,
        0x3d, 0x0c, 0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa,
        0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13,
        0x7e, 0x4f, 0x1c, 0x2d, 0xba, 0x8b, 0xd8, 0xe9,
        0xc7, 0xf6, 0xa5, 0x94, 0x03, 0x32, 0x61, 0x50,
        0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c,
        0x02, 0x33, 0x60, 0x51, 0xc6, 0xf7, 0xa4, 0x95,
        0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0x0d, 0x5e, 0x6f,
        0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6,
        0x7a, 0x4b, 0x18, 0x29, 0xbe, 0x8f, 0xdc, 0xed,
        0xc3, 0xf2, 0xa1, 0x90, 0x07, 0x36, 0x65, 0x54,
        0x39, 0x08, 0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae,
        0x80, 0xb1, 0xe2, 0xd3, 0x44, 0x75, 0x26, 0x17,
        0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x09, 0x5a, 0x6b,
        0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2,
        0xbf, 0x8e, 0xdd, 0xec, 0x7b, 0x4a, 0x19, 0x28,
        0x06, 0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91,
        0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0,
        0xfe, 0xcf, 0x9c, 0xad, 0x3a, 0x0b, 0x58, 0x69,
        0x04, 0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93,
        0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a,
        0xc1, 0xf0, 0xa3, 0x92, 0x05, 0x34, 0x67, 0x56,
        0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef,
        0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15,
        0x3b, 0x0a, 0x59, 0x68, 0xff, 0xce, 0x9d, 0xac
};

// TODO refactor so that every function accepts a pointer to SHT3x_IO_t
SHT3x_RESULT SHT3x_InitIO(uint8_t i2cAddress, SHT3x_Write_Func write, SHT3x_Read_Func read, SHT3x_DelayMs_Func delayMs, SHT3x_CRC8_Func crc8) {
  SHT3x_IO.i2cAddress = i2cAddress;
  SHT3x_IO.write = write;
  SHT3x_IO.read = read;
  SHT3x_IO.delayMs = delayMs;
  SHT3x_IO.crc8 = (crc8 == NULL)
    ? SHT3x_CRC8
    : crc8;

  return SHT3x_OK;
}

/**
 * @brief Read the serial number of the SHT3x sensor.
 *
 * Serial number structure:
 * - 2 bytes: Serial number word 1
 * - 1 byte: CRC for serial number word 1
 * - 2 bytes: Serial number word 2
 * - 1 byte: CRC for serial number word 2
 *
 * @note the sensor needs the time tIDLE = 1ms to respond to the I2C read header with an ACK Bit
 *
 * @see https://sensirion.com/media/documents/E5762713/63D103C2/Sensirion_electronic_identification_code_SHT3x.pdf
 * @param id
 * @return
 */
SHT3x_RESULT SHT3x_ReadDeviceID(uint32_t *id) {
    uint8_t serialNumberData[SHT3x_SERIAL_NUMBER_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t serialNumberCMD[] = {SHT3x_SERIAL_NUMBER_CMD_ID >> 8, SHT3x_SERIAL_NUMBER_CMD_ID & 0xFF};

    SHT3x_RESULT result = SHT3x_IO.write(SHT3x_IO.i2cAddress, serialNumberCMD, SHT3x_CMD_SIZE);

    if (result != SHT3x_OK)
      return result;

    SHT3x_IO.delayMs(1);

    result = SHT3x_IO.read(SHT3x_IO.i2cAddress, serialNumberData, SHT3x_SERIAL_NUMBER_SIZE);

    if (result != SHT3x_OK)
      return result;

    // Check CRC
    uint8_t word1ReceivedCRC = serialNumberData[2];
    uint8_t word2ReceivedCRC = serialNumberData[5];
    bool word1CRCValid = SHT3x_IO.crc8(serialNumberData, 2) == word1ReceivedCRC;
    bool word2CRCValid = SHT3x_IO.crc8(serialNumberData + 3, 2) == word2ReceivedCRC;

    if (word1CRCValid == false || word2CRCValid == false) {
      return SHT3x_CRC_ERROR;
    }

    *id = serialNumberData[0] << 24 | serialNumberData[1] << 16 | serialNumberData[3] << 8 | serialNumberData[4];

    return SHT3x_OK;
}

SHT3x_RESULT SHT3x_PeriodicAcquisitionMode(uint16_t modeCondition) {
  uint8_t cmd[] = {modeCondition >> 8, modeCondition & 0xFF};

  return SHT3x_IO.write(SHT3x_IO.i2cAddress, cmd, SHT3x_CMD_SIZE);
}

SHT3x_RESULT SHT3x_ReadMeasurements(int16_t *rawTemperature, uint16_t *rawHumidity) {
  uint8_t data[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t cmd[] = {SHT3x_READ_MEASUREMENT_CMD_ID >> 8, SHT3x_READ_MEASUREMENT_CMD_ID & 0xFF};

  SHT3x_RESULT result = SHT3x_IO.write(SHT3x_IO.i2cAddress, cmd, SHT3x_CMD_SIZE);

  if (result != SHT3x_OK)
    return result;

  // TODO handle: If no measurement data is present the I2C read header is responded with a NACK
  result = SHT3x_IO.read(SHT3x_IO.i2cAddress, data, 6);

  if (result != SHT3x_OK)
    return result;

  // Check CRC
  uint8_t temperatureCRC = data[2];
  uint8_t humidityCRC = data[5];
  bool temperatureCRCValid = SHT3x_IO.crc8(data, 2) == temperatureCRC;
  bool humidityCRCValid = SHT3x_IO.crc8(data + 3, 2) == humidityCRC;

  if (temperatureCRCValid == false || humidityCRCValid == false) {
    return SHT3x_CRC_ERROR;
  }

  *rawTemperature = data[0] << 8 | data[1];
  *rawHumidity = data[3] << 8 | data[4];

  return SHT3x_OK;
}

float SHT3x_RawToTemperatureC(int16_t rawTemperature) {
  // simplified (65536 instead of 65535) integer version of:
  // temp = (stemp * 175.0f) / 65535.0f - 45.0f;
  int32_t stemp = ((4375 * (int32_t)rawTemperature) >> 14) - 4500;

  return (float)stemp / 100.0f;
}

float SHT3x_RawToHumidityRH(uint16_t rawHumidity) {
  // simplified (65536 instead of 65535) integer version of:
  // humidity = (shum * 100.0f) / 65535.0f;
  uint32_t shum = (625 * (uint32_t)rawHumidity) >> 12;

  return (float)shum / 100.0f;
}

uint8_t SHT3x_CRC8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;  // Initial value

  for (uint8_t i = 0; i < len; i++) {
    crc = crc8LookupTable[crc ^ data[i]];
  }

  return crc;
}