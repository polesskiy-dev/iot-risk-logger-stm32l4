/*!
 * @file sleep.c
 * @brief Implementation of the power mode manager
 *
 * Detailed description of the implementation file.
 *
 * @date 14/08/2024
 * @author artempolisskyi
 */

#include "power_mode_manager.h"

extern void SystemClock_Config(void);

static osStatus_t handlePwrModeManagerMessage(PWRM_MANAGER_Actor_t *this, message_t *message);
static void I2C1_PinsToAnalog(void);

PWRM_MANAGER_Actor_t PWRM_MANAGER_Actor = {
        .super = {
                .actorId = PWRM_MANAGER_ACTOR_ID,
                .messageHandler = (messageHandler_t) handlePwrModeManagerMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = PWRM_NO_STATE
};

void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
  // Disable SysTick Interrupt
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

  // Suspend the HAL tick
  HAL_SuspendTick();

  switch (PWRM_MANAGER_Actor.state) {
    case PWRM_STANDBY:
      #ifdef DEBUG
        fprintf(stdout, "Entering STANDBY Mode...\n");
      #endif

      HAL_PWR_EnterSTANDBYMode();
      break;
    case PWRM_STOP2:
      #ifdef DEBUG
        fprintf(stdout, "Entering STOP2 Mode...\n");
      #endif



      // TODO remove it, it's debug disabling
      I2C1_PinsToAnalog();
      DBGMCU->CR &= ~(DBGMCU_CR_DBG_STOP | DBGMCU_CR_DBG_STANDBY | DBGMCU_CR_DBG_SLEEP);

      HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
      break;
  }

  // TODO handle non sleep modes
}

void PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
  // Restore the system clock after waking up
  SystemClock_Config();

  // Enable SysTick Interrupt
  SysTick->CTRL  |= SysTick_CTRL_TICKINT_Msk;

  // Resume the HAL tick (if using SysTick)
  HAL_ResumeTick();

  #ifdef DEBUG
    fprintf(stdout, "Exited Sleep Mode...\n");
  #endif
}

actor_t* PWRM_MANAGER_ActorInit(void) {
  PWRM_MANAGER_Actor.state = PWRM_STOP2;

  fprintf(stdout, "Power Mode Manager initialized\n");

  return &PWRM_MANAGER_Actor.super;
}

static osStatus_t handlePwrModeManagerMessage(PWRM_MANAGER_Actor_t *this, message_t *message) {
  switch (message->event) {
    // TODO handle sleep manager messages
    default:
      return osOK;
  }

  return osOK;
}

static void I2C1_PinsToAnalog(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}