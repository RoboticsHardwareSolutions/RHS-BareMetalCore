#include "cli.h"
#include "rhs.h"
#include "rhs_hal.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include "rhs_version.h"
#include <m-dict.h>
#include "core/m_cstr_dup.h"

#define TAG "cli"

#define MAX_LINE_LENGTH 64

/** Upper bound of tokens in one line: each token takes at least "x " (2 chars) */
#define CLI_MAX_ARGS (MAX_LINE_LENGTH / 2)

typedef struct
{
    void* context; /**< User context of the callback; CliOptionList for option commands */
    CliCallback callback; /**< Raw-line handler: user one or cli_options_handler() for option commands */
    uint32_t    flags;    /**< Not used yet */
} CliCommand;

DICT_DEF2(CliCommandDict, const char*, M_CSTR_DUP_OPLIST, CliCommand, M_POD_OPLIST);

struct Cli
{
    RHSMutex*        mutex;
    uint8_t          cursor_position;
    char             line[MAX_LINE_LENGTH];
    CliCommandDict_t commands;
};

/**
 * @brief CliCallback of option commands: dispatches "<cmd> <opt> [args...]" to the option table.
 *
 * Tokenizes @p args, looks up the option by name and either prints help or
 * calls the option handler with the tokens after the option name and
 * CliOptionList::context.
 * Unknown options and handler errors are reported to the CLI; options without a handler are ignored.
 *
 * @param args Command line (modified in place); may be NULL.
 * @param ctx  CliOptionList registered by cli_add_command_options(), with the user context inside.
 */
static void cli_options_handler(char* args, void* ctx);

/**
 * @brief Adds new command to command dictionary (under the CLI mutex).
 *
 * An already registered name is not replaced: an error is logged instead.
 *
 * @param app CLI instance
 * @param name Command name (copied): starts with a visible character, no spaces/tabs (asserted)
 * @param cmd Command entity
 */
static void cli_add_to_dict(Cli* app, const char* name, CliCommand cmd)
{
    rhs_assert(app && name);
    rhs_assert(app->commands);
    rhs_assert(isgraph((unsigned char) name[0]));
    rhs_assert(strpbrk(name, " \t") == NULL);
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    if (CliCommandDict_get(app->commands, name))
    {
        RHS_LOG_E(TAG, "Command '%s' already registered", name);
    }
    else
    {
        CliCommandDict_set_at(app->commands, name, cmd);
    }
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}

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
    rhs_assert(app && callback);
    CliCommand command = {
        .context  = context,
        .callback = callback,
    };
    cli_add_to_dict(app, name, command);
}

void cli_add_command_options(Cli* app, const char* name, CliOptionList* list, void* context)
{
    rhs_assert(list);
    list->context = context;
    cli_add_command(app, name, cli_options_handler, list);
}

void cli_remove_command(Cli* app, const char* name)
{
    rhs_assert(app && name);
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    CliCommandDict_erase(app->commands, name);
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
}

