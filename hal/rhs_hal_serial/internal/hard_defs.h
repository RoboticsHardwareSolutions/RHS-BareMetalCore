
#if defined(RPLC_XL) || defined(RPLC_L)
#    include "stm32f7xx_ll_usart.h"
#    define RHS_INTERFACE_RS232 USART3
#    define RS232_STR "USART3"
#    define RHS_INTERRUPT_RS232 RHSHalInterruptIdUsart3
#    define RHS_DMA_TX_RS232 RHSHalInterruptIdDMA1Stream3
// #    define RHS_DMA_RX_RS232 RHSHalInterruptIdUsart3 // TODO
#    define RHS_INTERFACE_RS485 USART6
#    define RS485_STR "USART6"
#    define RHS_INTERRUPT_RS485 RHSHalInterruptIdUsart6
#    define RHS_DMA_TX_RS485 RHSHalInterruptIdDMA2Stream6
#    define RHS_DMA_RX_RS485 RHSHalInterruptIdDMA2Stream1
#    if !defined(RPLC_XL)
#        define RHS_INTERFACE_RS422 USART5
#        define RS422_STR "USART5"
#        define RHS_INTERRUPT_RS422 RHSHalInterruptIdUart5
// #        define RHS_DMA_TX_RS422 RHSHalInterruptIdDMA1Stream3 // TODO
// #        define RHS_DMA_RX_RS422 RHSHalInterruptIdUsart3 // TODO
#    endif
#elif defined(RPLC_M)
#    include "stm32f1xx_ll_usart.h"
#    define RHS_INTERFACE_RS232 USART3
#    define RS232_STR "USART3"
#    define RHS_INTERRUPT_RS232 RHSHalInterruptIdUsart3
#    define RHS_DMA_TX_RS232 RHSHalInterruptIdDMA1Channel2
#    define RHS_DMA_RX_RS232 RHSHalInterruptIdDMA1Channel3
#    define RHS_INTERFACE_RS485 UART5
#    define RS485_STR "UART5"
#    define RHS_INTERRUPT_RS485 RHSHalInterruptIdUart5
#    define RHS_DMA_TX_RS485 RHSHalInterruptIdDMA1Channel2  // TODO
#    define RHS_DMA_RX_RS485 RHSHalInterruptIdDMA1Channel3  // TODO
#    define RHS_INTERFACE_RS422 UART4
#    define RS422_STR "UART4"
#    define RHS_INTERRUPT_RS422 RHSHalInterruptIdUart4
#    define RHS_DMA_TX_RS422 RHSHalInterruptIdDMA1Channel2  // TODO
#    define RHS_DMA_RX_RS422 RHSHalInterruptIdDMA1Channel3  // TODO
#else
#    error "Not implemented Serial for this platform"
#endif
