#include "io.h"
#include "rhs_hal_gpio.h"

#ifdef BMPLC_XL
#    include "stm32f7xx_hal.h"
static const RHSGpioPin key_pins[5] = {
    {GPIOE, LL_GPIO_PIN_4},
    {GPIOF, LL_GPIO_PIN_3},
    {GPIOF, LL_GPIO_PIN_2},
    {GPIOF, LL_GPIO_PIN_1},
    {GPIOF, LL_GPIO_PIN_0},
};
static const RHSGpioPin out_pins[5] = {
    {GPIOF, LL_GPIO_PIN_14},
    {GPIOF, LL_GPIO_PIN_13},
    {GPIOF, LL_GPIO_PIN_12},
    {GPIOF, LL_GPIO_PIN_11},
    {GPIOF, LL_GPIO_PIN_4},
};
static const RHSGpioPin in_pins[5] = {
    {GPIOF, LL_GPIO_PIN_15},
    {GPIOB, LL_GPIO_PIN_2},
    {GPIOB, LL_GPIO_PIN_1},
    {GPIOB, LL_GPIO_PIN_0},
    {GPIOG, LL_GPIO_PIN_12},
};

/* Board-revision strap, same pin rhs_hal_can.c checks for CAN1 pin-mux
 * (PG10 high -> newer revision routing CAN1 to PD0/PD1, low -> PA11/PA12).
 * IN0-4's electrical polarity also differs between the two revisions: on the
 * PG10=high revision, the raw level needs one more inversion on top of the
 * usual active-low interpretation. */
static const RHSGpioPin rev_strap_pin   = {GPIOG, LL_GPIO_PIN_10};
static bool              xl_rev_strap_high = false;

#elif BMPLC_L
#    include "stm32f7xx_hal.h"
#    define KEY0_Pin GPIO_PIN_4
#    define KEY0_GPIO_Port GPIOE
#    define KEY4_Pin GPIO_PIN_0
#    define KEY4_GPIO_Port GPIOF
#    define KEY3_Pin GPIO_PIN_1
#    define KEY3_GPIO_Port GPIOF
#    define KEY2_Pin GPIO_PIN_2
#    define KEY2_GPIO_Port GPIOF
#    define KEY1_Pin GPIO_PIN_3
#    define KEY1_GPIO_Port GPIOF
#    define OUT4_Pin GPIO_PIN_4
#    define OUT4_GPIO_Port GPIOF
#    define IN0_Pin GPIO_PIN_4
#    define IN0_GPIO_Port GPIOC
#    define IN1_Pin GPIO_PIN_5
#    define IN1_GPIO_Port GPIOC
#    define IN2_Pin GPIO_PIN_0
#    define IN2_GPIO_Port GPIOB
#    define IN3_Pin GPIO_PIN_1
#    define IN3_GPIO_Port GPIOB
#    define OUT0_Pin GPIO_PIN_12
#    define OUT0_GPIO_Port GPIOF
#    define OUT1_Pin GPIO_PIN_14
#    define OUT1_GPIO_Port GPIOF
#    define OUT2_Pin GPIO_PIN_0
#    define OUT2_GPIO_Port GPIOG
#    define OUT3_Pin GPIO_PIN_7
#    define OUT3_GPIO_Port GPIOE
#    define IN4_Pin GPIO_PIN_12
#    define IN4_GPIO_Port GPIOG

#elif BMPLC_M
#    include "stm32f1xx_hal.h"
static const RHSGpioPin key_pins[5] = {
    {GPIOC, LL_GPIO_PIN_3},
    {GPIOC, LL_GPIO_PIN_2},
    {GPIOC, LL_GPIO_PIN_1},
    {GPIOC, LL_GPIO_PIN_0},
    {GPIOC, LL_GPIO_PIN_13},
};
static const RHSGpioPin out_pins[5] = {
    {GPIOC, LL_GPIO_PIN_5},
    {GPIOC, LL_GPIO_PIN_4},
    {GPIOA, LL_GPIO_PIN_7},
    {GPIOA, LL_GPIO_PIN_6},
    {GPIOA, LL_GPIO_PIN_5},
};
static const RHSGpioPin in_pins[5] = {
    {GPIOA, LL_GPIO_PIN_0},
    {GPIOA, LL_GPIO_PIN_1},
    {GPIOA, LL_GPIO_PIN_2},
    {GPIOA, LL_GPIO_PIN_3},
    {GPIOA, LL_GPIO_PIN_4},
};

#endif

/* BMPLC_L keeps the original direct-register path (not migrated to rhs_hal_gpio). */
#if defined(BMPLC_L)
#    define PIN_IS_HIGH(PORT, PIN, ...) (PORT->IDR & PIN) != 0
#    define PIN_IS_LOW(PORT, PIN, ...) (PORT->IDR & PIN) == 0
#    define PIN_HI(PORT, PIN...) (PORT->BSRR) = (PIN)
#    define PIN_LOW(PORT, PIN...) (PORT->BSRR) = ((uint32_t) (PIN) << 16)
#endif

