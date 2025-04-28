#include <rhs_hal_interrupt.h>

#ifdef STM32F765xx
#    include "stm32f765xx.h"
#    include "stm32f7xx_ll_rcc.h"
#elif STM32F407xx
#    include "stm32f407xx.h"
#    include "stm32f4xx_ll_rcc.h"
#elif STM32F103xE
#    include "stm32f103xe.h"
#    include "stm32f1xx_ll_rcc.h"
#endif
#include "rhs.h"

#define RHS_HAL_INTERRUPT_DEFAULT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5)

#define RHS_HAL_INTERRUPT_ACCOUNT_START() const uint32_t _isr_start = DWT->CYCCNT;
#define RHS_HAL_INTERRUPT_ACCOUNT_END()                     \
    const uint32_t _time_in_isr = DWT->CYCCNT - _isr_start; \
    rhs_hal_interrupt.counter_time_in_isr_total += _time_in_isr;

typedef struct
{
    RHSHalInterruptISR isr;
    void*              context;
} RHSHalInterruptISRPair;

typedef struct
{
    RHSHalInterruptISRPair isr[RHSHalInterruptIdMax];
    uint32_t               counter_time_in_isr_total;
} RHSHalIterrupt;

static RHSHalIterrupt rhs_hal_interrupt = {};

const IRQn_Type rhs_hal_interrupt_irqn[RHSHalInterruptIdMax] = {
#ifdef STM32F765xx
    /* CAN */
    [RHSHalInterruptIdCAN1Rx0] = CAN1_RX0_IRQn,
    [RHSHalInterruptIdCAN1SCE] = CAN1_SCE_IRQn,
    [RHSHalInterruptIdCAN1Tx]  = CAN1_TX_IRQn,
#    if !defined(RPLC_XL) && !defined(RPLC_L)
    [RHSHalInterruptIdCAN2Rx0] = CAN2_RX0_IRQn,
    [RHSHalInterruptIdCAN2SCE] = CAN2_SCE_IRQn,
    [RHSHalInterruptIdCAN2Tx]  = CAN2_TX_IRQn,
#    endif
    /* UART */
    [RHSHalInterruptIdUsart3] = USART3_IRQn,
    [RHSHalInterruptIdUart5]  = UART5_IRQn,
    [RHSHalInterruptIdUsart6] = USART6_IRQn,

    /* DMA */
    [RHSHalInterruptIdDMA1Stream3] = DMA1_Stream3_IRQn,
    [RHSHalInterruptIdDMA2Stream1] = DMA2_Stream1_IRQn,
    [RHSHalInterruptIdDMA2Stream6] = DMA2_Stream6_IRQn,

#elif STM32F407xx
    /* CAN */
    [RHSHalInterruptIdCAN1Rx0] = CAN1_RX0_IRQn,
    [RHSHalInterruptIdCAN1SCE] = CAN1_SCE_IRQn,
    [RHSHalInterruptIdCAN1Tx]  = CAN1_TX_IRQn,
    [RHSHalInterruptIdCAN2Rx0] = CAN2_RX0_IRQn,
    [RHSHalInterruptIdCAN2SCE] = CAN2_SCE_IRQn,
    [RHSHalInterruptIdCAN2Tx]  = CAN2_TX_IRQn,

#elif defined(STM32F103xE)
    /* UART */
    [RHSHalInterruptIdUsart3] = USART3_IRQn,
    
#endif
};

__attribute__((always_inline)) inline static void rhs_hal_interrupt_call(RHSHalInterruptId index)
{
    const RHSHalInterruptISRPair* isr_descr = &rhs_hal_interrupt.isr[index];
    rhs_assert(isr_descr->isr);

    RHS_HAL_INTERRUPT_ACCOUNT_START();
    isr_descr->isr(isr_descr->context);
    RHS_HAL_INTERRUPT_ACCOUNT_END();
}

