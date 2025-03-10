#include "rhs_hal_rtc.h"
#include "rtc.h"

void rhs_hal_rtc_init(void) {}

void rhs_hal_rtc_set_datetime(datetime_t* datetime)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    sDate.Month   = datetime->month;
    sDate.Date    = datetime->day;
    sDate.Year    = datetime->year % 100;
    sDate.WeekDay = datetime->weekday;

    sTime.Seconds        = datetime->seconds;
    sTime.Minutes        = datetime->minutes;
    sTime.Hours          = datetime->hours;
    sTime.TimeFormat     = RTC_HOURFORMAT_24;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}

void rhs_hal_rtc_get_datetime(datetime_t* out_datetime)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    out_datetime->year    = sDate.Year + 2000;
    out_datetime->month   = sDate.Month;
    out_datetime->day     = sDate.Date;
    out_datetime->weekday = sDate.WeekDay;

    out_datetime->hours    = sTime.Hours;
    out_datetime->minutes  = sTime.Minutes;
    out_datetime->seconds  = sTime.Seconds;
    out_datetime->mseconds = ((uint64_t) (sTime.SecondFraction - sTime.SubSeconds) * 1000 / (sTime.SecondFraction + 1));
}

void rhs_hal_rtc_set_timestamp(uint64_t seconds, uint32_t mseconds)
{
    datetime_t datetime;
    datetime = convert_unix_time_2_date(seconds);
    rhs_hal_rtc_set_datetime(&datetime);
}

void rhs_hal_rtc_get_timestamp(uint64_t* out_seconds, uint32_t* out_mseconds)
{
    datetime_t datetime;

    rhs_hal_rtc_get_datetime(&datetime);

    *out_seconds  = convert_date_2_unix_time(&datetime);
    *out_mseconds = datetime.mseconds;
}
