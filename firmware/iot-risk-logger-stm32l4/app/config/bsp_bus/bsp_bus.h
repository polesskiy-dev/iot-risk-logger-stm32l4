/*!
 * @file bsp_bus.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 18/08/2024
 * @author artempolisskyi
 */

#ifndef BSP_BUS_H
#define BSP_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "custom_bus.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"

int32_t I2C_WriteReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t I2C_ReadReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t I2C_WriteReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t I2C_ReadReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t I2C_Send(uint16_t DevAddr, uint8_t *pData, uint16_t Length);
int32_t I2C_Recv(uint16_t DevAddr, uint8_t *pData, uint16_t Length);
int32_t I2C_SendRecv(uint16_t DevAddr, uint8_t *pTxdata, uint8_t *pRxdata, uint16_t Length);

#ifdef __cplusplus
}
#endif

#endif //BSP_BUS_H