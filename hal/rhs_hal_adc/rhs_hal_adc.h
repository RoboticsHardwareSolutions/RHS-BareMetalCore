/**
 * @file rhs_hal_adc.h
 * @brief ADC HAL API
 *
 * Small blocking subset of the ADC: single-ended, software triggered, one
 * channel per conversion, no DMA and no interrupts.
 *
 * Supported MCU: STM32G0B1.
 *
 * Couple things to keep in mind:
 *
 * - ADC resolution is 12 bits.
 * - Reference is VDDA (on small packages VREF+ is bonded to VDDA inside, so
 *   VREFBUF can not be used). VDDA is measured with VREFINT and its factory
 *   calibration on every rhs_hal_adc_configure, all conversions use it.
 * - ADC clock is the synchronous PCLK/2 or PCLK/4 (ADC max is 35 MHz).
 * - External channels use the configured sampling time, internal channels
 *   (VREFINT, TEMPSENSOR, VBAT) always use 160.5 ADC clocks.
 *
 * How to use:
 *
 * - rhs_hal_gpio_init_simple(&pin, RHSGpioModeAnalog) - configure your pins
 * - rhs_hal_adc_acquire - acquire ADC handle to work with
 * - rhs_hal_adc_configure - configure ADC block
 * - rhs_hal_adc_read - read value
 * - rhs_hal_adc_release - release ADC handle
 */
#pragma once
#include "stdbool.h"
#include "stdint.h"

typedef struct RHSHalAdcHandle RHSHalAdcHandle;

/**
 * ADC clock
 */
typedef enum
{
    RHSHalAdcClockSync16, /**< PCLK/4: 16 MHz at 64 MHz PCLK, synchronous */
    RHSHalAdcClockSync32, /**< PCLK/2: 32 MHz at 64 MHz PCLK, synchronous */
} RHSHalAdcClock;

/**
 * ADC oversampling
 */
typedef enum
{
    RHSHalAdcOversample2,    /**< ADC will take 2 samples per each value */
    RHSHalAdcOversample4,    /**< ADC will take 4 samples per each value */
    RHSHalAdcOversample8,    /**< ADC will take 8 samples per each value */
    RHSHalAdcOversample16,   /**< ADC will take 16 samples per each value */
    RHSHalAdcOversample32,   /**< ADC will take 32 samples per each value */
    RHSHalAdcOversample64,   /**< ADC will take 64 samples per each value */
    RHSHalAdcOversample128,  /**< ADC will take 128 samples per each value */
    RHSHalAdcOversample256,  /**< ADC will take 256 samples per each value */
    RHSHalAdcOversampleNone, /**< disable oversampling */
} RHSHalAdcOversample;

/**
 * ADC sampling time of external channels
 */
typedef enum
{
    RHSHalAdcSamplingtime1_5,   /**< Sampling time 1.5 ADC clock */
    RHSHalAdcSamplingtime3_5,   /**< Sampling time 3.5 ADC clock */
    RHSHalAdcSamplingtime7_5,   /**< Sampling time 7.5 ADC clock */
    RHSHalAdcSamplingtime12_5,  /**< Sampling time 12.5 ADC clock */
    RHSHalAdcSamplingtime19_5,  /**< Sampling time 19.5 ADC clock */
    RHSHalAdcSamplingtime39_5,  /**< Sampling time 39.5 ADC clock */
    RHSHalAdcSamplingtime79_5,  /**< Sampling time 79.5 ADC clock */
    RHSHalAdcSamplingtime160_5, /**< Sampling time 160.5 ADC clock */
} RHSHalAdcSamplingTime;

/**
 * ADC channels
 */
