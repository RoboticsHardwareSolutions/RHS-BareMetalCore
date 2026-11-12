# cli

Interactive command line over the RHS log stream (RTT/UART). Always on.

- Task: `cli_srv`, stack 4096
- Record: `RECORD_CLI`

## API (`cli.h`)

| Function | Description |
|---|---|
| `cli_add_command(cli, name, callback, context)` | Register a command with a raw-line handler |
| `cli_add_command_options(cli, name, list, context)` | Register a command with sub-options (`CLI_OPTION_LIST`) and built-in `?` help |
| `cli_remove_command(cli, name)` | Unregister a command |

```c
Cli* cli = rhs_record_open(RECORD_CLI);
cli_add_command(cli, "my_cmd", my_cmd_cb, context);
rhs_record_close(RECORD_CLI);
```

## Built-in commands

`?`, `uptime`, `free`, `log`, `reset`, `uid`, `top`, `info`, `crash`, `hardfault`.
