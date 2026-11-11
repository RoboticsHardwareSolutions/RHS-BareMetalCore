# notification

Plays sound sequences on the speaker (`rhs_hal_speaker`).

- CMake option: `RHS_SERVICE_NOTIFICATION` (also enables `rhs_hal_speaker`)
- Task: `notification_srv`, stack 1024
- Record: `RECORD_NOTIFY`

## Files

| File | Content |
|---|---|
| `notification.h` | Message types, `notification_message()` |
| `notification_i.h` | Service state and queue message |
| `notification.c` | Task, message processing, public API |
| `notification_messages.h/.c` | Delays, `sequence_success`, `sequence_error`, ... |
| `notification_messages_notes.h/.c` | Notes `message_note_c0` ... `message_note_b8` |

## Usage

```c
NotificationApp* notify = rhs_record_open(RECORD_NOTIFY);
notification_message(notify, &sequence_success);
rhs_record_close(RECORD_NOTIFY);
```
