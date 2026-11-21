#include <rhs.h>
#include <rhs_hal_adc.h>
#include <rhs_hal_cortex.h>

#if defined(STM32G0B1xx)
#    include "stm32g0xx_ll_adc.h"
#    include "stm32g0xx_ll_bus.h"
#else
#    error "rhs_hal_adc: not implemented for this MCU"
#endif

#define TAG "RHSHalAdc"

/* Polling timeouts, with a margin over the worst case: 256 oversamples of
 * (12.5 + 160.5) clocks at 16 MHz is ~2.8 ms */
#define RHS_HAL_ADC_TIMEOUT_US       10000U
#define RHS_HAL_ADC_CALIB_TIMEOUT_US 1000U

struct RHSHalAdcHandle
{
    ADC_TypeDef* adc;
    RHSMutex*    mutex;
    uint32_t     vdda_mv;
};

static const uint32_t rhs_hal_adc_clock[] = {
    [RHSHalAdcClockSync16] = LL_ADC_CLOCK_SYNC_PCLK_DIV4,
    [RHSHalAdcClockSync32] = LL_ADC_CLOCK_SYNC_PCLK_DIV2,
};

static const uint32_t rhs_hal_adc_oversample_ratio[] = {
    [RHSHalAdcOversample2]   = LL_ADC_OVS_RATIO_2,
    [RHSHalAdcOversample4]   = LL_ADC_OVS_RATIO_4,
    [RHSHalAdcOversample8]   = LL_ADC_OVS_RATIO_8,
    [RHSHalAdcOversample16]  = LL_ADC_OVS_RATIO_16,
    [RHSHalAdcOversample32]  = LL_ADC_OVS_RATIO_32,
    [RHSHalAdcOversample64]  = LL_ADC_OVS_RATIO_64,
    [RHSHalAdcOversample128] = LL_ADC_OVS_RATIO_128,
    [RHSHalAdcOversample256] = LL_ADC_OVS_RATIO_256,
};

static const uint32_t rhs_hal_adc_oversample_shift[] = {
    [RHSHalAdcOversample2]   = LL_ADC_OVS_SHIFT_RIGHT_1,
    [RHSHalAdcOversample4]   = LL_ADC_OVS_SHIFT_RIGHT_2,
    [RHSHalAdcOversample8]   = LL_ADC_OVS_SHIFT_RIGHT_3,
    [RHSHalAdcOversample16]  = LL_ADC_OVS_SHIFT_RIGHT_4,
    [RHSHalAdcOversample32]  = LL_ADC_OVS_SHIFT_RIGHT_5,
    [RHSHalAdcOversample64]  = LL_ADC_OVS_SHIFT_RIGHT_6,
    [RHSHalAdcOversample128] = LL_ADC_OVS_SHIFT_RIGHT_7,
    [RHSHalAdcOversample256] = LL_ADC_OVS_SHIFT_RIGHT_8,
};

static const uint32_t rhs_hal_adc_sampling_time[] = {
    [RHSHalAdcSamplingtime1_5]   = LL_ADC_SAMPLINGTIME_1CYCLE_5,
    [RHSHalAdcSamplingtime3_5]   = LL_ADC_SAMPLINGTIME_3CYCLES_5,
    [RHSHalAdcSamplingtime7_5]   = LL_ADC_SAMPLINGTIME_7CYCLES_5,
    [RHSHalAdcSamplingtime12_5]  = LL_ADC_SAMPLINGTIME_12CYCLES_5,
    [RHSHalAdcSamplingtime19_5]  = LL_ADC_SAMPLINGTIME_19CYCLES_5,
    [RHSHalAdcSamplingtime39_5]  = LL_ADC_SAMPLINGTIME_39CYCLES_5,
    [RHSHalAdcSamplingtime79_5]  = LL_ADC_SAMPLINGTIME_79CYCLES_5,
    [RHSHalAdcSamplingtime160_5] = LL_ADC_SAMPLINGTIME_160CYCLES_5,
};

static const uint32_t rhs_hal_adc_channel_map[] = {
    [RHSHalAdcChannel0]          = LL_ADC_CHANNEL_0,
    [RHSHalAdcChannel1]          = LL_ADC_CHANNEL_1,
    [RHSHalAdcChannel2]          = LL_ADC_CHANNEL_2,
    [RHSHalAdcChannel3]          = LL_ADC_CHANNEL_3,
    [RHSHalAdcChannel4]          = LL_ADC_CHANNEL_4,
    [RHSHalAdcChannel5]          = LL_ADC_CHANNEL_5,
    [RHSHalAdcChannel6]          = LL_ADC_CHANNEL_6,
    [RHSHalAdcChannel7]          = LL_ADC_CHANNEL_7,
    [RHSHalAdcChannel8]          = LL_ADC_CHANNEL_8,
    [RHSHalAdcChannel9]          = LL_ADC_CHANNEL_9,
    [RHSHalAdcChannel10]         = LL_ADC_CHANNEL_10,
    [RHSHalAdcChannel11]         = LL_ADC_CHANNEL_11,
    [RHSHalAdcChannel12]         = LL_ADC_CHANNEL_12,
    [RHSHalAdcChannel13]         = LL_ADC_CHANNEL_13,
    [RHSHalAdcChannel14]         = LL_ADC_CHANNEL_14,
    [RHSHalAdcChannel15]         = LL_ADC_CHANNEL_15,
    [RHSHalAdcChannel16]         = LL_ADC_CHANNEL_16,
    [RHSHalAdcChannel17]         = LL_ADC_CHANNEL_17,
    [RHSHalAdcChannel18]         = LL_ADC_CHANNEL_18,
    [RHSHalAdcChannelVREFINT]    = LL_ADC_CHANNEL_VREFINT,
    [RHSHalAdcChannelTEMPSENSOR] = LL_ADC_CHANNEL_TEMPSENSOR,
    [RHSHalAdcChannelVBAT]       = LL_ADC_CHANNEL_VBAT,
};

