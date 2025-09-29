#pragma once
#include "stdint.h"
#include "core/thread.h"

typedef enum {
    RHSInternalApplicationFlagDefault = 0,
    RHSInternalApplicationFlagInsomniaSafe = (1 << 0),
} RHSInternalApplicationFlag;

typedef struct {
    const RHSThreadCallback app;
    const char* name;
    const uint16_t stack_size;
    const RHSInternalApplicationFlag flags;
} RHSInternalApplication;

extern const short RHS_SERVICES_COUNT;
extern const RHSInternalApplication RHS_SERVICES[];

typedef void (*RHSInternalOnStartHook)(void);

extern const short RHS_START_UP_COUNT;
extern const RHSInternalOnStartHook RHS_START_UP[];

typedef void (*RHSInternalOnTestHook)(void);

extern const short RHS_TESTS_COUNT;
extern const RHSInternalOnTestHook RHS_TESTS[];
