# RHS HAL Flash Ex - Usage Example

This module provides interrupt-safe flash operations for external QSPI flash memory.

## Features

- **Interrupt Safe**: All functions can be called from both task context and interrupt context
- **Non-blocking ISR Operations**: When called from ISR, operations use non-blocking mutex acquire (timeout = 0)
- **Clear Error Codes**: Specific return codes for different error conditions

## Return Codes

```c
#define RHS_FLASH_EX_OK      0   // Success
#define RHS_FLASH_EX_ERROR  -1   // General error
#define RHS_FLASH_EX_BUSY   -2   // Flash is busy (when called from ISR)
```

## Usage Examples

### Reading from Task Context
```c
uint8_t buffer[256];
int result = rhs_hal_flash_ex_read(0x1000, buffer, sizeof(buffer));

if (result == RHS_FLASH_EX_OK) {
    // Success - data read into buffer
} else if (result == RHS_FLASH_EX_ERROR) {
    // Flash operation failed
}
```

### Reading from Interrupt Context
```c
// In interrupt handler
void timer_interrupt_handler(void) {
    uint8_t urgent_data[32];
    int result = rhs_hal_flash_ex_read(0x2000, urgent_data, sizeof(urgent_data));
    
    if (result == RHS_FLASH_EX_OK) {
        // Success - data read immediately
        process_urgent_data(urgent_data);
    } else if (result == RHS_FLASH_EX_BUSY) {
        // Flash is currently in use by another task
        // Schedule deferred read or retry later
        schedule_deferred_read(0x2000, sizeof(urgent_data));
    } else {
        // Flash operation error
        log_error("Flash read failed in ISR");
    }
}
```

### Writing from Task Context
```c
uint8_t data[256] = { /* your data */ };
int result = rhs_hal_flash_ex_write(0x3000, data, sizeof(data));

switch (result) {
    case RHS_FLASH_EX_OK:
        // Write successful
        break;
    case RHS_FLASH_EX_ERROR:
        // Write failed
        break;
}
```

### Error Handling Best Practices

```c
int flash_operation_with_retry(uint32_t addr, uint8_t* data, uint32_t size) {
    int result = rhs_hal_flash_ex_read(addr, data, size);
    
    if (result == RHS_FLASH_EX_BUSY && RHS_IS_ISR()) {
        // We're in ISR and flash is busy - cannot wait
        return result;  // Caller should handle retry
    } else if (result == RHS_FLASH_EX_ERROR) {
        // Hardware error - log and return
        rhs_log_error("Flash read error at 0x%08X", addr);
        return result;
    }
    
    return result;
}
```

## Thread Safety

All functions are thread-safe and use mutex protection internally. The mutex behavior depends on calling context:

- **Task Context**: Uses blocking mutex acquire with infinite timeout
- **ISR Context**: Uses non-blocking mutex acquire (timeout = 0)

## Important Notes

1. **ISR Context**: Functions return `RHS_FLASH_EX_BUSY` if mutex is unavailable when called from ISR
2. **Performance**: ISR operations are non-blocking but may fail if flash is busy with another operation
3. **Error Handling**: Always check return codes and handle `RHS_FLASH_EX_BUSY` appropriately in ISR context
