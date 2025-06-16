#include "cli.h"
#include "cli_app.h"
#include "rhs.h"
#include "rhs_hal.h"
#include <stddef.h>
#include "SEGGER_RTT.h"

#define TAG "cli"

#define MAX_LINE_LENGTH 64

Cli* cli_alloc(void)
{
    Cli* app   = malloc(sizeof(Cli));
    app->line  = calloc(1, MAX_LINE_LENGTH);
    app->mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
    return app;
}

void cli_add_command(Cli* app, const char* name, CliCallback callback, void* context)
{
    CliCommand* command;
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    for (int i = 0; i < sizeof(app->commands) / sizeof(app->commands[0]); i++)
    {
        if (i == sizeof(app->commands) / sizeof(app->commands[0]))
        {
            rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
            return;
        }
        command = &app->commands[i];
        if (command->name == NULL)
        {
            break;
        }
        if (strcmp(command->name, name) == 0)
        {
            RHS_LOG_E(TAG, "Cammand already exists");
            rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
            return;
        }
    }
    command->name     = name;
    command->callback = callback;
    command->context  = context;
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}

static char cli_getc(void)
{
    char c = 0;

    // Only for RTT (it will be not here in the future)
    static bool enter = false;
    SEGGER_RTT_Read(0, &c, 1);
    if (c != 0)
    {
        enter = true;
    }
    else if (enter)
    {
        enter = false;
        c     = CliSymbolAsciiCR;
    }

    rhs_delay_tick(10);
    return c;
}

void cli_reset(Cli* app)
{
    app->cursor_position = 0;
    memset(app->line, 0, MAX_LINE_LENGTH);
}

void cli_handle_enter(Cli* app)
{
    for (int i = 0; i < sizeof(app->commands) / sizeof(app->commands[0]); i++)
    {
        if (app->commands[i].name == NULL)
        {
            break;
        }
        char*  separator      = strchr(app->line, ' ');
        size_t command_length = separator ? (size_t) (separator - app->line) : strlen(app->line);
        if (strncmp(app->line, app->commands[i].name, command_length) == 0 &&
            strlen(app->commands[i].name) == command_length)
        {
            app->commands[i].callback(separator ? separator + 1 : NULL, app->commands[i].context);
            cli_reset(app);
            return;
        }
    }
    if (*app->line != 0)
    {
        RHS_LOG_I(TAG, "Unknown command");
    }
    cli_reset(app);
}

void cli_process_input(Cli* app)
{
    char in_chr = cli_getc();

    size_t rx_len;

    if (in_chr == CliSymbolAsciiTab)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiTab");
    }
    else if (in_chr == CliSymbolAsciiSOH)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiSOH");
    }
    else if (in_chr == CliSymbolAsciiETX)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiETX");
    }
    else if (in_chr == CliSymbolAsciiEOT)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiEOT");
    }
    else if (in_chr == CliSymbolAsciiEsc)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiEsc");
    }
    else if (in_chr == CliSymbolAsciiBackspace || in_chr == CliSymbolAsciiDel)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiDel");
    }
    else if (in_chr == CliSymbolAsciiCR)
    {
        cli_handle_enter(app);
    }
    else if ((in_chr >= 0x20 && in_chr < 0x7F))
    {
        app->line[app->cursor_position] = in_chr;
        app->cursor_position++;
    }
    else
    {
    }
}

void cli_command_uptime(char* args, void* context)
{
    uint32_t uptime = rhs_get_tick() / rhs_kernel_get_tick_frequency();
    SEGGER_RTT_printf(0, "Uptime: %luh%lum%lus\r\n", uptime / 60 / 60, uptime / 60 % 60, uptime % 60);
}

void cli_command_free(char* args, void* context)
{
    RHS_LOG_I(TAG, "total_heap: %d", memmgr_get_total_heap());
    RHS_LOG_I(TAG, "minimum_free_heap: %d", memmgr_get_minimum_free_heap());
    RHS_LOG_I(TAG, "free_heap: %d", memmgr_get_free_heap());
    RHS_LOG_I(TAG, "isr_ticks: %d", rhs_hal_interrupt_get_time_in_isr_total());
}

