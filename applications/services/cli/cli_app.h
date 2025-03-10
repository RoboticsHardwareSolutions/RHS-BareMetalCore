#pragma once
#include "rhs.h"

typedef struct
{
    const char* name;
    CliCallback callback;
    void*       context;
    uint32_t    flags;
} CliCommand;

struct Cli
{
    RHSMutex*  mutex;
    char*      line;
    uint8_t    cursor_position;
    CliCommand commands[32];
};
