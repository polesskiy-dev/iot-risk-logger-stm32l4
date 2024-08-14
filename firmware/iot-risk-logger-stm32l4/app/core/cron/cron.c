/*!
 * @file cron.c
 * @brief implementation of cron
 *
 * Detailed description of the implementation file.
 *
 * @date 09/08/2024
 * @author artempolisskyi
 */

#include "cron.h"

static osStatus_t handleCronMessage(CRON_Actor_t *this, message_t *message);
static uint8_t monthStrToNumber(const char* monthStr);
static osStatus_t setTimeFromUnixTimestamp(int32_t timestamp);
static osStatus_t setWakeUpPeriod(uint32_t periodSeconds);
static int32_t getCurrentUnixTimestamp(void);

static HAL_StatusTypeDef setCurrentTime(void);
static HAL_StatusTypeDef setCurrentDate(void);

CRON_Actor_t CRON_Actor = {
  .super = {
    .actorId = CRON_ACTOR_ID,
    .messageHandler = (messageHandler_t) handleCronMessage,
    .osMessageQueueId = NULL,
    .osThreadId = NULL,
  },
};

actor_t* CRON_ActorInit(void) {
  // set time to current (compilation time)
  HAL_StatusTypeDef status = setCurrentTime();

  // set date to current (compilation date)
  status |= setCurrentDate();

  // TODO remove after debugging
  status |= setWakeUpPeriod(30); // every 30 seconds

  fprintf(stdout, "Cron initialized: %d\n", status);
  return &CRON_Actor.super;
}


void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
  int32_t currentTimestamp = getCurrentUnixTimestamp();

  osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_RTC_WAKE_UP, .payload.value = currentTimestamp}, 0, 0);
}

static osStatus_t handleCronMessage(CRON_Actor_t *this, message_t *message) {
  switch (message->event) {
    case GLOBAL_CMD_SET_TIME_DATE:
      return setTimeFromUnixTimestamp((int32_t)message->payload.value);
    case GLOBAL_CMD_SET_WAKE_UP_PERIOD:
      return setWakeUpPeriod(message->payload.value);
  }
  return osOK;
}

static osStatus_t setTimeFromUnixTimestamp(int32_t timestamp) {
  // Convert Unix timestamp to broken-down time structure
  time_t rawTime = timestamp;
  struct tm *timeStruct = gmtime(&rawTime);

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  // Set time
  sTime.Hours = timeStruct->tm_hour;
  sTime.Minutes = timeStruct->tm_min;
  sTime.Seconds = timeStruct->tm_sec;
  sTime.TimeFormat = RTC_HOURFORMAT_24;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;

  // Set date
  sDate.Year = timeStruct->tm_year - YEARS_FROM_1900_TO_2000; // Adjust year to RTC format (00-99)
  sDate.Month = timeStruct->tm_mon + 1; // Adjust month to RTC format (1-12)
  sDate.Date = timeStruct->tm_mday;
  sDate.WeekDay = (timeStruct->tm_wday == 0) ? 7 : timeStruct->tm_wday; // Sunday = 0 in tm struct, should be 7 in RTC

  // Set RTC time and date
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
    return osError;
  }

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
    return osError;
  }

  return osOK;
}

static osStatus_t setWakeUpPeriod(uint32_t periodSeconds) {
  // Disable wake-up timer to modify it
  if (HAL_RTCEx_DeactivateWakeUpTimer(&hrtc) != HAL_OK) {
    return osError;
  }

  uint32_t wakeUpCounter = periodSeconds - 1; // Adjust for 0-based counter

  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeUpCounter, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, WAKE_UP_AUTO_CLEAR) != HAL_OK) {
    return osError;
  }

  return osOK;
}

static int32_t getCurrentUnixTimestamp(void) {
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  HAL_StatusTypeDef status = HAL_RTC_GetTime (&hrtc, &sTime, RTC_FORMAT_BIN);
  status |= HAL_RTC_GetDate (&hrtc, &sDate, RTC_FORMAT_BIN);

  // Convert time and date to Unix timestamp
  struct tm timeStruct = {
    .tm_sec = sTime.Seconds,
    .tm_min = sTime.Minutes,
    .tm_hour = sTime.Hours,
    .tm_mday = sDate.Date, // day of the month, 1-31
    .tm_mon = sDate.Month - 1, // months from 0
    .tm_year = sDate.Year + YEARS_FROM_1900_TO_2000, // years from 1900
  };

  return mktime(&timeStruct);
}

/**
 * @brief Set the current time to the compilation time
 */
static HAL_StatusTypeDef setCurrentTime(void) {
  RTC_TimeTypeDef sTime = {0};

  // __TIME__ is a string in the format "HH:MM:SS"
  char timeStr[] = __TIME__; // TODO handle timezone subtraction

  // Extract hours, minutes, and seconds
  sTime.Hours = atoi(&timeStr[0]);    // Convert "HH" to integer
  sTime.Minutes = atoi(&timeStr[3]);  // Convert "MM" to integer
  sTime.Seconds = atoi(&timeStr[6]);  // Convert "SS" to integer

  sTime.TimeFormat = RTC_HOURFORMAT_24;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;

  return HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

/**
 * @brief Set the current date to the compilation date
 */
static HAL_StatusTypeDef setCurrentDate(void) {
  RTC_DateTypeDef sDate = {0};

  // __DATE__ is a string in the format "MMM DD YYYY"
  char dateStr[] = __DATE__;

  // Extract and convert the month, day, and year
  char monthStr[4] = {dateStr[0], dateStr[1], dateStr[2], '\0'};  // Extract "MMM"
  sDate.Month = monthStrToNumber(monthStr);             // Convert month to number

  sDate.Date = atoi(&dateStr[4]);        // Convert "DD" to integer
  sDate.Year = atoi(&dateStr[7]) - 2000; // Convert "YYYY" to last two digits (RTC stores only last two digits)

  // Set a placeholder weekday; you can adjust this if you need the exact day
  sDate.WeekDay = RTC_WEEKDAY_MONDAY; // TODO: Change this based on actual weekday

  return HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

/**
 * @brief Helper function to convert month string to integer
 */
static uint8_t monthStrToNumber(const char* monthStr) {
  if (strcmp(monthStr, "Jan") == 0) return 1;
  if (strcmp(monthStr, "Feb") == 0) return 2;
  if (strcmp(monthStr, "Mar") == 0) return 3;
  if (strcmp(monthStr, "Apr") == 0) return 4;
  if (strcmp(monthStr, "May") == 0) return 5;
  if (strcmp(monthStr, "Jun") == 0) return 6;
  if (strcmp(monthStr, "Jul") == 0) return 7;
  if (strcmp(monthStr, "Aug") == 0) return 8;
  if (strcmp(monthStr, "Sep") == 0) return 9;
  if (strcmp(monthStr, "Oct") == 0) return 10;
  if (strcmp(monthStr, "Nov") == 0) return 11;
  if (strcmp(monthStr, "Dec") == 0) return 12;
  return 0; // Return 0 if the month string is invalid
}