static char cli_getc(void)
{
    static bool enter = false;

    char c = getchar();

    // A line end ends the command at once: a next command already in the buffer is not glued to it
    if (c == '\r' || c == '\n')
    {
        enter = false;
        c     = CliSymbolAsciiCR;
    }
    // A terminal which sends no line end: the command ends when the buffer runs empty
    else if (c != 0)
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

static void cli_reset(Cli* app)
{
    app->cursor_position = 0;
    memset(app->line, 0, MAX_LINE_LENGTH);
}

static void cli_handle_enter(Cli* app)
{
    rhs_assert(app);

    // Legacy callbacks get the raw rest of the line: NULL if there is no separator
    char* args = strpbrk(app->line, " \t");
    if (args)
        *args++ = '\0';

    // The command runs under the mutex, so it can't be removed while running.
    // Thus handlers must not (un)register CLI commands: the mutex is not recursive.
    rhs_assert(rhs_mutex_acquire(app->mutex, RHSWaitForever) == RHSStatusOk);
    const CliCommand* command = CliCommandDict_get(app->commands, app->line);
    if (command && command->callback)
    {
        command->callback(args, command->context);
    }
    else
    {
        printf("Unknown command. Type '?' for list of commands.\r\n");
    }
    rhs_assert(rhs_mutex_release(app->mutex) == RHSStatusOk);
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
        // The second half of CRLF is an empty line, not a command
        if (app->cursor_position > 0)
        {
            cli_handle_enter(app);
        }
    }
    else if ((in_chr >= 0x20 && in_chr < 0x7F))
    {
        // Keep the last byte for the terminating '\0', extra characters are dropped
        if (app->cursor_position < MAX_LINE_LENGTH - 1)
        {
            app->line[app->cursor_position] = in_chr;
            app->cursor_position++;
        }
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

#define CLI_TOP_INTERVAL_MS (1000U)
#define CLI_TOP_INTERVAL_MAX_MS (60000U)

/* top [ms]: CPU usage over the interval, measured by two samples, so the first output is valid too */
void cli_command_top(char* args, void* context)
{
    uint32_t interval_ms = CLI_TOP_INTERVAL_MS;
    if (args != NULL)
    {
        /* Not strtoul: it would be the only user of it in a Release image */
        interval_ms = 0U;
        for (const char* c = args; interval_ms <= CLI_TOP_INTERVAL_MAX_MS; c++)
        {
            if (!isdigit((unsigned char) *c))
            {
                if (*c != '\0' || c == args)
                    interval_ms = 0U;
                break;
            }
            interval_ms = interval_ms * 10U + (uint32_t) (*c - '0');
        }
        if (interval_ms == 0U || interval_ms > CLI_TOP_INTERVAL_MAX_MS)
        {
            printf("top [1-%u ms]\r\n", CLI_TOP_INTERVAL_MAX_MS);
            return;
        }
    }

    static RHSThreadList* thread_list = NULL;
    if (!thread_list)  // FIXME: It must be separate thread
        thread_list = rhs_thread_list_create();
    uint16_t count;

    rhs_thread_enumerate(thread_list);
    rhs_delay_ms(interval_ms);
    rhs_thread_enumerate(thread_list);
    count = rhs_thread_list_size(thread_list);

    /* The name column is as wide as the longest name */
    int name_width = 4;
    for (size_t i = 0; i < count; i++)
    {
        int len = (int) strlen(rhs_thread_list_at(thread_list, i)->name);
        if (len > name_width)
            name_width = len;
    }

    printf("Tasks: %u, CPU over %lu ms, Stack: min free, words\r\n", count, (unsigned long) interval_ms);
    printf("%-*s  %-9s  %4s  %4s  %5s\r\n", name_width, "Name", "State", "Prio", "CPU", "Stack");

    for (size_t i = 0; i < count; i++)
    {
        RHSThreadListItem* item = rhs_thread_list_at(thread_list, i);
        char               cpu[8];
        if (item->cpu < 1)
            snprintf(cpu, sizeof(cpu), "<1%%");
        else
            snprintf(cpu, sizeof(cpu), "%u%%", item->cpu);
        printf("%-*s  %-9s  %4d  %4s  %5lu\r\n",
               name_width,
               item->name,
               item->state,
               (int) item->priority,
               cpu,
               (unsigned long) item->stack_min_free);
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

int32_t cli_srv(void* context)
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

    return 0;
}

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

/** Prints names of all options of the command, two per line */
static void cli_options_help(const CliOptionList* options)
{
    printf("Options:\r\n");
    for (size_t i = 0; i < options->count; i++)
    {
        printf("%-12s", options->list[i].name);
        (i % 2 == 1) ? printf("\r\n") : printf(" ");
    }
    if (options->count % 2 == 1)
        printf("\r\n");
}

static void cli_options_handler(char* args, void* ctx)
{
    CliOptionList* options = ctx;
    char*          av[CLI_MAX_ARGS];
    int            ac = cli_tokenize(args, av, CLI_MAX_ARGS);

    if (ac == 0 || strcmp(av[0], "?") == 0)
    {
        cli_options_help(options);
        return;
    }

    for (size_t i = 0; i < options->count; i++)
    {
        const CliOption* option = &options->list[i];
        if (strcmp(av[0], option->name) != 0)
            continue;

        // "<cmd> <opt> ?" - describe a single option
        if (ac > 1 && strcmp(av[1], "?") == 0)
        {
            printf("Usage: '%s' - %s\r\n", option->name, option->desc);
            return;
        }
        if (option->handler)
        {
            int err = option->handler(ac - 1, av + 1, options->context);
            if (err)
            {
                printf("%s failed (code %d)\r\n", option->name, err);
            }
        }
        return;
    }
    printf("Unknown option '%s'. '?' for available options\r\n", av[0]);
}
