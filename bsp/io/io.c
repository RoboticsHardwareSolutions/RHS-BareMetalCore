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

/** INs defines **/
/* IN0-4 are driven by an active-low input stage (external pull-up + sinking source):
 * electrically LOW means the input signal is asserted, electrically HIGH is the
 * unasserted/floating default. IS_HIGH()/IS_LOW() here report the logical signal
 * state, not the raw electrical level -- hence the inversion below. */
bool IN0_IS_HIGH(void)
{
    return in_is_high(0);
}
bool IN1_IS_HIGH(void)
{
    return in_is_high(1);
}
bool IN2_IS_HIGH(void)
{
    return in_is_high(2);
}
bool IN3_IS_HIGH(void)
{
    return in_is_high(3);
}
bool IN4_IS_HIGH(void)
{
    return in_is_high(4);
}
bool IN0_IS_LOW(void)
{
    return in_is_low(0);
}
bool IN1_IS_LOW(void)
{
    return in_is_low(1);
}
bool IN2_IS_LOW(void)
{
    return in_is_low(2);
}
bool IN3_IS_LOW(void)
{
    return in_is_low(3);
}
bool IN4_IS_LOW(void)
{
    return in_is_low(4);
}

/** OUTs defines **/
void OUT0_ON(void)
{
    out_on(0);
}
void OUT0_OFF(void)
{
    out_off(0);
}
void OUT1_ON(void)
{
    out_on(1);
}
void OUT1_OFF(void)
{
    out_off(1);
}
void OUT2_ON(void)
{
    out_on(2);
}
void OUT2_OFF(void)
{
    out_off(2);
}
void OUT3_ON(void)
{
    out_on(3);
}
void OUT3_OFF(void)
{
    out_off(3);
}
void OUT4_ON(void)
{
    out_on(4);
}
void OUT4_OFF(void)
{
    out_off(4);
}

bool OUT0_IS_HIGH(void)
{
    return out_is_high(0);
}
bool OUT1_IS_HIGH(void)
{
    return out_is_high(1);
}
bool OUT2_IS_HIGH(void)
{
    return out_is_high(2);
}
bool OUT3_IS_HIGH(void)
{
    return out_is_high(3);
}
bool OUT4_IS_HIGH(void)
{
    return out_is_high(4);
}

/** KEYs defines **/

void KEY0_ON(void)
{
    key_on(0);
}
void KEY0_OFF(void)
{
    key_off(0);
}
void KEY1_ON(void)
{
    key_on(1);
}
void KEY1_OFF(void)
{
    key_off(1);
}
void KEY2_ON(void)
{
    key_on(2);
}
void KEY2_OFF(void)
{
    key_off(2);
}
void KEY3_ON(void)
{
    key_on(3);
}
void KEY3_OFF(void)
{
    key_off(3);
}
void KEY4_ON(void)
{
    key_on(4);
}
void KEY4_OFF(void)
{
    key_off(4);
}

bool KEY0_IS_OPEN(void)
{
    return key_is_open(0);
}

bool KEY1_IS_OPEN(void)
{
    return key_is_open(1);
}

bool KEY2_IS_OPEN(void)
{
    return key_is_open(2);
}

bool KEY3_IS_OPEN(void)
{
    return key_is_open(3);
}

bool KEY4_IS_OPEN(void)
{
    return key_is_open(4);
}

void io_init(void)
{
#if defined(BMPLC_XL)
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
#elif defined(BMPLC_M)
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

void io_in_exti_enable(uint8_t ch, RHSGpioExtiCallback callback, void* context)
{
    rhs_assert(ch < 5);
    rhs_hal_gpio_init_ex(&in_pins[ch], RHSGpioModeInterruptRiseFall, RHSGpioPullNo, RHSGpioSpeedLow, RHSGpioAltFnUnused);
    rhs_hal_gpio_add_int_callback(&in_pins[ch], callback, context);
}
