/*!
 * @file bsp_bus.c
 * @brief implementation of bsp_bus
 *
 * Detailed description of the implementation file.
 *
 * @date 18/08/2024
 * @author artempolisskyi
 */

#include "bsp_bus.h"

extern osMutexId_t i2cMutexHandle;

int32_t I2C_WriteReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_WriteReg(Addr, Reg, pData, Length);
    osMutexRelease(i2cMutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t I2C_ReadReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_ReadReg(Addr, Reg, pData, Length);
    osMutexRelease(i2cMutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t I2C_WriteReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_WriteReg16(Addr, Reg, pData, Length);
    osMutexRelease(i2cMutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t I2C_ReadReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_ReadReg16(Addr, Reg, pData, Length);
    osMutexRelease(i2cMutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t I2C_Send(uint16_t DevAddr, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_Send(DevAddr, pData, Length);
    osMutexRelease(i2cMutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}

int32_t I2C_Recv(uint16_t DevAddr, uint8_t *pData, uint16_t Length) {
  int32_t ret;

  if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK) {
    ret = BSP_I2C1_Recv(DevAddr, pData, Length);
    osMutexRelease(i2cMutexHandle);
  } else {
    ret = BSP_ERROR_BUSY; // or another appropriate error code
  }

  return ret;
}