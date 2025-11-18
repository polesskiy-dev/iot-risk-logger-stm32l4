/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMutexId_t i2cMutexHandle;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  i2cMutexHandle = osMutexNew(&(osMutexAttr_t){ .name = "i2cMutex" });
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  extern actor_t* ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS];
  /* add threads, ... */

  // TODO maybe move it to /app/init/system_init.c and call from there

  /**
   * @brief Initialize actors threads
   *
   * Save pointers to them in the common registry
   * Not all actors have threads, but all of them have os message queues, so they should be initialized in terms of os
   * */
  ACTORS_LOOKUP_SystemRegistry[CRON_ACTOR_ID]                         = CRON_ActorInit();
  ACTORS_LOOKUP_SystemRegistry[PWRM_MANAGER_ACTOR_ID]                 = PWRM_MANAGER_ActorInit();
  ACTORS_LOOKUP_SystemRegistry[TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID]  = TH_SENS_TaskInit();
  ACTORS_LOOKUP_SystemRegistry[LIGHT_SENSOR_ACTOR_ID]                 = LIGHT_SENS_TaskInit();
  ACTORS_LOOKUP_SystemRegistry[IMU_ACTOR_ID]                          = IMU_TaskInit();
  ACTORS_LOOKUP_SystemRegistry[MEMORY_ACTOR_ID]                       = MEMORY_TaskInit();
  ACTORS_LOOKUP_SystemRegistry[NFC_ACTOR_ID]                          = NFC_TaskInit();
  ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]                   = EV_MANAGER_ActorInit(defaultTaskHandle); // should be initialized last

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  (void) argument; // Avoid unused parameter warning
  message_t msg;
  /* Infinite loop */
  for(;;)
  {
    // process event manager messages
    if (osMessageQueueGet(EV_MANAGER_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      EV_MANAGER_Actor.super.messageHandler(&EV_MANAGER_Actor.super, &msg);
    }
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

