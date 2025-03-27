#include "rhs_hal_rtc.h"

#if defined(RPLC_XL) || defined(RPLC_L)
#    include "stm32f7xx_ll_rtc.h"

typedef struct
{
    uint8_t  log_level : 4;
    uint8_t  year2000 : 1;
    uint32_t reserved : 27;
} SystemReg;

_Static_assert(sizeof(SystemReg) == 4, "SystemReg size mismatch");

RTC_HandleTypeDef hrtc;

static void rtc_init(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv   = 127;
    hrtc.Init.SynchPrediv    = 255;
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        rhs_crash("RTC init failed");
    }

    if (LL_RTC_IsActiveFlag_INITS(hrtc.Instance) == 0)
    {
        sTime.Hours   = 0x12;
        sTime.Minutes = 0x0;
        sTime.Seconds = 0x0;
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
        {
            rhs_crash("RTC init failed");
        }
        sDate.WeekDay = RTC_WEEKDAY_MONDAY;
        sDate.Month   = RTC_MONTH_JANUARY;
        sDate.Date    = 0x1;
        sDate.Year    = 0x24;

        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
        {
            rhs_crash("RTC init failed");
        }
    }
}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (rtcHandle->Instance == RTC)
    {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            rhs_crash("RTC init failed");
        }

        __HAL_RCC_RTC_ENABLE();
    }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        __HAL_RCC_RTC_DISABLE();
    }
}
#else
#    error "Not implement RTC for this platform"
#endif

void rhs_hal_rtc_init(void)
{
    rtc_init();
}

void rhs_hal_rtc_set_datetime(datetime_t* datetime)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    uint32_t   reg  = rhs_hal_rtc_get_register(RHSHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*) &reg;

    data->year2000 = datetime->year >= 2000 ? 1 : 0;
    rhs_hal_rtc_set_register(RHSHalRtcRegisterSystem, reg);

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
        rhs_crash("RTC init failed");
    }
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        rhs_crash("RTC init failed");
    }
}

void rhs_hal_rtc_get_datetime(datetime_t* out_datetime)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    uint32_t   reg  = rhs_hal_rtc_get_register(RHSHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*) reg;

    vPortEnterCritical();
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        rhs_crash("RTC init failed");
    }

    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        rhs_crash("RTC init failed");
    }
    vPortExitCritical();

    out_datetime->year    = data->year2000 ? 2000 + sDate.Year : 1900 + sDate.Year;
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
    datetime          = convert_unix_time_2_date(seconds);
    datetime.mseconds = mseconds;
    rhs_hal_rtc_set_datetime(&datetime);
}

void rhs_hal_rtc_get_timestamp(uint64_t* out_seconds, uint32_t* out_mseconds)
{
    datetime_t datetime;
    rhs_hal_rtc_get_datetime(&datetime);
    *out_seconds  = convert_date_2_unix_time(&datetime);
    *out_mseconds = datetime.mseconds;
}

uint32_t rhs_hal_rtc_get_register(RHSHalRtcRegister reg)
{
    return LL_RTC_BAK_GetRegister(RTC, reg);
}

void rhs_hal_rtc_set_register(RHSHalRtcRegister reg, uint32_t value)
{
    LL_RTC_BAK_SetRegister(RTC, reg, value);
}
