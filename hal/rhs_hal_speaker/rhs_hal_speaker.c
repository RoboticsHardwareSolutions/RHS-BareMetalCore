#include "rhs_hal_speaker.h"
#include "rhs.h"
#include "tim.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "semphr.h"

static TIM_HandleTypeDef* tim_handler;

static RHSMutex* rhs_hal_speaker_mutex = NULL;

void rhs_hal_speaker_init(void)
{
    rhs_assert(rhs_hal_speaker_mutex == NULL);
    rhs_hal_speaker_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
    tim_handler           = &htim9;
}

void rhs_hal_speaker_deinit(void)
{
    rhs_assert(rhs_hal_speaker_mutex != NULL);
    rhs_mutex_free(rhs_hal_speaker_mutex);
    rhs_hal_speaker_mutex = NULL;
}

static inline uint32_t rhs_hal_speaker_calculate_autoreload(float frequency)
{
    uint32_t autoreload = (SystemCoreClock / tim_handler->Init.Prescaler / frequency) - 1;
    if (autoreload < 2)
    {
        autoreload = 2;
    }
    else if (autoreload > UINT16_MAX)
    {
        autoreload = UINT16_MAX;
    }

    return autoreload;
}

static inline uint32_t rhs_hal_speaker_calculate_compare(float volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > 1)
        volume = 1;
    volume = volume * volume * volume;

    uint32_t compare_value = volume * tim_handler->Instance->ARR / 2;

    if (compare_value == 0)
    {
        compare_value = 1;
    }

    return compare_value;
}

bool rhs_hal_speaker_acquire(uint32_t timeout)
{
    rhs_assert(!RHS_IS_IRQ_MODE());

    if (rhs_mutex_acquire(rhs_hal_speaker_mutex, timeout) == RHSStatusOk)
    {
        // TODO set chanel here
        return true;
    }
    else
    {
        return false;
    }
}

void rhs_hal_speaker_release(void)
{
    rhs_assert(!RHS_IS_IRQ_MODE());
    rhs_assert(rhs_hal_speaker_is_mine());

    rhs_hal_speaker_stop();
    // TODO reset chanel here

    rhs_assert(rhs_mutex_release(rhs_hal_speaker_mutex) == RHSStatusOk);
}

bool rhs_hal_speaker_is_mine(void)
{
    return (RHS_IS_IRQ_MODE()) || (rhs_mutex_get_owner(rhs_hal_speaker_mutex) == xTaskGetCurrentTaskHandle());
}

void rhs_hal_speaker_start(float frequency, float volume)
{
    htim9.Init.Period           = rhs_hal_speaker_calculate_autoreload(frequency);
    tim_handler->Instance->ARR  = htim9.Init.Period;
    tim_handler->Instance->CCR1 = rhs_hal_speaker_calculate_compare(volume);

    HAL_TIM_PWM_Start(tim_handler, TIM_CHANNEL_1);
}

void rhs_hal_speaker_stop(void)
{
    HAL_TIM_PWM_Stop(tim_handler, TIM_CHANNEL_1);
}
