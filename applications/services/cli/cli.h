#pragma once

#define RECORD_CLI "cli"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CliSymbolAsciiSOH       = 0x01,
    CliSymbolAsciiETX       = 0x03,
    CliSymbolAsciiEOT       = 0x04,
    CliSymbolAsciiBell      = 0x07,
    CliSymbolAsciiBackspace = 0x08,
    CliSymbolAsciiTab       = 0x09,
    CliSymbolAsciiLF        = 0x0A,
    CliSymbolAsciiCR        = 0x0D,
    CliSymbolAsciiEsc       = 0x1B,
    CliSymbolAsciiUS        = 0x1F,
    CliSymbolAsciiSpace     = 0x20,
    CliSymbolAsciiDel       = 0x7F,
} CliSymbols;

typedef struct Cli Cli;

typedef void (*CliCallback)(char* args, void* context);

void cli_add_command(Cli* app, const char* name, CliCallback callback, void* context);

void cli_remove_command(Cli* app, const char* name);

/**
 * @brief Handler of a single command option (sub-command).
 *
 * Called when the user enters "<cmd> <opt> [args...]".
 *
 * @param ac Number of arguments following the option name (may be 0).
 * @param av Arguments after the option name, split by spaces.
 *           Each av[i] is a null-terminated string, valid only during the call.
 * @param ctx User context passed to cli_add_command_options().
 * @return 0 on success, otherwise an error code printed as "<cmd> <opt> failed (<code>)".
 */
typedef int (*CliOptionHandler)(int ac, char* av[], void* ctx);

/** Description of a single command option. */
typedef struct
{
    const char* const name;    /**< Option name, matched against the first argument */
    const char* const desc;    /**< Short description shown by "<cmd> ?" and "<cmd> <opt> ?" */
    CliOptionHandler  handler; /**< Option handler; NULL if the option is not implemented */
} CliOption;

/** Table of options belonging to one command. */
typedef struct
{
    const CliOption* list;  /**< Option table */
    unsigned         count; /**< Number of entries in @ref list */
} CliOptionList;

/**
 * @brief Builds a CliOptionList from a static array of CliOption.
 *
 * @p opts must be an array (not a pointer), its size is computed with sizeof.
 * Example: cli_add_command_options(cli, "mc", CLI_OPTION_LIST(mc_options), NULL);
 */
#define CLI_OPTION_LIST(opts)                                   \
    (CliOptionList)                                             \
    {                                                           \
        .list = (opts), .count = sizeof(opts) / sizeof(*(opts)) \
    }

/**
 * @brief Registers command @p name that dispatches to a table of options.
 *
 * Supported syntax (tokens are separated by spaces/tabs):
 *  - "<cmd>" or "<cmd> ?"    - print the list of options with descriptions;
 *  - "<cmd> <opt> ?"         - print the description of a single option;
 *  - "<cmd> <opt> [args...]" - call the option handler with [args...].
 * An unknown <opt> prints an error with a hint to use "<cmd> ?".
 *
 * @param app     CLI instance.
 * @param name    Command name (copied): non-empty, without spaces/tabs.
 *                If it is already registered, an error is logged and the
 *                existing command is kept.
 * @param opList  Option table. The underlying CliOption array is stored by
 *                pointer and must outlive the registration (static storage).
 * @param context User context passed to every option handler as @p ctx.
 *
 * Option handlers have the same restrictions as CliCallback, see cli_add_command().
 *
 * Example:
 * @code
 * static int foo_bar(int ac, char* av[], void* ctx)
 * {
 *     Foo* foo = ctx;
 *     if (ac != 1)
 *         return -1; // printed as "foo bar failed (-1)"
 *     foo_do_bar(foo, atoi(av[0]));
 *     return 0;
 * }
 *
 * static int foo_baz(int ac, char* av[], void* ctx)
 * {
 *     foo_do_baz((Foo*)ctx);
 *     return 0;
 * }
 *
 * static const CliOption foo_options[] = {
 *     {"bar", "Do bar: bar <value>", foo_bar},
 *     {"baz", "Do baz", foo_baz},
 * };
 *
 * Cli* cli = rhs_record_open(RECORD_CLI);
 * cli_add_command_options(cli, "foo", CLI_OPTION_LIST(foo_options), foo);
 * rhs_record_close(RECORD_CLI);
 *
 * // > foo        - list options
 * // > foo bar ?  - describe "bar"
 * // > foo bar 42 - foo_bar(1, {"42"}, foo)
 *
 * cli = rhs_record_open(RECORD_CLI);
 * cli_remove_command(cli, "foo");
 * rhs_record_close(RECORD_CLI);
 * @endcode
 */
void cli_add_command_options(Cli* app, const char* name, CliOptionList opList, void* context);

#ifdef __cplusplus
}  // extern "C"
#endif
