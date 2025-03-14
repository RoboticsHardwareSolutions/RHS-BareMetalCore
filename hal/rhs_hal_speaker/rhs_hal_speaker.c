#include "rhs_hal_speaker.h"
#include "rhs.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "semphr.h"

static RHSMutex* rhs_hal_speaker_mutex = NULL;

#if defined(RPLC_XL) || defined(RPLC_L)
#    include "stm32f7xx_hal.h"
TIM_HandleTypeDef htim;

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM9)
    {
        __HAL_RCC_TIM9_CLK_ENABLE();
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (timHandle->Instance == TIM9)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        GPIO_InitStruct.Pin       = GPIO_PIN_5;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM9)
    {
        __HAL_RCC_TIM9_CLK_DISABLE();
    }
}

static void tim_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_OC_InitTypeDef     sConfigOC          = {0};

    htim.Instance               = TIM9;
    htim.Init.Prescaler         = 215;
    htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim.Init.Period            = 376;
    htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim) != HAL_OK)
    {
        rhs_crash("Speaker init failed");
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim, &sClockSourceConfig) != HAL_OK)
    {
        rhs_crash("Speaker init failed");
    }
    if (HAL_TIM_PWM_Init(&htim) != HAL_OK)
    {
        rhs_crash("Speaker init failed");
    }
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        rhs_crash("Speaker init failed");
    }
    HAL_TIM_MspPostInit(&htim);
}
#else
#    include "stm32f1xx_hal.h"
TIM_HandleTypeDef htim;

void HAL_TIM_OC_MspInit(TIM_HandleTypeDef* htim_oc)
{
    if (htim_oc->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim_handle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (htim_handle->Instance == TIM3)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin   = GPIO_PIN_5;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        __HAL_AFIO_REMAP_TIM3_PARTIAL();
    }
}

void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef* htim_oc)
{
    if (htim_oc->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_DISABLE();
    }
}
static void tim_init(void)
{
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};
    TIM_OC_InitTypeDef      sConfigOC          = {0};
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* USER CODE BEGIN TIM3_Init 1 */

    /* USER CODE END TIM3_Init 1 */
    htim.Instance               = TIM3;
    htim.Init.Prescaler         = 71;
    htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim.Init.Period            = 376;
    htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim) != HAL_OK)
    {
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim, &sClockSourceConfig) != HAL_OK)
    {
    }
    if (HAL_TIM_PWM_Init(&htim) != HAL_OK)
    {
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig) != HAL_OK)
    {
    }
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
    }
    HAL_TIM_MspPostInit(&htim);
}
#endif

void rhs_hal_speaker_init(void)
{
    rhs_assert(rhs_hal_speaker_mutex == NULL);
    tim_init();
    rhs_hal_speaker_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
}

void rhs_hal_speaker_deinit(void)
{
    rhs_assert(rhs_hal_speaker_mutex != NULL);
    rhs_mutex_free(rhs_hal_speaker_mutex);
    rhs_hal_speaker_mutex = NULL;
}

static inline uint32_t rhs_hal_speaker_calculate_autoreload(float frequency)
{
    uint32_t autoreload = (SystemCoreClock / htim.Init.Prescaler / frequency) - 1;
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

    uint32_t compare_value = volume * htim.Instance->ARR / 2;

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
#if defined(RPLC_XL) || defined(RPLC_L)
    htim.Init.Period    = rhs_hal_speaker_calculate_autoreload(frequency);
    htim.Instance->ARR  = htim.Init.Period;
    htim.Instance->CCR1 = rhs_hal_speaker_calculate_compare(volume);

    HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1);
#elif defined(RPLC_M)
    htim.Init.Period    = rhs_hal_speaker_calculate_autoreload(frequency);
    htim.Instance->ARR  = htim.Init.Period;
    htim.Instance->CCR2 = rhs_hal_speaker_calculate_compare(volume);

    HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_2);
#else
#    error "Unknown platform"
#endif
}

void rhs_hal_speaker_stop(void)
{
#if defined(RPLC_XL) || defined(RPLC_L)
    HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
#elif defined(RPLC_M)
    HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_2);
#else
#    error "Unknown platform"
#endif
}
