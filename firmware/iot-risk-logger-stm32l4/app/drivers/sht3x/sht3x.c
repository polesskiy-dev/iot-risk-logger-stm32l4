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

SHT3x_IO_t SHT3x_IO = {
        .i2cAddress = 0x00,
        .writeCommand = NULL,
        .readCommand = NULL,
        .crc8 = NULL
};

SHT3x_RESULT SHT3x_InitIO(uint8_t i2cAddress, SHT3x_WriteCommand_Func writeCommand, SHT3x_ReadCommand_Func readCommand, SHT3x_CRC8_Func crc8) {
  SHT3x_IO.i2cAddress = i2cAddress;
  SHT3x_IO.writeCommand = writeCommand;
  SHT3x_IO.readCommand = readCommand;
  SHT3x_IO.crc8 = crc8;

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
 * @see https://sensirion.com/media/documents/E5762713/63D103C2/Sensirion_electronic_identification_code_SHT3x.pdf
 * @param id
 * @return
 */
SHT3x_RESULT SHT3x_ReadDeviceID(uint32_t *id) {
    uint8_t serialNumberData[SHT3x_SERIAL_NUMBER_SIZE];
    SHT3x_RESULT result = SHT3x_IO.readCommand(SHT3x_IO.i2cAddress, SHT3x_SERIAL_NUMBER_CMD_ID, serialNumberData, SHT3x_SERIAL_NUMBER_SIZE);

    if (result != SHT3x_OK) {
      return result;
    }

    // TODO remove this check after the CRC8 function is implemented
    if (SHT3x_IO.crc8 != NULL) {
      // Check CRC
      uint8_t word1ReceivedCRC = serialNumberData[2];
      uint8_t word2ReceivedCRC = serialNumberData[5];
      bool word1CRCValid = SHT3x_IO.crc8(serialNumberData, 2) == word1ReceivedCRC;
      bool word2CRCValid = SHT3x_IO.crc8(serialNumberData + 3, 2) == word2ReceivedCRC;

      if (word1CRCValid == false || word2CRCValid == false) {
        return SHT3x_CRC_ERROR;
      }
    }

    *id = serialNumberData[0] << 24 | serialNumberData[1] << 16 | serialNumberData[3] << 8 | serialNumberData[4];

    return SHT3x_OK;
}

float SHT3x_RawToTemperatureC(uint16_t rawTemperature) {
  return -45.0f + 175.0f * (rawTemperature / 65535.0f);
}

float SHT3x_RawToHumidityRH(uint16_t rawHumidity) {
  return 100.0f * (rawHumidity / 65535.0f);
}