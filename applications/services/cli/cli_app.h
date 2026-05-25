#pragma once
#include "rhs.h"

typedef struct
{
    // const char* name;
    CliCallback callback;
    void*       context;
    uint32_t    flags;
} CliCommand;
