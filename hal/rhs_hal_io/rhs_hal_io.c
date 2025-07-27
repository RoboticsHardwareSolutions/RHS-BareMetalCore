#include "rhs_hal_io.h"

#ifdef BMPLC_XL
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
#    define IN0_Pin GPIO_PIN_15
#    define IN0_GPIO_Port GPIOF
#    define IN1_Pin GPIO_PIN_2
#    define IN1_GPIO_Port GPIOB
#    define IN2_Pin GPIO_PIN_0
#    define IN2_GPIO_Port GPIOB
#    define IN3_Pin GPIO_PIN_1
#    define IN3_GPIO_Port GPIOB
#    define OUT0_Pin GPIO_PIN_14
#    define OUT0_GPIO_Port GPIOF
#    define OUT1_Pin GPIO_PIN_13
#    define OUT1_GPIO_Port GPIOF
#    define OUT2_Pin GPIO_PIN_12
#    define OUT2_GPIO_Port GPIOF
#    define OUT3_Pin GPIO_PIN_11
#    define OUT3_GPIO_Port GPIOF
#    define IN4_Pin GPIO_PIN_12
#    define IN4_GPIO_Port GPIOG

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

#endif

/** pin control **/
#define PIN_IS_HIGH(PORT, PIN, ...) (PORT->IDR & PIN) != 0
#define PIN_IS_LOW(PORT, PIN, ...) (PORT->IDR & PIN) == 0
#define PIN_HI(PORT, PIN...) (PORT->BSRR) = (PIN)
#define PIN_LOW(PORT, PIN...) (PORT->BSRR) = ((uint32_t) (PIN) << 16)

/** INs defines **/
bool IN0_IS_HIGH(void)
{
    return PIN_IS_HIGH(IN0_GPIO_Port, IN0_Pin);
}
bool IN1_IS_HIGH(void)
{
    return PIN_IS_HIGH(IN1_GPIO_Port, IN1_Pin);
}
bool IN2_IS_HIGH(void)
{
    return PIN_IS_HIGH(IN2_GPIO_Port, IN2_Pin);
}
bool IN3_IS_HIGH(void)
{
    return PIN_IS_HIGH(IN3_GPIO_Port, IN3_Pin);
}
bool IN4_IS_HIGH(void)
{
    return PIN_IS_HIGH(IN4_GPIO_Port, IN4_Pin);
}
bool IN0_IS_LOW(void)
{
    return PIN_IS_LOW(IN0_GPIO_Port, IN0_Pin);
}
bool IN1_IS_LOW(void)
{
    return PIN_IS_LOW(IN1_GPIO_Port, IN1_Pin);
}
bool IN2_IS_LOW(void)
{
    return PIN_IS_LOW(IN2_GPIO_Port, IN2_Pin);
}
bool IN3_IS_LOW(void)
{
    return PIN_IS_LOW(IN3_GPIO_Port, IN3_Pin);
}
bool IN4_IS_LOW(void)
{
    return PIN_IS_LOW(IN4_GPIO_Port, IN4_Pin);
}

/** OUTs defines **/
void OUT0_ON(void)
{
    PIN_HI(OUT0_GPIO_Port, OUT0_Pin);
}
void OUT0_OFF(void)
{
    PIN_LOW(OUT0_GPIO_Port, OUT0_Pin);
}
void OUT1_ON(void)
{
    PIN_HI(OUT1_GPIO_Port, OUT1_Pin);
}
void OUT1_OFF(void)
{
    PIN_LOW(OUT1_GPIO_Port, OUT1_Pin);
}
void OUT2_ON(void)
{
    PIN_HI(OUT2_GPIO_Port, OUT2_Pin);
}
void OUT2_OFF(void)
{
    PIN_LOW(OUT2_GPIO_Port, OUT2_Pin);
}
void OUT3_ON(void)
{
    PIN_HI(OUT3_GPIO_Port, OUT3_Pin);
}
void OUT3_OFF(void)
{
    PIN_LOW(OUT3_GPIO_Port, OUT3_Pin);
}
void OUT4_ON(void)
{
    PIN_HI(OUT4_GPIO_Port, OUT4_Pin);
}
void OUT4_OFF(void)
{
    PIN_LOW(OUT4_GPIO_Port, OUT4_Pin);
}

/** KEYs defines **/

void KEY0_ON(void)
{
    PIN_HI(KEY0_GPIO_Port, KEY0_Pin);
}
void KEY0_OFF(void)
{
    PIN_LOW(KEY0_GPIO_Port, KEY0_Pin);
}
void KEY1_ON(void)
{
    PIN_HI(KEY1_GPIO_Port, KEY1_Pin);
}
void KEY1_OFF(void)
{
    PIN_LOW(KEY1_GPIO_Port, KEY1_Pin);
}
void KEY2_ON(void)
{
    PIN_HI(KEY2_GPIO_Port, KEY2_Pin);
}
void KEY2_OFF(void)
{
    PIN_LOW(KEY2_GPIO_Port, KEY2_Pin);
}
void KEY3_ON(void)
{
    PIN_HI(KEY3_GPIO_Port, KEY3_Pin);
}
void KEY3_OFF(void)
{
    PIN_LOW(KEY3_GPIO_Port, KEY3_Pin);
}
void KEY4_ON(void)
{
    PIN_HI(KEY4_GPIO_Port, KEY4_Pin);
}
void KEY4_OFF(void)
{
    PIN_LOW(KEY4_GPIO_Port, KEY4_Pin);
}

void rhs_hal_io_init(void)
{
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
}
