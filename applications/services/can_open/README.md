# can_open

CANopen master on top of `co_stack`, `rcan` and `rhs_hal_can`.

- CMake option: `RHS_SERVICE_CAN_OPEN` (also enables `rhs_hal_can`)
- Task: `can_open_srv`, stack 2048
- Record: `RECORD_CAN_OPEN`
- CLI: `can_open`

## Files

| File | Content |
|---|---|
| `can_open.h` | Public API: nodes, SDO, PDO, OD field access |
| `can_open_i.h` | Service state, queue messages, shared globals |
| `can_open.c` | Task: dispatches RX frames, PDO events and TX queue |
| `can_open_stack.c` | `co_stack` glue: node start, SDO, `canSend`, timers |
| `can_open_pdo.c` | PDO configuration / mapping, OD getters |
| `can_open_od.h/.c` | Common object dictionary entries (`od_*`) |
| `can_open_cli.c` | `can_open` CLI command |

## Usage

```c
CanOpenApp* co = rhs_record_open(RECORD_CAN_OPEN);
co_start_node(co, &MyOD_Data, node_id, RHSHalCANId1, 500000);
co_get_sdo(co, &MyOD_Data, node_id, 0x1000, 0x00, on_sdo);
rhs_record_close(RECORD_CAN_OPEN);
```

## CLI

| Command | Action |
|---|---|
| `can_open -f <id>` | Print frames with this CAN ID (hex) |
| `can_open -e <id>` | Stop printing this CAN ID |
| `can_open -c` | Clear the filter list |
| `can_open -l` | List filtered IDs |
| `can_open -s <id> [b0 .. b7]` | Send a frame with this ID and up to 8 data bytes (hex) |
| `can_open -m` | Help |
