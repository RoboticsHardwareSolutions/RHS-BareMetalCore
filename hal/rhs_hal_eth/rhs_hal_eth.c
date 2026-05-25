#include "rhs.h"
#include "rhs_hal.h"

void rhs_hal_eth_init(void)
{
// Initialise Ethernet. Enable MAC GPIO pins for STM32F407
#if defined(STM32F407xx) || defined(STM32F765xx)
    uint16_t pins[] = {PIN('A', 1),   // ETH_RMII_REF_CLK
                       PIN('A', 2),   // ETH_RMII_MDIO
                       PIN('A', 7),   // ETH_RMII_CRS_DV
                       PIN('B', 11),  // ETH_RMII_TX_EN
                       PIN('B', 12),  // ETH_RMII_TXD0
                       PIN('B', 13),  // ETH_RMII_TXD1
                       PIN('C', 1),   // ETH_RMII_MDC
                       PIN('C', 4),   // ETH_RMII_RXD0
                       PIN('C', 5)};  // ETH_RMII_RXD1
#else
#    error "Please define Ethernet pins for your STM32 model"
#endif
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        gpio_init(pins[i],
                  MG_GPIO_MODE_AF,
                  MG_GPIO_OTYPE_PUSH_PULL,
                  MG_GPIO_SPEED_INSANE,
                  MG_GPIO_PULL_NONE,
                  11);  // 11 is the Ethernet function
    }
    NVIC_EnableIRQ(ETH_IRQn);                // Setup Ethernet IRQ handler
    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;  // Use RMII. Goes first!

    // Enable Ethernet MAC clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN;

    // Perform hardware reset of Ethernet MAC peripheral
    RCC->AHB1RSTR |= RCC_AHB1RSTR_ETHMACRST;

    // Wait until reset is applied
    while ((RCC->AHB1RSTR & RCC_AHB1RSTR_ETHMACRST) == 0)
        ;

    // Release reset
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETHMACRST;

    // Wait until reset is released and MAC registers return to default values
    while ((RCC->AHB1RSTR & RCC_AHB1RSTR_ETHMACRST) != 0 || (ETH->MACCR & 0x00008000) == 0)
        ;
}

void rhs_hal_eth_deinit(void)
{
    // Disable Ethernet IRQ handler
    NVIC_DisableIRQ(ETH_IRQn);

    // Disable Ethernet MAC clocks
    RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN);
    SYSCFG->PMC &= ~SYSCFG_PMC_MII_RMII_SEL;  // Use RMII. Goes first!

    // Reset GPIO pins to default state
#if defined(STM32F407xx) || defined(STM32F765xx)
    uint16_t pins[] = {PIN('A', 1),   // ETH_RMII_REF_CLK
                       PIN('A', 2),   // ETH_RMII_MDIO
                       PIN('A', 7),   // ETH_RMII_CRS_DV
                       PIN('B', 11),  // ETH_RMII_TX_EN
                       PIN('B', 12),  // ETH_RMII_TXD0
                       PIN('B', 13),  // ETH_RMII_TXD1
                       PIN('C', 1),   // ETH_RMII_MDC
                       PIN('C', 4),   // ETH_RMII_RXD0
                       PIN('C', 5)};  // ETH_RMII_RXD1

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        gpio_init(pins[i],
                  MG_GPIO_MODE_INPUT,
                  MG_GPIO_OTYPE_PUSH_PULL,
                  MG_GPIO_SPEED_LOW,
                  MG_GPIO_PULL_NONE,
                  0);  // No alternate function
    }
#endif
}
