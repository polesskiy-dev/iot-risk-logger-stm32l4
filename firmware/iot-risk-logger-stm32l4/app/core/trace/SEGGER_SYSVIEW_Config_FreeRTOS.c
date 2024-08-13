/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2021 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the SystemView and RTT protocol, and J-Link.       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: 3.32                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SEGGER_SYSVIEW_Config_FreeRTOS.c
Purpose : Sample setup configuration of SystemView with FreeRTOS.
Revision: $Rev: 7745 $
*/
#include "FreeRTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "stm32l412xx.h"

extern const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI;

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// The application name to be displayed in SystemViewer
#define SYSVIEW_APP_NAME        "IoT Risk Data Logger"

// The target device name
#define SYSVIEW_DEVICE_NAME     "Cortex-M4"

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_GET_TIMESTAMP in SEGGER_SYSVIEW_Conf.h
#define SYSVIEW_TIMESTAMP_FREQ  (configCPU_CLOCK_HZ)

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#define SYSVIEW_CPU_FREQ        configCPU_CLOCK_HZ

/*
 * The SRAM total size is 128 Kbytes. It is split into 2 parts:
 * SRAM1 is 96 Kbytes starting from address 0x20000000 and
 * SRAM2 is 32 Kbytes starting from address 0x10000000. SRAM1 is located in the usual ARM
 * memory space for RAM while SRAM2 can be directly accessed through Data code and Instruction code buses
 * with 0 wait states, and can be used for code execution.
 */

// The lowest RAM address used for IDs (pointers)
#define SYSVIEW_RAM_BASE        (0x10000000) // SRAM2 base address

/*
 * IRQn_Type.SysTick_IRQn == -1 but actually it is 15 words (0x0000 003C) from NVIC start,
 * so we need to add 15 to get the correct IRQ number e.g. WWDG is 16, PVD_PVM is 17 etc.
 */
#define SYS_TICK_IRQ_NUMBER_OFFSET (15)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define IRQN_OFFSET(IRQnumber) (IRQnumber + SYS_TICK_IRQ_NUMBER_OFFSET)
#define IRQN_TO_STRING(IRQnumber) TOSTRING(IRQN_OFFSET(IRQnumber))
#define IRQ_TO_SYSVIEW_DESCRIPTION(IRQname) "I#" IRQN_TO_STRING(IRQname) "=" #IRQname

/*********************************************************************
*
*       _cbSendSystemDesc()
*
*  Function description
*    Sends SystemView description strings.
*/
static void _cbSendSystemDesc(void) {
  SEGGER_SYSVIEW_SendSysDesc("N="SYSVIEW_APP_NAME",D="SYSVIEW_DEVICE_NAME",O=FreeRTOS");
  SEGGER_SYSVIEW_SendSysDesc("I#15=SysTick");
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(WWDG_IRQn));
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(RTC_WKUP_IRQn));
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(EXTI0_IRQn));
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(EXTI1_IRQn));
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(EXTI9_5_IRQn));
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(USB_IRQn));
  SEGGER_SYSVIEW_SendSysDesc(IRQ_TO_SYSVIEW_DESCRIPTION(QUADSPI_IRQn));
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
void SEGGER_SYSVIEW_Conf(void) {
  SEGGER_SYSVIEW_Init(SYSVIEW_TIMESTAMP_FREQ, SYSVIEW_CPU_FREQ,
                      &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);
  SEGGER_SYSVIEW_SetRAMBase(SYSVIEW_RAM_BASE);
}

/*************************** End of file ****************************/
