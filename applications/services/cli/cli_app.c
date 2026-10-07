#include "cli.h"
#include "cli_app.h"
#include "rhs.h"
#include "rhs_hal.h"
#include <stddef.h>
#include "rhs_version.h"
#include <m-dict.h>
#include "core/m_cstr_dup.h"

#define TAG "cli"

#define MAX_LINE_LENGTH 64

DICT_DEF2(CliCommandDict, const char*, M_CSTR_DUP_OPLIST, CliCommand, M_POD_OPLIST);

struct Cli
{
    RHSMutex*        mutex;
    uint8_t          cursor_position;
    char             line[MAX_LINE_LENGTH];
    CliCommandDict_t commands;
};

Cli* cli_alloc(void)
{
    Cli* app             = malloc(sizeof(Cli));
    app->mutex           = rhs_mutex_alloc(RHSMutexTypeNormal);
    app->cursor_position = 0;
    memset(app->line, 0, sizeof(app->line));
    CliCommandDict_init(app->commands);
    return app;
}

void cli_add_command(Cli* app, const char* name, CliCallback callback, void* context)
{
    rhs_assert(strcmp(name, " ") != 0);

    CliCommand command = {
        .context  = context,
        .callback = callback,
    };

    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    CliCommandDict_set_at(app->commands, name, command);
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}

void cli_remove_command(Cli* app, const char* name)
{
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    CliCommandDict_erase(app->commands, name);
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}

static char cli_getc(void)
{
    static bool enter = false;

    char c = getchar();

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
    char* end;
    if (end = strchr(app->line, ' '))
    {
        *end = 0;
    }

    CliCommand* command = CliCommandDict_get(app->commands, app->line);
    if (command)
    {
        command->callback(end ? end + 1 : NULL, command->context);
    }
    else
    {
        RHS_LOG_W(TAG, "Unknown command. Type '?' for list of commands.");
    }
    cli_reset(app);
}

void cli_process_input(Cli* app)
{
    char in_chr = cli_getc();

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
    printf("Uptime: %luh%lum%lus\r\n", uptime / 60 / 60, uptime / 60 % 60, uptime % 60);
}

void cli_command_free(char* args, void* context)
{
    printf("total_heap: %d\r\n", memmgr_get_total_heap());
    printf("minimum_free_heap: %d\r\n", memmgr_get_minimum_free_heap());
    printf("free_heap: %d\r\n", memmgr_get_free_heap());
    printf("isr_ticks: %ld\r\n", rhs_hal_interrupt_get_time_in_isr_total());
}

void cli_commands(char* args, void* context)
{
    Cli* app = (Cli*) context;
    printf("Available commands:\r\n");

    const size_t columns = 3;

    size_t commands_count = CliCommandDict_size(app->commands);

    CliCommandDict_it_t iterator;
    CliCommandDict_it(iterator, app->commands);
    for (size_t i = 0; i < commands_count; i++)
    {
        const CliCommandDict_itref_t* item = CliCommandDict_cref(iterator);
        printf("%-30s", item->key);
        CliCommandDict_next(iterator);

        if (i % columns == columns - 1)
            printf("\r\n");
    }
    if (commands_count % columns != 0)
        printf("\r\n");
}

void cli_command_log(char* args, void* context)
{
    if (args == NULL)
    {
        printf("log level is %d\r\n", rhs_log_get_level());
    }
    else if (strlen(args) == 1 && args[0] >= '0' && args[0] <= '6')
    {
        rhs_log_set_level(args[0] - '0');
        printf("log level is %d\r\n", rhs_log_get_level());
    }
    else
    {
        char* separator = strchr(args, ' ');
        /* Start of non paramemters section */
        if (separator == NULL || *(separator + 1) == 0)
        {
            printf("Invalid argument\r\n");
            return;
        }
        /* End of non paramemters section */
        else if (strstr(args, "-e") == args)
        {
            rhs_log_exclude_tag(separator + 1);
            printf("%s was excluded\r\n", separator + 1);
            return;
        }
        else if (strstr(args, "-ue") == args)
        {
            rhs_log_unexclude_tag(separator + 1);
            printf("%s was unexcluded\r\n", separator + 1);
            return;
        }

        printf("Invalid argument\r\n");
    }
}

void cli_command_reset(char* args, void* context)
{
    rhs_hal_power_reset();
}