static RHSHalAdcHandle rhs_hal_adc_handle = {0};

static bool rhs_hal_adc_is_internal_channel(RHSHalAdcChannel channel)
{
    return channel == RHSHalAdcChannel12 || channel == RHSHalAdcChannel13 || channel == RHSHalAdcChannel14 ||
           channel >= RHSHalAdcChannelVREFINT;
}

static void rhs_hal_adc_wait(volatile const uint32_t* reg, uint32_t mask, bool set, uint32_t timeout_us)
{
    RHSHalCortexTimer timer = rhs_hal_cortex_timer_get(timeout_us);
    while (((*reg & mask) != 0) != set)
    {
        if (rhs_hal_cortex_timer_is_expired(timer))
        {
            rhs_crash("ADC timeout");
        }
    }
}

static void rhs_hal_adc_disable(ADC_TypeDef* adc)
{
    if (LL_ADC_REG_IsConversionOngoing(adc))
    {
        LL_ADC_REG_StopConversion(adc);
        rhs_hal_adc_wait(&adc->CR, ADC_CR_ADSTART, false, RHS_HAL_ADC_TIMEOUT_US);
    }
    if (LL_ADC_IsEnabled(adc))
    {
        LL_ADC_Disable(adc);
        rhs_hal_adc_wait(&adc->CR, ADC_CR_ADEN, false, RHS_HAL_ADC_TIMEOUT_US);
    }
}

void rhs_hal_adc_init(void)
{
    rhs_hal_adc_handle.adc   = ADC1;
    rhs_hal_adc_handle.mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
}

RHSHalAdcHandle* rhs_hal_adc_acquire(void)
{
    rhs_assert(rhs_mutex_acquire(rhs_hal_adc_handle.mutex, RHSWaitForever) == RHSStatusOk);

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC);

    return &rhs_hal_adc_handle;
}

void rhs_hal_adc_release(RHSHalAdcHandle* handle)
{
    rhs_assert(handle);

    rhs_hal_adc_disable(handle->adc);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(handle->adc), LL_ADC_PATH_INTERNAL_NONE);
    LL_ADC_DisableInternalRegulator(handle->adc);

    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_ADC);

    rhs_assert(rhs_mutex_release(handle->mutex) == RHSStatusOk);
}

void rhs_hal_adc_configure(RHSHalAdcHandle* handle)
{
    rhs_hal_adc_configure_ex(handle, RHSHalAdcClockSync32, RHSHalAdcOversample64, RHSHalAdcSamplingtime160_5);
}

