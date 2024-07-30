/*!
 * @file opt3001.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 29/07/2024
 * @author artempolisskyi
 */

#ifndef OPT3001_H
#define OPT3001_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"

/**
 * @brief Config Conversion time
 */
#define OPT3001_CONFIG_CONVERSION_TIME_800_MS   (0x0800)

/**
 * @brief Config Range number.
 * Selects the full-scale lux range of the device
 */
#define OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE  (0xC000)

/**
 * @brief Config Operation mode.
 */
#define OPT3001_CONFIG_MODE_CONTINUOUS          (0x0600)
#define OPT3001_CONFIG_MODE_SINGLE_SHOT         (0x0200)
#define OPT3001_CONFIG_MODE_SHUTDOWN            (0x0000)

/**
 * @brief Config Latch.
 * 1 = The device functions in latched window-style comparison operation,
 * latching the interrupt reporting mechanisms until a user-controlled clearing event.
 * 0 = The device functions in transparent hysteresis-style comparison operation.
 */
#define OPT3001_CONFIG_LATCH_ENABLED            (0x0010)

#define OPT3001_CONFIG_FAULT_COUNT_1            (0x0000)
#define OPT3001_CONFIG_FAULT_COUNT_2            (0x0001)
#define OPT3001_CONFIG_FAULT_COUNT_4            (0x0002)
#define OPT3001_CONFIG_FAULT_COUNT_8            (0x0003)

#define OPT3001_CONFIG_DEFAULT                  (OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
                                                 OPT3001_CONFIG_CONVERSION_TIME_800_MS  | \
                                                 OPT3001_CONFIG_MODE_SHUTDOWN           | \
                                                 OPT3001_CONFIG_LATCH_ENABLED           | \
                                                 OPT3001_CONFIG_FAULT_COUNT_1)

/**
* @brief  OPT3001 Light Sensor registers addresses.
*/
#define OPT3001_RESULT_REG          (0x00)
#define OPT3001_CONFIG_REG          (0x01)
#define OPT3001_LIMIT_LOW_REG       (0x02)
#define OPT3001_LIMIT_HIGH_REG      (0x03)
#define OPT3001_MANUFACTURER_ID_REG (0x7E)
#define OPT3001_DEVICE_ID_REG       (0x7F)

#define OPT3001_MANUFACTURER_ID     (0x5449)

#define OPT3001_REGISTER_SIZE       (2)

/**
* @brief Calculate the exponent and mantissa of lux value from the raw register value.
*/
#define OPT3001_REG_EXPONENT(n)		  ((n) >> 12)
#define OPT3001_REG_MANTISSA(n)		  ((n) & 0xfff)

/**
* @brief  OPT3001 Light Sensor status enumerator definition.
*/
#define OPT3001_OK      (0)
#define OPT3001_ERROR   (-1)
#define OPT3001_BUSY    (-2)
#define OPT3001_TIMEOUT (-3)
#define OPT3001_NACK    (-102)


#define OPT3001_RESULT int32_t

typedef OPT3001_RESULT (*OPT3001_WriteReg_Func)(uint16_t, uint16_t, uint8_t*, uint16_t);
typedef OPT3001_RESULT (*OPT3001_ReadReg_Func) (uint16_t, uint16_t, uint8_t*, uint16_t);

typedef struct {
  uint8_t i2cAddress;
  OPT3001_WriteReg_Func WriteReg;
  OPT3001_ReadReg_Func ReadReg;
} OPT3001_IO_t;

OPT3001_RESULT OPT3001_InitIO(uint8_t i2cAddress, OPT3001_WriteReg_Func WriteReg, OPT3001_ReadReg_Func ReadReg);
OPT3001_RESULT OPT3001_ReadDeviceID(uint16_t *id);
OPT3001_RESULT OPT3001_WriteConfig(uint16_t config);
OPT3001_RESULT OPT3001_ReadConfig(uint16_t *config);
OPT3001_RESULT OPT3001_WriteLowLimit(uint16_t lowLimitRawLux);
OPT3001_RESULT OPT3001_WriteHighLimit(uint16_t highLimitRawLux);
OPT3001_RESULT OPT3001_ReadResultRawLux(uint16_t *rawLux);
uint32_t rawToMilliLux(uint16_t rawLux);

#ifdef __cplusplus
}
#endif

#endif //OPT3001_H