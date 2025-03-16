#include "cli.h"
#include "cli_app.h"
#include "rhs.h"
#include <stddef.h>
#include "SEGGER_RTT.h"

#define TAG "cli"

#define MAX_LINE_LENGTH 64

static Cli* s_cli = NULL;

Cli* cli_alloc(void)
{
    Cli* cli   = malloc(sizeof(Cli));
    s_cli      = cli;
    cli->line  = calloc(1, MAX_LINE_LENGTH);
    cli->mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
    return cli;
}

void cli_add_command(const char* name, CliCallback callback, void* context)
{
    rhs_assert(s_cli);
    CliCommand* command;
    rhs_assert(rhs_mutex_acquire(s_cli->mutex, RHSWaitForever) == RHSStatusOk);
    for (int i = 0; i < sizeof(s_cli->commands) / sizeof(s_cli->commands[0]); i++)
    {
        if (i == sizeof(s_cli->commands) / sizeof(s_cli->commands[0]))
        {
            rhs_assert(rhs_mutex_release(s_cli->mutex) == RHSStatusOk);
            return;
        }
        command = &s_cli->commands[i];
        if (command->name == NULL)
        {
            break;
        }
        if (strcmp(command->name, name) == 0)
        {
            RHS_LOG_E(TAG, "Cammand already exists\r\n");
            rhs_assert(rhs_mutex_release(s_cli->mutex) == RHSStatusOk);
            return;
        }
    }
    command->name     = name;
    command->callback = callback;
    command->context  = context;
    rhs_assert(rhs_mutex_release(s_cli->mutex) == RHSStatusOk);
}

char cli_getc()
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

void cli_reset(Cli* cli)
{
    cli->cursor_position = 0;
    memset(cli->line, 0, MAX_LINE_LENGTH);
}

void cli_handle_enter(Cli* cli)
{
    for (int i = 0; i < sizeof(cli->commands) / sizeof(cli->commands[0]); i++)
    {
        if (cli->commands[i].name == NULL)
        {
            break;
        }
        char* separator = strchr(cli->line, ' ');
        char* command   = strstr(cli->line, cli->commands[i].name);
        if (command != NULL && command == cli->line)
        {
            cli->commands[i].callback(separator ? separator + 1 : NULL, cli->commands[i].context);
            cli_reset(cli);
            return;
        }
    }
    if (*cli->line != 0)
    {
        SEGGER_RTT_printf(0, "Unknown command\r\n");
    }
    cli_reset(cli);
}

void cli_process_input(Cli* cli)
{
    char in_chr = cli_getc();

    size_t rx_len;

    if (in_chr == CliSymbolAsciiTab)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiTab\r\n");
    }
    else if (in_chr == CliSymbolAsciiSOH)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiSOH\r\n");
    }
    else if (in_chr == CliSymbolAsciiETX)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiETX\r\n");
    }
    else if (in_chr == CliSymbolAsciiEOT)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiEOT\r\n");
    }
    else if (in_chr == CliSymbolAsciiEsc)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiEsc\r\n");
    }
    else if (in_chr == CliSymbolAsciiBackspace || in_chr == CliSymbolAsciiDel)
    {
        RHS_LOG_D(TAG, "CliSymbolAsciiDel\r\n");
    }
    else if (in_chr == CliSymbolAsciiCR)
    {
        cli_handle_enter(cli);
    }
    else if ((in_chr >= 0x20 && in_chr < 0x7F))
    {
        cli->line[cli->cursor_position] = in_chr;
        cli->cursor_position++;
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
    SEGGER_RTT_printf(0, "Free heap size: %d\r\n", memmgr_get_free_heap());
    SEGGER_RTT_printf(0, "Minimum heap size: %d\r\n", memmgr_get_minimum_free_heap());
}

void cli_commands(char* args, void* context)
{
    SEGGER_RTT_printf(0, "Available commands:\r\n");
    for (int i = 0; i < sizeof(s_cli->commands) / sizeof(s_cli->commands[0]); i++)
    {
        if (s_cli->commands[i].name == NULL)
        {
            break;
        }
        SEGGER_RTT_printf(0, "%s\r\n", s_cli->commands[i].name);
    }
}

void cli_command_log(char* args, void* context)
{
    if (args == NULL)
    {
        RHS_LOG_I(TAG, "log level is %d\r\n", rhs_log_get_level());
    }
    else if (strlen(args) == 1 && args[0] >= '0' && args[0] <= '6')
    {
        rhs_log_set_level(args[0] - '0');
        RHS_LOG_I(TAG, "log level is %d\r\n", rhs_log_get_level());
    }
    else
    {
        char* separator = strchr(args, ' ');
        if (separator == NULL || *(separator + 1) == 0)
        {
            RHS_LOG_E(TAG, "Invalid argument\r\n");
            return;
        }
        else if (strstr(args, "-e") == args)
        {
            rhs_log_exclude_tag(separator + 1);
            RHS_LOG_I(TAG, "TAG %s was excluded\r\n", separator + 1);
            return;
        }
        else if (strstr(args, "-ue") == args)
        {
            rhs_log_unexclude_tag(separator + 1);
            RHS_LOG_I(TAG, "TAG %s was unexcluded\r\n", separator + 1);
            return;
        }
        else if (strstr(args, "-s") == args)
        {
            rhs_log_save(separator + 1);
            return;
        }
        else if (strstr(args, "-l") == args)
        {
            uint16_t s = rhs_count_saved_log();
            for(int i = 0; i < s; i++)
            {
                SEGGER_RTT_printf(0, "%s\r\n", rhs_read_saved_log(i));
            }
            return;
        }
        
        RHS_LOG_E(TAG, "Invalid argument\r\n");
    }
}

int32_t cli_service(void* context)
{
    Cli* cli = cli_alloc();

    cli_add_command("uptime", cli_command_uptime, NULL);
    cli_add_command("free", cli_command_free, NULL);
    cli_add_command("?", cli_commands, NULL);
    cli_add_command("log", cli_command_log, NULL);

    for (;;)
    {
        cli_process_input(cli);
    }
}