void rhs_hal_adc_configure_ex(RHSHalAdcHandle*      handle,
                              RHSHalAdcClock        clock,
                              RHSHalAdcOversample   oversample,
                              RHSHalAdcSamplingTime sampling_time)
{
    rhs_assert(handle);
    rhs_assert(clock <= RHSHalAdcClockSync32);
    rhs_assert(oversample <= RHSHalAdcOversampleNone);
    rhs_assert(sampling_time <= RHSHalAdcSamplingtime160_5);

    ADC_TypeDef* adc = handle->adc;

    // Most of the configuration registers are writable only with ADC disabled
    rhs_hal_adc_disable(adc);

    LL_ADC_SetClock(adc, rhs_hal_adc_clock[clock]);
    LL_ADC_SetResolution(adc, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(adc, LL_ADC_DATA_ALIGN_RIGHT);
    LL_ADC_SetLowPowerMode(adc, LL_ADC_LP_MODE_NONE);

    LL_ADC_REG_SetTriggerSource(adc, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetSequencerConfigurable(adc, LL_ADC_REG_SEQ_FIXED);
    LL_ADC_REG_SetSequencerDiscont(adc, LL_ADC_REG_SEQ_DISCONT_DISABLE);
    LL_ADC_REG_SetContinuousMode(adc, LL_ADC_REG_CONV_SINGLE);
    LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_NONE);
    LL_ADC_REG_SetOverrun(adc, LL_ADC_REG_OVR_DATA_OVERWRITTEN);

    if (oversample == RHSHalAdcOversampleNone)
    {
        LL_ADC_SetOverSamplingScope(adc, LL_ADC_OVS_DISABLE);
    }
    else
    {
        LL_ADC_SetOverSamplingScope(adc, LL_ADC_OVS_GRP_REGULAR_CONTINUED);
        LL_ADC_SetOverSamplingDiscont(adc, LL_ADC_OVS_REG_CONT);
        LL_ADC_ConfigOverSamplingRatioShift(adc,
                                            rhs_hal_adc_oversample_ratio[oversample],
                                            rhs_hal_adc_oversample_shift[oversample]);
    }

    // External channels use COMMON_1, internal ones need long sampling and use COMMON_2
    LL_ADC_SetSamplingTimeCommonChannels(adc, LL_ADC_SAMPLINGTIME_COMMON_1, rhs_hal_adc_sampling_time[sampling_time]);
    LL_ADC_SetSamplingTimeCommonChannels(adc, LL_ADC_SAMPLINGTIME_COMMON_2, LL_ADC_SAMPLINGTIME_160CYCLES_5);
    for (RHSHalAdcChannel channel = RHSHalAdcChannel0; channel < RHSHalAdcChannelNone; channel++)
    {
        LL_ADC_SetChannelSamplingTime(adc,
                                      rhs_hal_adc_channel_map[channel],
                                      rhs_hal_adc_is_internal_channel(channel) ? LL_ADC_SAMPLINGTIME_COMMON_2
                                                                               : LL_ADC_SAMPLINGTIME_COMMON_1);
    }

    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc),
                                   LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_PATH_INTERNAL_TEMPSENSOR |
                                       LL_ADC_PATH_INTERNAL_VBAT);

    // Enable ADC internal voltage regulator and wait for its stabilization
    LL_ADC_EnableInternalRegulator(adc);
    rhs_hal_cortex_delay_us(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);

    // Run ADC self calibration
    LL_ADC_StartCalibration(adc);
    rhs_hal_adc_wait(&adc->CR, ADC_CR_ADCAL, false, RHS_HAL_ADC_CALIB_TIMEOUT_US);
    // Delay between ADC end of calibration and ADC enable: 2 ADC clocks, 1 us is far longer
    rhs_hal_cortex_delay_us(1);

    // Enable ADC
    LL_ADC_ClearFlag_ADRDY(adc);
    LL_ADC_Enable(adc);
    rhs_hal_adc_wait(&adc->ISR, ADC_ISR_ADRDY, true, RHS_HAL_ADC_TIMEOUT_US);

    // Temperature sensor needs the longest time to stabilize after its path enabling
    rhs_hal_cortex_delay_us(LL_ADC_DELAY_TEMPSENSOR_STAB_US);

    // Reference is VDDA, measure it once with factory calibrated VREFINT
    handle->vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(rhs_hal_adc_read(handle, RHSHalAdcChannelVREFINT),
                                                       LL_ADC_RESOLUTION_12B);
}

uint16_t rhs_hal_adc_read(RHSHalAdcHandle* handle, RHSHalAdcChannel channel)
{
    rhs_assert(handle);
    rhs_assert(channel < RHSHalAdcChannelNone);

    ADC_TypeDef* adc = handle->adc;
    rhs_assert(LL_ADC_IsEnabled(adc) == 1);
    rhs_assert(LL_ADC_IsDisableOngoing(adc) == 0);
    rhs_assert(LL_ADC_REG_IsConversionOngoing(adc) == 0);

    // A new channel selection is applied only after CCRDY
    LL_ADC_ClearFlag_CCRDY(adc);
    LL_ADC_REG_SetSequencerChannels(adc, rhs_hal_adc_channel_map[channel]);
    rhs_hal_adc_wait(&adc->ISR, ADC_ISR_CCRDY, true, RHS_HAL_ADC_TIMEOUT_US);

    LL_ADC_ClearFlag_EOC(adc);
    LL_ADC_REG_StartConversion(adc);
    rhs_hal_adc_wait(&adc->ISR, ADC_ISR_EOC, true, RHS_HAL_ADC_TIMEOUT_US);

    return LL_ADC_REG_ReadConversionData12(adc);
}

float rhs_hal_adc_convert_to_voltage(RHSHalAdcHandle* handle, uint16_t value)
{
    rhs_assert(handle);
    // Float math instead of __LL_ADC_CALC_DATA_TO_VOLTAGE: it is integer and drops the fraction of mV
    return (float) value * (float) handle->vdda_mv / (float) __LL_ADC_DIGITAL_SCALE(LL_ADC_RESOLUTION_12B);
}

float rhs_hal_adc_convert_vref(RHSHalAdcHandle* handle, uint16_t value)
{
    UNUSED(handle);
    return (float) __LL_ADC_CALC_VREFANALOG_VOLTAGE(value, LL_ADC_RESOLUTION_12B);
}

float rhs_hal_adc_convert_temp(RHSHalAdcHandle* handle, uint16_t value)
{
    rhs_assert(handle);
    return (float) __LL_ADC_CALC_TEMPERATURE(handle->vdda_mv, value, LL_ADC_RESOLUTION_12B);
}

float rhs_hal_adc_convert_vbat(RHSHalAdcHandle* handle, uint16_t value)
{
    return rhs_hal_adc_convert_to_voltage(handle, value) * 3;
}
