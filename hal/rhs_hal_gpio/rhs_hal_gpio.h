#pragma once

#if defined(STM32F103xE)
#    include <stm32f103xe.h>
#    include <stm32f1xx.h>
#elif defined(STM32F407xx)
#    include <stm32f407xx.h>
#elif defined(STM32F765xx)
#    include <stm32f765xx.h>
#else
#    error "Device not specified for rhs_hal_gpio"
#endif

#include <stdbool.h>
#include <stdint.h>

// ─── Common pin macros ────────────────────────────────────────────────────────

#define BIT(x) (1UL << (x))
#define SETBITS(R, CLEARMASK, SETMASK) (R) = ((R) & ~(CLEARMASK)) | (SETMASK)
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8)

// ─── GPIO base address ────────────────────────────────────────────────────────

#if defined(STM32F407xx) || defined(STM32F765xx)
#    define GPIO(N) ((GPIO_TypeDef*) (0x40020000 + 0x400 * (N)))
#else /* STM32F1 */
#    define GPIO(N) ((GPIO_TypeDef*) (GPIOA_BASE + 0x400 * (N)))
#endif

// ─── Common register-level helpers ───────────────────────────────────────────

static inline GPIO_TypeDef* gpio_bank(uint16_t pin)
{
    return GPIO(PINBANK(pin));
}

static inline void gpio_toggle(uint16_t pin)
{
    GPIO_TypeDef* gpio = gpio_bank(pin);
    uint32_t      mask = BIT(PINNO(pin));
    gpio->BSRR         = mask << (gpio->ODR & mask ? 16 : 0);
}

static inline int gpio_read(uint16_t pin)
{
    return gpio_bank(pin)->IDR & BIT(PINNO(pin)) ? 1 : 0;
}

static inline void gpio_write(uint16_t pin, bool val)
{
    GPIO_TypeDef* gpio = gpio_bank(pin);
    gpio->BSRR         = BIT(PINNO(pin)) << (val ? 0 : 16);
}

// ─── STM32F4 / STM32F7 ───────────────────────────────────────────────────────
#if defined(STM32F407xx) || defined(STM32F765xx)

enum
{
    MG_GPIO_MODE_INPUT,
    MG_GPIO_MODE_OUTPUT,
    MG_GPIO_MODE_AF,
    MG_GPIO_MODE_ANALOG
};
enum
{
    MG_GPIO_OTYPE_PUSH_PULL,
    MG_GPIO_OTYPE_OPEN_DRAIN
};
enum
{
    MG_GPIO_SPEED_LOW,
    MG_GPIO_SPEED_MEDIUM,
    MG_GPIO_SPEED_HIGH,
    MG_GPIO_SPEED_INSANE
};
enum
{
    MG_GPIO_PULL_NONE,
    MG_GPIO_PULL_UP,
    MG_GPIO_PULL_DOWN
};

static inline void gpio_init(uint16_t pin, uint8_t mode, uint8_t type, uint8_t speed, uint8_t pull, uint8_t af)
{
    GPIO_TypeDef* gpio = gpio_bank(pin);
    uint8_t       n    = (uint8_t) (PINNO(pin));
    RCC->AHB1ENR |= BIT(PINBANK(pin));  // Enable GPIO clock
    SETBITS(gpio->OTYPER, 1UL << n, ((uint32_t) type) << n);
    SETBITS(gpio->OSPEEDR, 3UL << (n * 2), ((uint32_t) speed) << (n * 2));
    SETBITS(gpio->PUPDR, 3UL << (n * 2), ((uint32_t) pull) << (n * 2));
    SETBITS(gpio->AFR[n >> 3], 15UL << ((n & 7) * 4), ((uint32_t) af) << ((n & 7) * 4));
    SETBITS(gpio->MODER, 3UL << (n * 2), ((uint32_t) mode) << (n * 2));
}

static inline void gpio_input(uint16_t pin)
{
    gpio_init(pin, MG_GPIO_MODE_INPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);
}

static inline void gpio_output(uint16_t pin)
{
    gpio_init(pin, MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);
}

// ─── STM32F1 ─────────────────────────────────────────────────────────────────
#elif defined(STM32F103xE)

