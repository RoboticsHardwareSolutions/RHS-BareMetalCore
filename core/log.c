#include <stdarg.h>
#include "string.h"
#include "log.h"
#include "check.h"
#include "SEGGER_RTT.h"
#include "kernel.h"
#include "mutex.h"
#include "common.h"

#define _RHS_LOG_CLR(clr) "\033[0;" clr "m"
#define _RHS_LOG_CLR_RESET "\033[0m"

#define _RHS_LOG_CLR_BLACK "30"
#define _RHS_LOG_CLR_RED "31"
#define _RHS_LOG_CLR_GREEN "32"
#define _RHS_LOG_CLR_BROWN "33"
#define _RHS_LOG_CLR_BLUE "34"
#define _RHS_LOG_CLR_PURPLE "35"

#define _RHS_LOG_CLR_E _RHS_LOG_CLR(_RHS_LOG_CLR_RED)
#define _RHS_LOG_CLR_W _RHS_LOG_CLR(_RHS_LOG_CLR_BROWN)
#define _RHS_LOG_CLR_I _RHS_LOG_CLR(_RHS_LOG_CLR_GREEN)
#define _RHS_LOG_CLR_D _RHS_LOG_CLR(_RHS_LOG_CLR_BLUE)
#define _RHS_LOG_CLR_T _RHS_LOG_CLR(_RHS_LOG_CLR_PURPLE)

#define RHS_LOG_LEVEL_DEFAULT RHSLogLevelDebug
#define MAX_LOG_COUNT 16
#define MAX_LOG_LENGTH 120
#define MAX_TAG_COUNT 32
#define MAX_TAG_LENGTH 16

__attribute__((weak)) extern uint32_t _ram_log_start;
__attribute__((weak)) extern uint32_t _ram_log_end;
__attribute__((weak)) extern uint32_t _ram_log_size;

#define LOG_MAGIC_KEY 0x28735F7A
typedef struct
{
    uint32_t MAGIC_KEY;
    uint16_t count;
    char     space[];
} save_log_t;

static RHSLogLevel log_level = RHS_LOG_LEVEL_DEFAULT;
static char*       exclude_tag[MAX_TAG_COUNT];
static RHSMutex*   mutex;

static save_log_t* save_log = NULL;
uint32_t max_log_count;

int _write(int file, char* ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        SEGGER_RTT_PutChar(0, ptr[i]);
    }
    return len;
}

void rhs_log_init(void)
{
    if ((&_ram_log_start != NULL) && (&_ram_log_size != NULL))
    {
        SEGGER_RTT_printf(0, "exist %d\r\n", (uint32_t) &_ram_log_size);
        static save_log_t __attribute__((section("MB_MEM_LOG"))) log;
        max_log_count = (uint32_t)&_ram_log_size / MAX_LOG_LENGTH;
        save_log = &log;
    }
    else
    {
        SEGGER_RTT_printf(0, "not exist\r\n");
        static save_log_t log;
        max_log_count = MAX_LOG_COUNT;
        save_log = &log;
        save_log = malloc(sizeof(save_log_t) + max_log_count * MAX_LOG_LENGTH);
    }

    mutex = rhs_mutex_alloc(RHSMutexTypeRecursive);
}

void rhs_log_print_format(RHSLogLevel level, const char* tag, const char* format, ...)
{
    if (level > log_level)
    {
        return;
    }
    for (int i = 0; i < MAX_TAG_COUNT; i++)
    {
        if (exclude_tag[i] != NULL && strcmp(exclude_tag[i], tag) == 0)
        {
            return;
        }
    }

    if (!RHS_IS_ISR())
    {
        if (rhs_mutex_acquire(mutex, rhs_kernel_is_running() ? RHSWaitForever : 0) != RHSStatusOk)
        {
            return;
        }
    }
    else
    {
        if (rhs_mutex_get_owner(mutex))
            return;
    }

    const char* color      = _RHS_LOG_CLR_RESET;
    const char* log_letter = " ";
    switch (level)
    {
    case RHSLogLevelError:
        color      = _RHS_LOG_CLR_E;
        log_letter = "E";
        break;
    case RHSLogLevelWarn:
        color      = _RHS_LOG_CLR_W;
        log_letter = "W";
        break;
    case RHSLogLevelInfo:
        color      = _RHS_LOG_CLR_I;
        log_letter = "I";
        break;
    case RHSLogLevelDebug:
        color      = _RHS_LOG_CLR_D;
        log_letter = "D";
        break;
    case RHSLogLevelTrace:
        color      = _RHS_LOG_CLR_T;
        log_letter = "T";
        break;
    default:
        break;
    }

    SEGGER_RTT_printf(0, "%s%d:\t[%s][%s]:\t", color, rhs_get_tick(), log_letter, tag);
    va_list ParamList;
    va_start(ParamList, format);
    SEGGER_RTT_vprintf(0, format, &ParamList);
    va_end(ParamList);
    SEGGER_RTT_printf(0, "%s\n", _RHS_LOG_CLR_RESET);
    rhs_mutex_release(mutex);
}

void rhs_log_exclude_tag(char* tag)
{
    rhs_assert(tag);
    for (uint8_t i = 0; i < MAX_TAG_COUNT; i++)
    {
        if (exclude_tag[i] == NULL)
        {
            exclude_tag[i] = malloc(MAX_TAG_LENGTH);
            strncpy(exclude_tag[i], tag, MAX_TAG_LENGTH);
            return;
        }
    }
}

void rhs_log_unexclude_tag(char* tag)
{
    rhs_assert(tag);
    for (uint8_t i = 0; i < MAX_TAG_COUNT; i++)
    {
        if ((exclude_tag[i] != NULL) && (strcmp(exclude_tag[i], tag) == 0))
        {
            free(exclude_tag[i]);
            exclude_tag[i] = NULL;
            return;
        }
    }
}

void rhs_log_set_level(RHSLogLevel level)
{
    rhs_assert(level <= RHSLogLevelTrace);

    if (level == RHSLogLevelDefault)
    {
        level = RHS_LOG_LEVEL_DEFAULT;
    }
    log_level = level;
}

RHSLogLevel rhs_log_get_level(void)
{
    return log_level;
}

void rhs_log_save(char* str, ...)
{
    if (save_log->MAGIC_KEY != LOG_MAGIC_KEY || save_log->count >= max_log_count)
    {
        rhs_erase_saved_log();
        save_log->MAGIC_KEY = LOG_MAGIC_KEY;
        save_log->count = 0;
    }

    char* p = save_log->space + save_log->count * MAX_LOG_LENGTH;

    va_list ParamList;
    va_start(ParamList, str);
    vsnprintf(p, MAX_LOG_LENGTH, str, ParamList);
    va_end(ParamList);

    save_log->count++;
}

char* rhs_read_saved_log(uint16_t index)
{
    if (save_log->MAGIC_KEY == LOG_MAGIC_KEY && index < save_log->count)
    {
        return &save_log->space[index * MAX_LOG_LENGTH];
    }
    else
    {
        return NULL;
    }
}

void rhs_erase_saved_log(void)
{
    save_log->MAGIC_KEY = 0;
}

uint16_t rhs_count_saved_log(void)
{
    return save_log->MAGIC_KEY == LOG_MAGIC_KEY ? save_log->count : 0;
}