void cli_command_uid(char* args, void* context)
{
    const uint8_t* uid = rhs_hal_version_uid();
    printf("UID %02X%02X-%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X\n",
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
    static RHSThreadList* thread_list = NULL;
    if (!thread_list) // FIXME: It must be separate thread
        thread_list = rhs_thread_list_create();
    uint16_t count;

    rhs_thread_enumerate(thread_list);
    count = rhs_thread_list_size(thread_list);

    printf("Total run count: %u\r\n", count);
    printf("%-32s %-10s %-5s %-6s %-10s\r\n", "Task Name", "State", "Prio", "RunTime", "StackMinFree");

    for (size_t i = 0; i < count; i++)
    {
        RHSThreadListItem* item = rhs_thread_list_at(thread_list, i);
        printf("%-32s %-10s %-3d %-4s %-5ld\r\n",
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
               (long) item->stack_min_free);
    }

    // rhs_thread_list_destroy(thread_list);
}

void cli_command_crash(char* args, void* context)
{
    rhs_crash("Remote Crash");
}

void cli_command_hardfault(char* args, void* context)
{
    /* Call NULL function pointer: jumps to 0x00000000 (even address → T-bit = 0),
     * triggers INVSTATE UsageFault which escalates to HardFault. */
    ((void (*)(void)) NULL)();
}

void cli_info(char* args, void* context)
{
    PRINT_ALL_VERSIONS();
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
    cli_add_command(app, "crash", cli_command_crash, NULL);
    cli_add_command(app, "hardfault", cli_command_hardfault, NULL);
    cli_add_command(app, "info", cli_info, NULL);

    for (;;)
    {
        cli_process_input(app);
    }
}

/* ---------------------------------------------------------------------------
 * Commands with options: "<cmd> <opt> [args...]"
 * ------------------------------------------------------------------------- */

/** Upper bound of tokens in one line: each token takes at least "x " (2 chars) */
#define CLI_MAX_ARGS (MAX_LINE_LENGTH / 2)

/** Context of a command registered via cli_add_option() */
typedef struct
{
    void*            ctx;      /**< User context passed to cli_add_option() */
    const char*      command;  /**< Command name (not copied, used for output) */
    const CliOption* options;  /**< Option table (not copied) */
    size_t           op_count; /**< Number of entries in @ref options */
} cli_entry_t;

/**
 * @brief Splits @p str into tokens separated by spaces/tabs, in place.
 *
 * Separators are replaced with '\0', empty tokens are not produced.
 * Tokens beyond @p max_tokens are left unparsed.
 *
 * @param str        String to split (modified); may be NULL.
 * @param tokens     Output array of pointers into @p str.
 * @param max_tokens Capacity of @p tokens.
 * @return Number of tokens stored in @p tokens.
 */
static int cli_tokenize(char* str, char* tokens[], int max_tokens)
{
    int count = 0;
    while (str && *str && count < max_tokens)
    {
        while (*str == ' ' || *str == '\t')
            str++;
        if (*str == '\0')
            break;
        tokens[count++] = str;
        while (*str && *str != ' ' && *str != '\t')
            str++;
        if (*str)
            *str++ = '\0';
    }
    return count;
}

/** Prints all options of the command with their descriptions */
static void cli_options_help(cli_entry_t* entry)
{
    printf("%s options:\r\n", entry->command);
    for (size_t i = 0; i < entry->op_count; i++)
    {
        printf("%12s - %s\r\n", entry->options[i].name, entry->options[i].desc);
    }
}

/**
 * @brief CliCallback shared by all commands registered via cli_add_option().
 *
 * Tokenizes @p args, looks up the option by the first token and either
 * prints help or calls the option handler with the remaining tokens.
 *
 * @param args Command line after the command name (modified in place).
 * @param op   cli_entry_t of the command.
 */
static void cli_options_handler(char* args, void* op)
{
    rhs_assert(op);
    cli_entry_t* entry = op;
    char*        av[CLI_MAX_ARGS];
    int          ac = cli_tokenize(args, av, CLI_MAX_ARGS);

    // "<cmd>" or "<cmd> ?" - list all options
    if (ac == 0 || strcmp(av[0], "?") == 0)
    {
        cli_options_help(entry);
        return;
    }

    for (size_t i = 0; i < entry->op_count; i++)
    {
        const CliOption* option = &entry->options[i];
        if (strcmp(av[0], option->name) != 0)
            continue;

        // "<cmd> <opt> ?" - describe a single option
        if (ac > 1 && strcmp(av[1], "?") == 0)
        {
            printf("\t'%s %s' - %s\r\n", entry->command, option->name, option->desc);
            return;
        }
        if (!option->handler)
        {
            printf("ERROR: [CLI] %s: no handler for '%s'\r\n", entry->command, option->name);
            return;
        }
        // Skip the option name: the handler gets only its own arguments
        int err = option->handler(ac - 1, av + 1, entry->ctx);
        if (err)
        {
            printf("WARN: [CLI] %s %s failed (%d)\r\n", entry->command, option->name, err);
        }
        return;
    }
    printf("ERROR: [CLI] %s: unknown option '%s'. '?' for available options\r\n", entry->command, av[0]);
}

/**
 * Allocates a cli_entry_t for the command and registers it with the common
 * cli_options_handler(). The entry is freed by cli_remove_option().
 */
void cli_add_option(Cli* app, const char* name, const CliOptionList opList, void* context)
{
    rhs_assert(app && name);
    rhs_assert(name[0] != ' ');
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    CliCommand* existed = CliCommandDict_get(app->commands, name);
    if (!existed)
    {
        cli_entry_t* cli_ctx = calloc(1, sizeof(cli_entry_t));
        if (cli_ctx)
        {
            cli_ctx->ctx      = context;
            cli_ctx->command  = name;
            cli_ctx->op_count = opList.count;
            cli_ctx->options  = opList.list;
            // Mutex is already held: cli_add_command() would deadlock on it
            CliCommand command = {
                .context  = cli_ctx,
                .callback = cli_options_handler,
            };
            CliCommandDict_set_at(app->commands, name, command);
        }
        else
        {
            printf("ERROR: [CLI] Failed to add command '%s' - out of heap memory\r\n", name);
        }
    }
    else
    {
        printf("ERROR: [CLI] Command '%s' already registered\r\n", name);
    }
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}

void cli_remove_option(Cli* app, const char* name)
{
    rhs_assert(app && name);
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);

    CliCommand* command = CliCommandDict_get(app->commands, name);
    if (command)
    {
        if (command->callback == cli_options_handler)
        {
            free(command->context);
        }
        // Mutex is already held: cli_remove_command() would deadlock on it
        CliCommandDict_erase(app->commands, name);
    }
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}