void cli_commands(char* args, void* context)
{
    Cli* app = (Cli*) context;
    SEGGER_RTT_printf(0, "Available commands:\r\n");
    for (int i = 0; i < sizeof(app->commands) / sizeof(app->commands[0]); i++)
    {
        if (app->commands[i].name == NULL)
        {
            break;
        }
        SEGGER_RTT_printf(0, "%s\r\n", app->commands[i].name);
    }
}

void cli_command_log(char* args, void* context)
{
    if (args == NULL)
    {
        RHS_LOG_I(TAG, "log level is %d", rhs_log_get_level());
    }
    else if (strlen(args) == 1 && args[0] >= '0' && args[0] <= '6')
    {
        rhs_log_set_level(args[0] - '0');
        RHS_LOG_I(TAG, "log level is %d", rhs_log_get_level());
    }
    else
    {
        char* separator = strchr(args, ' ');
        if (separator == NULL || *(separator + 1) == 0)
        {
            if (strstr(args, "-l") == args)
            {
                uint16_t s = rhs_count_saved_log();
                for (int i = 0; i < s; i++)
                {
                    SEGGER_RTT_printf(0, "%s\r\n", rhs_read_saved_log(i));
                }
                return;
            }
            RHS_LOG_E(TAG, "Invalid argument");
            return;
        }
        else if (strstr(args, "-e") == args)
        {
            rhs_log_exclude_tag(separator + 1);
            RHS_LOG_I(TAG, "TAG %s was excluded", separator + 1);
            return;
        }
        else if (strstr(args, "-ue") == args)
        {
            rhs_log_unexclude_tag(separator + 1);
            RHS_LOG_I(TAG, "TAG %s was unexcluded", separator + 1);
            return;
        }
        else if (strstr(args, "-s") == args)
        {
            rhs_log_save(separator + 1);
            return;
        }

        RHS_LOG_E(TAG, "Invalid argument");
    }
}

void cli_command_reset(char* args, void* context)
{
    rhs_hal_power_reset();
}

void cli_command_uid(char* args, void* context)
{
    const uint8_t* uid = rhs_hal_version_uid();
    RHS_LOG_I(TAG,
              "UID %02X%02X-%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
              uid[0],
              uid[1],  // 16 - bit
              uid[2],
              uid[3],  // 16 - bit
              uid[4],
              uid[5],
              uid[6],
              uid[7],  // 32 - bits
              uid[8],
              uid[9],
              uid[10],
              uid[11]  // 32 - bits
    );
}

void cli_command_top(char* args, void* context)
{
    RHSThreadList* thread_list = rhs_thread_list_create();
    uint16_t       count;

    rhs_thread_enumerate(thread_list);
    count = rhs_thread_list_size(thread_list);

    RHS_LOG_I(TAG, "Total run count: %u", count);
    RHS_LOG_I(TAG, "%-32s %-10s %-5s %-6s %-10s", "Task Name", "State", "Prio", "RunTime", "StackMinFree");

    for (size_t i = 0; i < count; i++)
    {
        RHSThreadListItem* item = rhs_thread_list_at(thread_list, i);
        RHS_LOG_I(TAG,
                  "%-32s %-10s %-3d %-4s %-5d",
                  item->name,
                  item->state,
                  item->priority,
                  item->cpu < 1 ? "<1%"
                                : (
                                      {
                                          char buffer[12];
                                          itoa(item->cpu, buffer, 10);
                                          strcat(buffer, "%");
                                          buffer;
                                      }),
                  item->stack_min_free);
    }

    rhs_thread_list_destroy(thread_list);
}

int32_t cli_service(void* context)
{
    Cli* app = cli_alloc();
    rhs_record_create(RECORD_CLI, app);

    cli_add_command(app, "uptime", cli_command_uptime, NULL);
    cli_add_command(app, "free", cli_command_free, NULL);
    cli_add_command(app, "?", cli_commands, app);
    cli_add_command(app, "log", cli_command_log, NULL);
    cli_add_command(app, "reset", cli_command_reset, NULL);
    cli_add_command(app, "uid", cli_command_uid, NULL);
    cli_add_command(app, "top", cli_command_top, NULL);

    for (;;)
    {
        cli_process_input(app);
    }
}
