/*!
 * @file opt3001.c
 * @brief implementation of opt3001
 *
 * Detailed description of the implementation file.
 *
 * @date 29/07/2024
 * @author artempolisskyi
 */

#include "opt3001.h"

OPT3001_IO_t OPT3001_IO = {
    .i2cAddress = 0x00,
    .WriteReg = NULL,
    .ReadReg = NULL
};

OPT3001_RESULT OPT3001_InitIO(uint8_t i2cAddress, OPT3001_WriteReg_Func WriteReg, OPT3001_ReadReg_Func ReadReg) {
    OPT3001_IO.i2cAddress = i2cAddress;
    OPT3001_IO.WriteReg = WriteReg;
    OPT3001_IO.ReadReg = ReadReg;
    return OPT3001_OK;
}

OPT3001_RESULT OPT3001_ReadDeviceID(uint16_t *id) {
    uint8_t idData[OPT3001_REGISTER_SIZE];
    int32_t transferResult = OPT3001_IO.ReadReg(OPT3001_IO.i2cAddress, OPT3001_DEVICE_ID_REG, idData, OPT3001_REGISTER_SIZE);

    if (transferResult != OPT3001_OK)
        return OPT3001_ERROR;

    *id = (idData[0] << 8) | idData[1];
    return OPT3001_OK;
}

OPT3001_RESULT OPT3001_ReadConfig(uint16_t *config) {
    uint8_t configData[OPT3001_REGISTER_SIZE];
    int32_t transferResult = OPT3001_IO.ReadReg(OPT3001_IO.i2cAddress, OPT3001_CONFIG_REG, configData, OPT3001_REGISTER_SIZE);

    if (transferResult != OPT3001_OK)
        return OPT3001_ERROR;

    *config = (configData[0] << 8) | configData[1];
    return OPT3001_OK;
}

OPT3001_RESULT OPT3001_WriteConfig(uint16_t config) {
    uint8_t configData[OPT3001_REGISTER_SIZE] = {config >> 8, config & 0xFF};
    return OPT3001_IO.WriteReg(OPT3001_IO.i2cAddress, OPT3001_CONFIG_REG, configData, OPT3001_REGISTER_SIZE);
}

OPT3001_RESULT OPT3001_WriteLowLimit(uint16_t lowLimitRawLux) {
  uint8_t lowLimitData[OPT3001_REGISTER_SIZE] = {lowLimitRawLux >> 8, lowLimitRawLux & 0xFF};
  return OPT3001_IO.WriteReg(OPT3001_IO.i2cAddress, OPT3001_LIMIT_LOW_REG, lowLimitData, OPT3001_REGISTER_SIZE);
}

OPT3001_RESULT OPT3001_WriteHighLimit(uint16_t highLimitRawLux) {
  uint8_t highLimitData[OPT3001_REGISTER_SIZE] = {highLimitRawLux >> 8, highLimitRawLux & 0xFF};
  return OPT3001_IO.WriteReg(OPT3001_IO.i2cAddress, OPT3001_LIMIT_HIGH_REG, highLimitData, OPT3001_REGISTER_SIZE);
}

OPT3001_RESULT OPT3001_ReadResultRawLux(uint16_t *rawLux) {
    uint8_t rawLuxData[OPT3001_REGISTER_SIZE];
    int32_t transferResult = OPT3001_IO.ReadReg(OPT3001_IO.i2cAddress, OPT3001_RESULT_REG, rawLuxData, OPT3001_REGISTER_SIZE);

    if (transferResult != OPT3001_OK)
        return OPT3001_ERROR;

    *rawLux = (rawLuxData[0] << 8) | rawLuxData[1];
    return OPT3001_OK;
}

/**
 * @brief Convert raw lux value to milli lux
 *
 * @note the formula is lux = (2^LE[3:0]) * TL[11:0] where LE is the exponent and TL is the mantissa
 *
 * @param[in] rawLux raw data from sensor
 * @return milli lux
 */
uint32_t rawToMilliLux(uint16_t rawLux) {
  uint8_t exponent = OPT3001_REG_EXPONENT(rawLux) & 0x0F;
  uint16_t mantissa = OPT3001_REG_MANTISSA(rawLux);

  return (1 << exponent) * mantissa;
}