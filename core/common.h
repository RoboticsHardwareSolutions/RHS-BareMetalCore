#pragma once
#include <cmsis_compiler.h>

#ifndef RHS_IS_IRQ_MODE
#define RHS_IS_IRQ_MODE() (__get_IPSR() != 0U)
#endif

#ifndef RHS_IS_IRQ_MASKED
#define RHS_IS_IRQ_MASKED() (__get_PRIMASK() != 0U)
#endif

#ifndef RHS_IS_ISR
#define RHS_IS_ISR() (RHS_IS_IRQ_MODE() || RHS_IS_IRQ_MASKED())
#endif