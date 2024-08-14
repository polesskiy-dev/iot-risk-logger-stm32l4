/*!
 * @file sleep.c
 * @brief implementation of power mode manager
 *
 * Detailed description of the implementation file.
 *
 * @date 14/08/2024
 * @author artempolisskyi
 */

#include "power_mode_manager.h"

extern void SystemClock_Config(void);

static osStatus_t handlePwrModeManagerMessage(PWRM_MANAGER_Actor_t *this, message_t *message);

PWRM_MANAGER_Actor_t PWRM_MANAGER_Actor = {
        .super = {
                .actorId = PWRM_MANAGER_ACTOR_ID,
                .messageHandler = (messageHandler_t) handlePwrModeManagerMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = PWRM_STOP2
};

void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
  // Disable SysTick Interrupt
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

  // Suspend the HAL tick
  HAL_SuspendTick();

  switch (PWRM_MANAGER_Actor.state) {
    case PWRM_STANDBY:
      fprintf(stdout, "Entering STANDBY Mode...\n");
      HAL_PWR_EnterSTANDBYMode();
      break;
    case PWRM_STOP2:
      fprintf(stdout, "Entering STOP2 Mode...\n");
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

  fprintf(stdout, "Exited Sleep Mode...\n");
}

actor_t* PWRM_MANAGER_ActorInit(void) {
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