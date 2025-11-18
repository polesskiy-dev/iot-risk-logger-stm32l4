/*!
 * @file sensors_bus.c
 * @brief Implementation of thread-safe I2C sensor bus access layer
 *
 * Provides thread-safe access to I2C sensor devices using mutex protection.
 *
 * @date 18/08/2024
 * @author artempolisskyi
 */

#include "sensors_bus.h"

extern osMutexId_t i2c1MutexHandle;

int32_t SensorsBus_WriteReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_WriteReg(Addr, Reg, pData, Length);
    osMutexRelease(i2c1MutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t SensorsBus_ReadReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_ReadReg(Addr, Reg, pData, Length);
    osMutexRelease(i2c1MutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t SensorsBus_WriteReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_WriteReg16(Addr, Reg, pData, Length);
    osMutexRelease(i2c1MutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t SensorsBus_ReadReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_ReadReg16(Addr, Reg, pData, Length);
    osMutexRelease(i2c1MutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t SensorsBus_Send(uint16_t DevAddr, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_Send(DevAddr, pData, Length);
    osMutexRelease(i2c1MutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t SensorsBus_Recv(uint16_t DevAddr, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_Recv(DevAddr, pData, Length);
    osMutexRelease(i2c1MutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}