#if !defined(BMPLC_L)
static inline bool in_is_high(uint8_t ch)
{
    bool raw = rhs_hal_gpio_read(&in_pins[ch]);
#if defined(BMPLC_XL)
    return xl_rev_strap_high ? raw : !raw;
#else
    return !raw;
#endif
}
static inline bool in_is_low(uint8_t ch)
{
    return !in_is_high(ch);
}
static inline void out_on(uint8_t ch)
{
    rhs_hal_gpio_write(&out_pins[ch], true);
}
static inline void out_off(uint8_t ch)
{
    rhs_hal_gpio_write(&out_pins[ch], false);
}
static inline bool out_is_high(uint8_t ch)
{
    return rhs_hal_gpio_read(&out_pins[ch]);
}
static inline void key_on(uint8_t ch)
{
    rhs_hal_gpio_write(&key_pins[ch], true);
}
static inline void key_off(uint8_t ch)
{
    rhs_hal_gpio_write(&key_pins[ch], false);
}
static inline bool key_is_open(uint8_t ch)
{
    return rhs_hal_gpio_read(&key_pins[ch]);
}
#endif

/** INs defines **/
/* IN0-4 are driven by an active-low input stage (external pull-up + sinking source):
 * electrically LOW means the input signal is asserted, electrically HIGH is the
 * unasserted/floating default. IS_HIGH()/IS_LOW() here report the logical signal
 * state, not the raw electrical level -- hence the inversion below. */
bool IN0_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_LOW(IN0_GPIO_Port, IN0_Pin);
#else
    return in_is_high(0);
#endif
}
bool IN1_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_LOW(IN1_GPIO_Port, IN1_Pin);
#else
    return in_is_high(1);
#endif
}
bool IN2_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_LOW(IN2_GPIO_Port, IN2_Pin);
#else
    return in_is_high(2);
#endif
}
bool IN3_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_LOW(IN3_GPIO_Port, IN3_Pin);
#else
    return in_is_high(3);
#endif
}
bool IN4_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_LOW(IN4_GPIO_Port, IN4_Pin);
#else
    return in_is_high(4);
#endif
}
bool IN0_IS_LOW(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(IN0_GPIO_Port, IN0_Pin);
#else
    return in_is_low(0);
#endif
}
bool IN1_IS_LOW(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(IN1_GPIO_Port, IN1_Pin);
#else
    return in_is_low(1);
#endif
}
bool IN2_IS_LOW(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(IN2_GPIO_Port, IN2_Pin);
#else
    return in_is_low(2);
#endif
}
bool IN3_IS_LOW(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(IN3_GPIO_Port, IN3_Pin);
#else
    return in_is_low(3);
#endif
}
bool IN4_IS_LOW(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(IN4_GPIO_Port, IN4_Pin);
#else
    return in_is_low(4);
#endif
}

/** OUTs defines **/
void OUT0_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(OUT0_GPIO_Port, OUT0_Pin);
#else
    out_on(0);
#endif
}
void OUT0_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(OUT0_GPIO_Port, OUT0_Pin);
#else
    out_off(0);
#endif
}
void OUT1_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(OUT1_GPIO_Port, OUT1_Pin);
#else
    out_on(1);
#endif
}
void OUT1_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(OUT1_GPIO_Port, OUT1_Pin);
#else
    out_off(1);
#endif
}
void OUT2_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(OUT2_GPIO_Port, OUT2_Pin);
#else
    out_on(2);
#endif
}
void OUT2_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(OUT2_GPIO_Port, OUT2_Pin);
#else
    out_off(2);
#endif
}
void OUT3_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(OUT3_GPIO_Port, OUT3_Pin);
#else
    out_on(3);
#endif
}
void OUT3_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(OUT3_GPIO_Port, OUT3_Pin);
#else
    out_off(3);
#endif
}
void OUT4_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(OUT4_GPIO_Port, OUT4_Pin);
#else
    out_on(4);
#endif
}
void OUT4_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(OUT4_GPIO_Port, OUT4_Pin);
#else
    out_off(4);
#endif
}

bool OUT0_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(OUT0_GPIO_Port, OUT0_Pin);
#else
    return out_is_high(0);
#endif
}
bool OUT1_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(OUT1_GPIO_Port, OUT1_Pin);
#else
    return out_is_high(1);
#endif
}
bool OUT2_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(OUT2_GPIO_Port, OUT2_Pin);
#else
    return out_is_high(2);
#endif
}
bool OUT3_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(OUT3_GPIO_Port, OUT3_Pin);
#else
    return out_is_high(3);
