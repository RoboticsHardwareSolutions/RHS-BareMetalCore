#pragma once

#include <stdio.h>
#include <stdint.h>

typedef enum
{
    RHSLogLevelDefault = 0,
    RHSLogLevelNone    = 1,
    RHSLogLevelError   = 2,
    RHSLogLevelWarn    = 3,
    RHSLogLevelInfo    = 4,
    RHSLogLevelDebug   = 5,
    RHSLogLevelTrace   = 6,
} RHSLogLevel;

void rhs_log_init(void);

void rhs_log_print_format(RHSLogLevel level, const char* tag, const char* format, ...)
    _ATTRIBUTE((__format__(__printf__, 3, 4)));

void rhs_log_set_level(RHSLogLevel level);

RHSLogLevel rhs_log_get_level(void);

void rhs_log_exclude_tag(char* tag);

void rhs_log_unexclude_tag(char* tag);

#define RHS_LOG_E(tag, format, ...) rhs_log_print_format(RHSLogLevelError, tag, format, ##__VA_ARGS__)
#define RHS_LOG_W(tag, format, ...) rhs_log_print_format(RHSLogLevelWarn, tag, format, ##__VA_ARGS__)
#define RHS_LOG_I(tag, format, ...) rhs_log_print_format(RHSLogLevelInfo, tag, format, ##__VA_ARGS__)
#define RHS_LOG_D(tag, format, ...) rhs_log_print_format(RHSLogLevelDebug, tag, format, ##__VA_ARGS__)
#define RHS_LOG_T(tag, format, ...) rhs_log_print_format(RHSLogLevelTrace, tag, format, ##__VA_ARGS__)

char*          rhs_read_saved_log(unsigned short index);
void           rhs_erase_saved_log(void);
unsigned short rhs_count_saved_log(void);