__attribute__((always_inline)) inline static void rhs_hal_interrupt_enable(RHSHalInterruptId index, uint16_t priority)
{
    NVIC_SetPriority(rhs_hal_interrupt_irqn[index], NVIC_EncodePriority(NVIC_GetPriorityGrouping(), priority, 0));
    NVIC_EnableIRQ(rhs_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) inline static void rhs_hal_interrupt_clear_pending(RHSHalInterruptId index)
{
    NVIC_ClearPendingIRQ(rhs_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) inline static void rhs_hal_interrupt_get_pending(RHSHalInterruptId index)
{
    NVIC_GetPendingIRQ(rhs_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) inline static void rhs_hal_interrupt_set_pending(RHSHalInterruptId index)
{
    NVIC_SetPendingIRQ(rhs_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) inline static void rhs_hal_interrupt_disable(RHSHalInterruptId index)
{
    NVIC_DisableIRQ(rhs_hal_interrupt_irqn[index]);
}

void rhs_hal_interrupt_init(void)
{
    NVIC_SetPriority(SVCall_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
}

void rhs_hal_interrupt_set_isr(RHSHalInterruptId index, RHSHalInterruptISR isr, void* context)
{
    rhs_hal_interrupt_set_isr_ex(index, RHSHalInterruptPriorityNormal, isr, context);
}

void rhs_hal_interrupt_set_isr_ex(RHSHalInterruptId       index,
                                  RHSHalInterruptPriority priority,
                                  RHSHalInterruptISR      isr,
                                  void*                   context)
{
    rhs_assert(index < RHSHalInterruptIdMax);
    rhs_assert((priority >= RHSHalInterruptPriorityLowest && priority <= RHSHalInterruptPriorityHighest) ||
               priority == RHSHalInterruptPriorityKamiSama);

    uint16_t real_priority = RHS_HAL_INTERRUPT_DEFAULT_PRIORITY - priority;

    RHSHalInterruptISRPair* isr_descr = &rhs_hal_interrupt.isr[index];
    if (isr)
    {
        // Pre ISR set
        rhs_assert(isr_descr->isr == NULL);
    }
    else
    {
        // Pre ISR clear
        rhs_hal_interrupt_disable(index);
        rhs_hal_interrupt_clear_pending(index);
    }

    isr_descr->isr     = isr;
    isr_descr->context = context;
    __DMB();

    if (isr)
    {
        // Post ISR set
        rhs_hal_interrupt_clear_pending(index);
        rhs_hal_interrupt_enable(index, real_priority);
    }
    else
    {
        // Post ISR clear
    }
}
#ifdef STM32F765xx
/* CAN 1 RX0 */
void CAN1_RX0_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1Rx0);
}

void CAN1_SCE_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1SCE);
}

void CAN1_TX_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1Tx);
}

#    if defined(RPLC_XL) || defined(RPLC_L)

/* CAN 2 RX0 */
void CAN2_RX0_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN2Rx0);
}

void CAN2_SCE_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN2SCE);
}

void CAN2_TX_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1Tx);
}

#    endif

/* USART 3 */
void USART3_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdUsart3);
}

/* DMA 1 */
void DMA1_Stream3_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdDMA1Stream3);
}

/* DMA 2 */
void DMA2_Stream1_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdDMA2Stream1);
}

void DMA2_Stream6_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdDMA2Stream6);
}

#elif STM32F407xx

/* CAN 1 RX0 */
void CAN1_RX0_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1Rx0);
}

void CAN1_SCE_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1SCE);
}

void CAN1_TX_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1Tx);
}

/* CAN 2 RX0 */
void CAN2_RX0_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN2Rx0);
}

void CAN2_SCE_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN2SCE);
}

void CAN2_TX_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdCAN1Tx);
}

#elif STM32F103xE

#    include "usbd_core.h"

extern usbd_device udev;

extern void HW_IPCC_Tx_Handler(void);
extern void HW_IPCC_Rx_Handler(void);

void USB_LP_IRQHandler(void)
{
    usbd_poll(&udev);
}

void USB_HP_IRQHandler(void)
{
    usbd_poll(&udev);
}

/* USART 3 */
void USART3_IRQHandler(void)
{
    rhs_hal_interrupt_call(RHSHalInterruptIdUsart3);
}

#endif

void HardFault_Handler(void)
{
    rhs_crash("HardFault");
}

void BusFault_Handler(void)
{
    rhs_crash("BusFault");
}

void UsageFault_Handler(void)
{
    rhs_crash("UsageFault");
}

void DebugMon_Handler(void) {}

void NMI_Handler(void)
{
    if (LL_RCC_IsActiveFlag_HSECSS())
    {
        LL_RCC_ClearFlag_HSECSS();
        /* HSE CSS fired: resetting system */
        NVIC_SystemReset();
    }
}

void MemManage_Handler(void)
{
    rhs_crash("MemManage");
}

// Potential space-saver for updater build
const char* rhs_hal_interrupt_get_name(uint8_t exception_number)
{
    int32_t id = (int32_t) exception_number - 16;

    switch (id)
    {
    case NonMaskableInt_IRQn:
        return "NMI";
    case MemoryManagement_IRQn:
        return "MemMgmt";
    case BusFault_IRQn:
        return "BusFault";
    case UsageFault_IRQn:
        return "UsageFault";
    case SVCall_IRQn:
        return "SVC";
    case DebugMonitor_IRQn:
        return "DebugMon";
    case PendSV_IRQn:
        return "PendSV";
    case SysTick_IRQn:
        return "SysTick";
    case WWDG_IRQn:
        return "WWDG";
    case PVD_IRQn:
        return "PVD";
#if !defined(STM32F103xE)
    case TAMP_STAMP_IRQn:
        return "TAMP_STAMP";
    case RTC_WKUP_IRQn:
        return "RTC_WKUP";
#endif
    case FLASH_IRQn:
        return "FLASH";
    case RCC_IRQn:
        return "RCC";
    default:
        return NULL;
    }
}

uint32_t rhs_hal_interrupt_get_time_in_isr_total(void)
{
    return rhs_hal_interrupt.counter_time_in_isr_total;
}