#endif
}
bool OUT4_IS_HIGH(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(OUT4_GPIO_Port, OUT4_Pin);
#else
    return out_is_high(4);
#endif
}

/** KEYs defines **/

void KEY0_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(KEY0_GPIO_Port, KEY0_Pin);
#else
    key_on(0);
#endif
}
void KEY0_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(KEY0_GPIO_Port, KEY0_Pin);
#else
    key_off(0);
#endif
}
void KEY1_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(KEY1_GPIO_Port, KEY1_Pin);
#else
    key_on(1);
#endif
}
void KEY1_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(KEY1_GPIO_Port, KEY1_Pin);
#else
    key_off(1);
#endif
}
void KEY2_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(KEY2_GPIO_Port, KEY2_Pin);
#else
    key_on(2);
#endif
}
void KEY2_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(KEY2_GPIO_Port, KEY2_Pin);
#else
    key_off(2);
#endif
}
void KEY3_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(KEY3_GPIO_Port, KEY3_Pin);
#else
    key_on(3);
#endif
}
void KEY3_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(KEY3_GPIO_Port, KEY3_Pin);
#else
    key_off(3);
#endif
}
void KEY4_ON(void)
{
#if defined(BMPLC_L)
    PIN_HI(KEY4_GPIO_Port, KEY4_Pin);
#else
    key_on(4);
#endif
}
void KEY4_OFF(void)
{
#if defined(BMPLC_L)
    PIN_LOW(KEY4_GPIO_Port, KEY4_Pin);
#else
    key_off(4);
#endif
}

bool KEY0_IS_OPEN(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(KEY0_GPIO_Port, KEY0_Pin);
#else
    return key_is_open(0);
#endif
}

bool KEY1_IS_OPEN(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(KEY1_GPIO_Port, KEY1_Pin);
#else
    return key_is_open(1);
#endif
}

bool KEY2_IS_OPEN(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(KEY2_GPIO_Port, KEY2_Pin);
#else
    return key_is_open(2);
#endif
}

bool KEY3_IS_OPEN(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(KEY3_GPIO_Port, KEY3_Pin);
#else
    return key_is_open(3);
#endif
}

bool KEY4_IS_OPEN(void)
{
#if defined(BMPLC_L)
    return PIN_IS_HIGH(KEY4_GPIO_Port, KEY4_Pin);
#else
    return key_is_open(4);
#endif
}

void io_init(void)
{
#if defined(BMPLC_XL)
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    rhs_hal_gpio_init(&rev_strap_pin, RHSGpioModeInput, RHSGpioPullDown, RHSGpioSpeedLow);
    xl_rev_strap_high = rhs_hal_gpio_read(&rev_strap_pin);

    for (uint8_t ch = 0; ch < 5; ch++)
    {
        rhs_hal_gpio_write(&key_pins[ch], false);
        rhs_hal_gpio_write(&out_pins[ch], false);
        rhs_hal_gpio_init_simple(&key_pins[ch], RHSGpioModeOutputPushPull);
        rhs_hal_gpio_init_simple(&out_pins[ch], RHSGpioModeOutputPushPull);
        rhs_hal_gpio_init_simple(&in_pins[ch], RHSGpioModeInput);
    }
#elif defined(BMPLC_L)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOE, KEY0_Pin | OUT3_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOF,
                      KEY4_Pin | KEY3_Pin | KEY2_Pin | KEY1_Pin | OUT4_Pin | OUT0_Pin | OUT1_Pin,
                      GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(OUT2_GPIO_Port, OUT2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : PEPin PEPin */
    GPIO_InitStruct.Pin   = KEY0_Pin | OUT3_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /*Configure GPIO pins : PFPin PFPin PFPin PFPin
                             PFPin PFPin PFPin */
    GPIO_InitStruct.Pin   = KEY4_Pin | KEY3_Pin | KEY2_Pin | KEY1_Pin | OUT4_Pin | OUT0_Pin | OUT1_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /*Configure GPIO pins : PCPin PCPin */
    GPIO_InitStruct.Pin  = IN0_Pin | IN1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : PBPin PBPin */
    GPIO_InitStruct.Pin  = IN2_Pin | IN3_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin   = OUT2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(OUT2_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin  = IN4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(IN4_GPIO_Port, &GPIO_InitStruct);
#elif defined(BMPLC_M)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    for (uint8_t ch = 0; ch < 5; ch++)
    {
        rhs_hal_gpio_write(&key_pins[ch], false);
        rhs_hal_gpio_write(&out_pins[ch], false);
        rhs_hal_gpio_init_simple(&key_pins[ch], RHSGpioModeOutputPushPull);
        rhs_hal_gpio_init_simple(&out_pins[ch], RHSGpioModeOutputPushPull);
        rhs_hal_gpio_init_simple(&in_pins[ch], RHSGpioModeInput);
    }
#endif
}
