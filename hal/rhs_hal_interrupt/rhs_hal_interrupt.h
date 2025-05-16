#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Timer ISR */
typedef void (*RHSHalInterruptISR)(void* context);

typedef enum
{
#if defined(RPLC_XL) || defined(RPLC_L)
    /* CAN */
    RHSHalInterruptIdCAN1Rx0,
    RHSHalInterruptIdCAN1SCE,
    RHSHalInterruptIdCAN1Tx,

    /* USART */
    RHSHalInterruptIdUsart3,
    RHSHalInterruptIdUsart6,

    /* UART */
    RHSHalInterruptIdUart5,

    /* DMA */
    RHSHalInterruptIdDMA1Stream3,
    RHSHalInterruptIdDMA2Stream1,
    RHSHalInterruptIdDMA2Stream6,
#elif defined(RPLC_M)
    /* CAN */
    RHSHalInterruptIdCAN1Rx0,
    RHSHalInterruptIdCAN1SCE,
    RHSHalInterruptIdCAN1Tx,

    /* USART */
    RHSHalInterruptIdUsart3,
    RHSHalInterruptIdUart4,
    RHSHalInterruptIdUart5,

    /* DMA */
    RHSHalInterruptIdDMA1Channel2,
    RHSHalInterruptIdDMA1Channel3,
#else

#    if defined(STM32F765xx)
    /* CAN */
    RHSHalInterruptIdCAN1Rx0,
    RHSHalInterruptIdCAN1SCE,
    RHSHalInterruptIdCAN1Tx,
    RHSHalInterruptIdCAN2Rx0,
    RHSHalInterruptIdCAN2SCE,
    RHSHalInterruptIdCAN2Tx,

    /* USART */
    RHSHalInterruptIdUsart3,
    RHSHalInterruptIdUsart6,

    /* UART */
    RHSHalInterruptIdUart5,
    /* DMA */
    RHSHalInterruptIdDMA1Stream3,
    RHSHalInterruptIdDMA2Stream1,
    RHSHalInterruptIdDMA2Stream6,
#    elif defined(STM32F407xx) || defined(STM32F405xx)
    /* CAN */
    RHSHalInterruptIdCAN1Rx0,
    RHSHalInterruptIdCAN1SCE,
    RHSHalInterruptIdCAN1Tx,
    RHSHalInterruptIdCAN2Rx0,
    RHSHalInterruptIdCAN2SCE,
    RHSHalInterruptIdCAN2Tx,
#    elif defined(STM32F103xE)
    /* USART */
    RHSHalInterruptIdUsart3,
    RHSHalInterruptIdUsart4,
    RHSHalInterruptIdUsart5,
    /* DMA */
    RHSHalInterruptIdDMA1Channel2,
    RHSHalInterruptIdDMA1Channel3,
#    endif
#endif
    // Service value
    RHSHalInterruptIdMax,
} RHSHalInterruptId;

typedef enum
{
    RHSHalInterruptPriorityLowest  = -3, /**< Lowest priority level, you can use ISR-safe OS primitives */
    RHSHalInterruptPriorityLower   = -2, /**< Lower priority level, you can use ISR-safe OS primitives */
    RHSHalInterruptPriorityLow     = -1, /**< Low priority level, you can use ISR-safe OS primitives */
    RHSHalInterruptPriorityNormal  = 0,  /**< Normal(default) priority level, you can use ISR-safe OS primitives */
    RHSHalInterruptPriorityHigh    = 1,  /**< High priority level, you can use ISR-safe OS primitives */
    RHSHalInterruptPriorityHigher  = 2,  /**< Higher priority level, you can use ISR-safe OS primitives */
    RHSHalInterruptPriorityHighest = 3,  /**< Highest priority level, you can use ISR-safe OS primitives */

    /* Special group, read docs first(ALL OF THEM: especially FreeRTOS configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) */
    RHSHalInterruptPriorityKamiSama =
        6, /**< Forget about thread safety, you are god now. No one can prevent you from messing with OS critical
              section. You are not allowed to use any OS primitives, but who can stop you? Use this priority only for
              direct hardware interaction with LL HAL. */
} RHSHalInterruptPriority;

/** Initialize interrupt subsystem */
void rhs_hal_interrupt_init(void);

/** Set ISR and enable interrupt with default priority
 *
 * @warning    Interrupt flags are not cleared automatically. You may want to
 *             ensure that your peripheral status flags are cleared.
 *
 * @param      index    - interrupt ID
 * @param      isr      - your interrupt service routine or use NULL to clear
 * @param      context  - isr context
 */
void rhs_hal_interrupt_set_isr(RHSHalInterruptId index, RHSHalInterruptISR isr, void* context);

/** Set ISR and enable interrupt with custom priority
 *
 * @warning    Interrupt flags are not cleared automatically. You may want to
 *             ensure that your peripheral status flags are cleared.
 *
 * @param      index     - interrupt ID
 * @param      priority  - One of RHSHalInterruptPriority
 * @param      isr       - your interrupt service routine or use NULL to clear
 * @param      context   - isr context
 */
void rhs_hal_interrupt_set_isr_ex(RHSHalInterruptId       index,
                                  RHSHalInterruptPriority priority,
                                  RHSHalInterruptISR      isr,
                                  void*                   context);

/** Get interrupt name by exception number.
 * Exception number can be obtained from IPSR register.
 *
 * @param exception_number
 * @return const char* or NULL if interrupt name is not found
 */
const char* rhs_hal_interrupt_get_name(uint8_t exception_number);

/** Get total time(in CPU clocks) spent in ISR
 *
 * @return     total time in CPU clocks
 */
uint32_t rhs_hal_interrupt_get_time_in_isr_total(void);

#ifdef __cplusplus
}
#endif
