/*!
 * @file sht3x.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 04/08/2024
 * @author artempolisskyi
 */

#ifndef SHT3X_H
#define SHT3X_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"

#define SHT3x_COMMAND_SIZE 2

#define SHT3x_MEASURE_SINGLE_SHOT_HIGH_REPEATABILITY_CMD_ID                     (0x2400)
#define SHT3x_MEASURE_SINGLE_SHOT_HIGH_REPEATABILITY_CLOCK_STRETCHING_CMD_ID    (0x2c06)
#define SHT3x_MEASURE_SINGLE_SHOT_MEDIUM_REPEATABILITY_CMD_ID                   (0x240b)
#define SHT3x_MEASURE_SINGLE_SHOT_MEDIUM_REPEATABILITY_CLOCK_STRETCHING_CMD_ID  (0x2c0d)
#define SHT3x_MEASURE_SINGLE_SHOT_LOW_REPEATABILITY_CMD_ID                      (0x2416)
#define SHT3x_MEASURE_SINGLE_SHOT_LOW_REPEATABILITY_CLOCK_STRETCHING_CMD_ID     (0x2c10)
#define SHT3x_START_MEASUREMENT_0_5_MPS_HIGH_REPEATABILITY_CMD_ID               (0x2032)
#define SHT3x_START_MEASUREMENT_0_5_MPS_MEDIUM_REPEATABILITY_CMD_ID             (0x2024)
#define SHT3x_START_MEASUREMENT_0_5_MPS_LOW_REPEATABILITY_CMD_ID                (0x202f)
#define SHT3x_START_MEASUREMENT_1_MPS_HIGH_REPEATABILITY_CMD_ID                 (0x2130)
#define SHT3x_START_MEASUREMENT_1_MPS_MEDIUM_REPEATABILITY_CMD_ID               (0x2126)
#define SHT3x_START_MEASUREMENT_1_MPS_LOW_REPEATABILITY_CMD_ID                  (0x212d)
#define SHT3x_START_MEASUREMENT_2_MPS_HIGH_REPEATABILITY_CMD_ID                 (0x2236)
#define SHT3x_START_MEASUREMENT_2_MPS_MEDIUM_REPEATABILITY_CMD_ID               (0x2220)
#define SHT3x_START_MEASUREMENT_2_MPS_LOW_REPEATABILITY_CMD_ID                  (0x222b)
#define SHT3x_START_MEASUREMENT_4_MPS_HIGH_REPEATABILITY_CMD_ID                 (0x2334)
#define SHT3x_START_MEASUREMENT_4_MPS_MEDIUM_REPEATABILITY_CMD_ID               (0x2322)
#define SHT3x_START_MEASUREMENT_4_MPS_LOW_REPEATABILITY_CMD_ID                  (0x2329)
#define SHT3x_START_MEASUREMENT_10_MPS_HIGH_REPEATABILITY_CMD_ID                (0x2737)
#define SHT3x_START_MEASUREMENT_10_MPS_MEDIUM_REPEATABILITY_CMD_ID              (0x2721)
#define SHT3x_START_MEASUREMENT_10_MPS_LOW_REPEATABILITY_CMD_ID                 (0x273a)
#define SHT3x_START_ART_MEASUREMENT_CMD_ID                                      (0x2b32)
#define SHT3x_READ_MEASUREMENT_CMD_ID                                           (0xe000)
#define SHT3x_STOP_MEASUREMENT_CMD_ID                                           (0x3093)
#define SHT3x_ENABLE_HEATER_CMD_ID                                              (0x306d)
#define SHT3x_DISABLE_HEATER_CMD_ID                                             (0x3066)
#define SHT3x_READ_STATUS_REGISTER_CMD_ID                                       (0xf32d)
#define SHT3x_CLEAR_STATUS_REGISTER_CMD_ID                                      (0x3041)
#define SHT3x_SOFT_RESET_CMD_ID                                                 (0x30a2)

/**
* @brief  SHT3x Temperature & Humidity Sensor status enumerator definition.
*/
#define SHT3x_OK        (0)
#define SHT3x_ERROR     (-1)
#define SHT3x_BUSY      (-2)
#define SHT3x_TIMEOUT   (-3)
#define SHT3x_CRC_ERROR (-4)
#define SHT3x_NACK      (-102)

#define SHT3x_RESULT int32_t

typedef SHT3x_RESULT (*SHT3x_WriteCommand_Func)(uint16_t, uint16_t, uint8_t*, uint16_t);
typedef SHT3x_RESULT (*SHT3x_ReadCommand_Func) (uint16_t, uint16_t, uint8_t*, uint16_t);
typedef SHT3x_RESULT (*SHT3x_HardResetPulse_Func) (void);
typedef uint8_t (*SHT3x_CRC8_Func) (uint8_t*, uint8_t);

typedef struct {
  uint8_t i2cAddress;
  SHT3x_WriteCommand_Func writeCommand;
  SHT3x_ReadCommand_Func readCommand;
  SHT3x_HardResetPulse_Func hardResetPulse;
  SHT3x_CRC8_Func crc8;
} SHT3x_IO_t;

SHT3x_RESULT SHT3x_InitIO(uint8_t i2cAddress, SHT3x_WriteCommand_Func writeCommand, SHT3x_ReadCommand_Func readCommand, SHT3x_HardResetPulse_Func hardResetPulse, SHT3x_CRC8_Func crc8);
SHT3x_RESULT SHT3x_ReadDeviceID(uint16_t *id);
SHT3x_RESULT SHT3x_ReadStatus(uint16_t *status);
SHT3x_RESULT SHT3x_ClearStatus(void);
SHT3x_RESULT SHT3x_SingleShotAcquisitionMode(uint16_t modeCondition);
SHT3x_RESULT SHT3x_PeriodicAcquisitionMode(uint16_t modeCondition);
SHT3x_RESULT SHT3x_ReadMeasurements(uint16_t *rawTemperature, uint16_t *rawHumidity);
SHT3x_RESULT SHT3x_WriteLimit(uint16_t limitType, uint16_t *limit);
SHT3x_RESULT SHT3x_ReadLimit(uint16_t limitType, uint16_t *limit);
SHT3x_RESULT SHT3x_HardReset(void);
float SHT3x_RawToTemperatureC(uint16_t rawTemperature);
float SHT3x_RawToHumidityRH(uint16_t rawHumidity);
uint16_t SHT3x_TemperatureHumidityToLimit(float temperature, float humidity);
float SHT3x_LimitToTemperatureC(uint16_t temperature);
float SHT3x_LimitToHumidityC(uint16_t humidity);

#ifdef __cplusplus
}
#endif

#endif //SHT3X_H