typedef enum
{
    RHSHalAdcChannel0,  /**< Channel 0 */
    RHSHalAdcChannel1,  /**< Channel 1 */
    RHSHalAdcChannel2,  /**< Channel 2 */
    RHSHalAdcChannel3,  /**< Channel 3 */
    RHSHalAdcChannel4,  /**< Channel 4 */
    RHSHalAdcChannel5,  /**< Channel 5 */
    RHSHalAdcChannel6,  /**< Channel 6 */
    RHSHalAdcChannel7,  /**< Channel 7 */
    RHSHalAdcChannel8,  /**< Channel 8 */
    RHSHalAdcChannel9,  /**< Channel 9 */
    RHSHalAdcChannel10, /**< Channel 10 */
    RHSHalAdcChannel11, /**< Channel 11 */
    RHSHalAdcChannel12, /**< Internal channel, see `RHSHalAdcChannelTEMPSENSOR` */
    RHSHalAdcChannel13, /**< Internal channel, see `RHSHalAdcChannelVREFINT` */
    RHSHalAdcChannel14, /**< Internal channel, see `RHSHalAdcChannelVBAT` */
    RHSHalAdcChannel15, /**< Channel 15 */
    RHSHalAdcChannel16, /**< Channel 16 */
    RHSHalAdcChannel17, /**< Channel 17 */
    RHSHalAdcChannel18, /**< Channel 18 */
    /* Special channels: internal peripherals */
    RHSHalAdcChannelVREFINT,    /**< Internal voltage reference, used for VDDA calculation */
    RHSHalAdcChannelTEMPSENSOR, /**< On-die temperature sensor */
    RHSHalAdcChannelVBAT,       /**< VBAT/3 voltage */
    /* Special value to indicate that pin is not connected to ADC */
    RHSHalAdcChannelNone, /**< No channel */
} RHSHalAdcChannel;

/** Initialize ADC subsystem */
void rhs_hal_adc_init(void);

/**
 * Acquire ADC handle
 *
 * Enables ADC clock
 *
 * @return RHSHalAdcHandle pointer
 */
RHSHalAdcHandle* rhs_hal_adc_acquire(void);

/**
 * Release ADC handle
 *
 * Disables ADC and its clock
 *
 * @param handle The ADC handle
 */
void rhs_hal_adc_release(RHSHalAdcHandle* handle);

/**
 * Configure with default parameters and enable ADC
 *
 * Parameters used:
 * - RHSHalAdcClockSync32 - PCLK/2, 32 MHz at 64 MHz PCLK.
 * - RHSHalAdcOversample64 - acquire and average 64 samples:
 *   (1/32)*(12.5+160.5)*64 = 346us per one read.
 * - RHSHalAdcSamplingtime160_5 - (1/32)*160.5 = 5us, for relatively high
 *   impedance circuits.
 *
 * @param handle The ADC handle
 */
void rhs_hal_adc_configure(RHSHalAdcHandle* handle);

/**
 * Configure with extended parameters and enable ADC
 *
 * @param handle        The ADC handle
 * @param clock         The ADC clock
 * @param oversample    The ADC oversample mode
 * @param sampling_time The ADC sampling time of external channels
 */
void rhs_hal_adc_configure_ex(RHSHalAdcHandle* handle,
                              RHSHalAdcClock clock,
                              RHSHalAdcOversample oversample,
                              RHSHalAdcSamplingTime sampling_time);

/**
 * Read single ADC value
 *
 * @param handle  The ADC handle
 * @param channel The channel to sample
 * @return value, 12 bits
 */
uint16_t rhs_hal_adc_read(RHSHalAdcHandle* handle, RHSHalAdcChannel channel);

/**
 * Convert sampled value to voltage
 *
 * @param handle The ADC handle
 * @param value  The value acquired with `rhs_hal_adc_read`
 * @return Voltage in mV
 */
float rhs_hal_adc_convert_to_voltage(RHSHalAdcHandle* handle, uint16_t value);

/**
 * Convert sampled VREFINT value to VDDA voltage
 *
 * @param handle The ADC handle
 * @param value  The value acquired with `rhs_hal_adc_read` for
 *               `RHSHalAdcChannelVREFINT` channel
 * @return Voltage in mV
 */
float rhs_hal_adc_convert_vref(RHSHalAdcHandle* handle, uint16_t value);

/**
 * Convert sampled TEMPSENSOR value to temperature
 *
 * @param handle The ADC handle
 * @param value  The value acquired with `rhs_hal_adc_read` for
 *               `RHSHalAdcChannelTEMPSENSOR` channel
 * @return Temperature in degree C
 */
float rhs_hal_adc_convert_temp(RHSHalAdcHandle* handle, uint16_t value);

/**
 * Convert sampled VBAT value to voltage
 *
 * @param handle The ADC handle
 * @param value  The value acquired with `rhs_hal_adc_read` for
 *               `RHSHalAdcChannelVBAT` channel
 * @return Voltage in mV
 */
float rhs_hal_adc_convert_vbat(RHSHalAdcHandle* handle, uint16_t value);