// System clock configuration for STM32F103xE
// Max frequency: 72 MHz using external 8 MHz HSE crystal
// Flash latency: 2 WS for 48 MHz < SYSCLK <= 72 MHz
enum
{
    APB1_PRE = 4 /* AHB / 2 = 36 MHz */,
    APB2_PRE = 0 /* AHB / 1 = 72 MHz */
};
enum
{
    PLL_HSE = 8,
    PLL_MUL = 9
};  // 8 MHz * 9 = 72 MHz
#    define FLASH_LATENCY 2
#    define SYS_FREQUENCY (PLL_HSE * PLL_MUL * 1000000)  // Core 72 MHz
#    define APB2_FREQUENCY (SYS_FREQUENCY / (1 << APB2_PRE))
#    define APB1_FREQUENCY (SYS_FREQUENCY / (1 << (APB1_PRE - 3)))

static inline void spin(volatile uint32_t count)
{
    while (count--)
        (void) 0;
}

// GPIO modes for STM32F1 (CNF + MODE fields packed into 4 bits per pin)
enum
{
    GPIO_MODE_INPUT_ANALOG   = 0x0,
    GPIO_MODE_INPUT_FLOATING = 0x4,
    GPIO_MODE_INPUT_PULLUP   = 0x8
};
enum
{
    GPIO_MODE_OUTPUT_PP_10MHZ = 0x1,
    GPIO_MODE_OUTPUT_OD_10MHZ = 0x5
};
enum
{
    GPIO_MODE_OUTPUT_PP_2MHZ = 0x2,
    GPIO_MODE_OUTPUT_OD_2MHZ = 0x6
};
enum
{
    GPIO_MODE_OUTPUT_PP_50MHZ = 0x3,
    GPIO_MODE_OUTPUT_OD_50MHZ = 0x7
};
enum
{
    GPIO_MODE_AF_PP_10MHZ = 0x9,
    GPIO_MODE_AF_OD_10MHZ = 0xD
};
enum
{
    GPIO_MODE_AF_PP_2MHZ = 0xA,
    GPIO_MODE_AF_OD_2MHZ = 0xE
};
enum
{
    GPIO_MODE_AF_PP_50MHZ = 0xB,
    GPIO_MODE_AF_OD_50MHZ = 0xF
};

static inline void gpio_init(uint16_t pin, uint8_t mode)
{
    GPIO_TypeDef* gpio = gpio_bank(pin);
    uint8_t       n    = (uint8_t) (PINNO(pin));
    RCC->APB2ENR |= BIT(PINBANK(pin) + 2);  // GPIOA = bit 2, GPIOB = bit 3, ...

    if (n < 8)
    {
        SETBITS(gpio->CRL, 15UL << (n * 4), ((uint32_t) mode) << (n * 4));
    }
    else
    {
        SETBITS(gpio->CRH, 15UL << ((n - 8) * 4), ((uint32_t) mode) << ((n - 8) * 4));
    }

    if (mode == GPIO_MODE_INPUT_PULLUP)
    {
        gpio->BSRR = BIT(n);  // Activate internal pull-up
    }
}

static inline void gpio_input(uint16_t pin)
{
    gpio_init(pin, GPIO_MODE_INPUT_FLOATING);
}

static inline void gpio_output(uint16_t pin)
{
    gpio_init(pin, GPIO_MODE_OUTPUT_PP_50MHZ);
}

static inline void irq_exti_attach(uint16_t pin)
{
    uint8_t bank = (uint8_t) (PINBANK(pin)), n = (uint8_t) (PINNO(pin));
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;  // Enable AFIO clock
    AFIO->EXTICR[n / 4] &= ~(15UL << ((n % 4) * 4));
    AFIO->EXTICR[n / 4] |= (uint32_t) (bank << ((n % 4) * 4));
    EXTI->IMR |= BIT(n);
    EXTI->RTSR |= BIT(n);
    EXTI->FTSR |= BIT(n);
    int irqvec = n < 5 ? 6 + n : n < 10 ? 23 : 40;
    NVIC_SetPriority(irqvec, 3);
    NVIC_EnableIRQ(irqvec);
}

#endif /* device family */
