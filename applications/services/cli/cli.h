#pragma once

#define RECORD_CLI "cli"

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
