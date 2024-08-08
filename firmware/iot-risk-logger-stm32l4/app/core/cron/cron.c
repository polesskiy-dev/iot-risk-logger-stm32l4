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

static uint8_t monthStrToNumber(const char* monthStr);
static HAL_StatusTypeDef setCurrentTime(void);
static HAL_StatusTypeDef setCurrentDate(void);

HAL_StatusTypeDef CRON_Init(void) {
  // set time to current (compilation time)
  HAL_StatusTypeDef status = setCurrentTime();

  // set date to current (compilation date)
  status |= setCurrentDate();

  return status;
}

/**
 * @brief Set the current time to the compilation time
 */
static HAL_StatusTypeDef setCurrentTime(void) {
  RTC_TimeTypeDef sTime = {0};

  // __TIME__ is a string in the format "HH:MM:SS"
  char timeStr[] = __TIME__;

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
  sDate.WeekDay = RTC_WEEKDAY_MONDAY; // Placeholder: Change this based on actual weekday

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