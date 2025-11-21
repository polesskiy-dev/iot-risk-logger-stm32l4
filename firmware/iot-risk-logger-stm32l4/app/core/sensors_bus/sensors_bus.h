/*!
 * @file sensors_bus.h
 * @brief Thread-safe I2C bus access layer for sensor communication.
 *
 * Detailed description of the file.
 *
 * @date 18/08/2024
 * @author artempolisskyi
 */

#ifndef SENSORS_BUS_H
#define SENSORS_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "custom_bus.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"

int32_t SensorsBus_WriteReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t SensorsBus_ReadReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t SensorsBus_WriteReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t SensorsBus_ReadReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t SensorsBus_Send(uint16_t DevAddr, uint8_t *pData, uint16_t Length);
int32_t SensorsBus_Recv(uint16_t DevAddr, uint8_t *pData, uint16_t Length);

#ifdef __cplusplus
}
#endif

#endif //SENSORS_BUS_H