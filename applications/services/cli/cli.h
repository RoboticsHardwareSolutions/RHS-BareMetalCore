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
 * @param ctx User context passed to cli_add_option().
 * @return 0 on success, otherwise an error code that is printed to the CLI.
 */
typedef int (*CliOptionHandler)(int ac, char* av[], void* ctx);

/** Description of a single command option. */
typedef struct
{
    const char* const name;    /**< Option name, matched against the first argument */
    const char* const desc;    /**< Short description shown in help */
    CliOptionHandler  handler; /**< Option handler; NULL if the option is not implemented */
} CliOption;

/** Table of options belonging to one command. */
typedef struct
{
    const CliOption* list;  /**< Option table */
    const unsigned   count; /**< Number of entries in @ref list */
} CliOptionList;

/**
 * @brief Builds a CliOptionList from a static array of CliOption.
 *
 * @p opts must be an array (not a pointer), its size is computed with sizeof.
 * Example: cli_add_option(cli, "mc", CLI_OPTION_LIST(mc_options), NULL);
 */
#define CLI_OPTION_LIST(opts)                                   \
    (CliOptionList)                                             \
    {                                                           \
        .list = (opts), .count = sizeof(opts) / sizeof(*(opts)) \
    }

/**
 * @brief Registers command @p name that dispatches to a table of options.
 *
 * Supported syntax:
 *  - "<cmd>" or "<cmd> ?"    - print the list of options with descriptions;
 *  - "<cmd> <opt> ?"         - print the description of a single option;
 *  - "<cmd> <opt> [args...]" - call the option handler with [args...].
 *
 * @param app     CLI instance.
 * @param name    Command name. Stored by pointer for help/error output,
 *                so it must outlive the registration (e.g. a string literal).
 * @param opList  Option table. The underlying CliOption array is stored by
 *                pointer and must outlive the registration (static storage).
 * @param context User context passed to every option handler as @p ctx.
 *
 * Example:
 * @code
 * static int mc_speed(int ac, char* av[], void* ctx)
 * {
 *     Motor* motor = ctx;
 *     if (ac != 1)
 *         return -1; // printed as "mc speed failed (-1)"
 *     motor_set_speed(motor, atoi(av[0]));
 *     return 0;
 * }
 *
 * static int mc_stop(int ac, char* av[], void* ctx)
 * {
 *     motor_stop((Motor*)ctx);
 *     return 0;
 * }
 *
 * static const CliOption mc_options[] = {
 *     {"speed", "Set speed: speed <rpm>", mc_speed},
 *     {"stop", "Stop motor", mc_stop},
 * };
 *
 * Cli* cli = rhs_record_open(RECORD_CLI);
 * cli_add_option(cli, "mc", CLI_OPTION_LIST(mc_options), motor);
 * rhs_record_close(RECORD_CLI);
 *
 * // > mc            - list options
 * // > mc speed ?    - describe "speed"
 * // > mc speed 1500 - mc_speed(1, {"1500"}, motor)
 *
 * cli = rhs_record_open(RECORD_CLI);
 * cli_remove_option(cli, "mc");
 * rhs_record_close(RECORD_CLI);
 * @endcode
 */
void cli_add_option(Cli* app, const char* name, CliOptionList opList, void* context);

/**
 * @brief Removes a command registered via cli_add_option() and frees its
 *        internal context.
 *
 * Use it instead of cli_remove_command() for such commands, otherwise the
 * context allocated by cli_add_option() leaks. Commands registered via
 * cli_add_command() are removed too; their context is not freed.
 *
 * @param app  CLI instance.
 * @param name Command name.
 */
void cli_remove_option(Cli* app, const char* name);

#ifdef __cplusplus
}  // extern "C"
#endif
