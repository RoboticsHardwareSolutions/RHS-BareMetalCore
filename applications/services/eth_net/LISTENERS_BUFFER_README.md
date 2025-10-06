# Dynamic Listener Buffer

## Description

Implements a mechanism for dynamically storing all registered HTTP and TCP listeners in a linked list. During network manager restart, all listeners are automatically restored.

## Module Files

- **`eth_net_listeners.h`** - Public API for listener list management
- **`eth_net_listeners.c`** - Implementation of list management functions
- **`eth_net_srv.h`** - Main service (uses listeners API)
- **`eth_net_srv.c`** - Main service implementation

## Architecture

### Data Structures

#### `EthNetListener`
Linked list node storing information about a single listener:
```c
typedef struct EthNetListener {
    EthNetListenerType     type;      // Type: HTTP or TCP
    char*                  uri;       // URI (e.g., "http://0.0.0.0:80" or "tcp://0.0.0.0:502")
    mg_event_handler_t     fn;        // Callback event handler function
    void*                  context;   // User context
    struct EthNetListener* next;      // Pointer to next element
} EthNetListener;
```

#### Added to `EthNet`
```c
struct EthNet {
    ...
    EthNetListener* listeners;  // Head of listeners linked list
};
```

## API Functions (eth_net_listeners.h)

### `eth_net_listeners_add()`
```c
void eth_net_listeners_add(EthNetListener** listeners, 
                           EthNetListenerType type, 
                           const char* uri, 
                           mg_event_handler_t fn, 
                           void* context);
```
Adds a new listener to the end of the list.
- **listeners** - pointer to list head
- **type** - listener type (HTTP/TCP)
- **uri** - URI string (will be copied)
- **fn** - event handler function
- **context** - user context

**What it does:**
- Allocates memory for `EthNetListener` structure
- Copies URI string (dynamic memory allocation)
- Stores all parameters (type, handler function, context)
- Adds element to the end of list

### `eth_net_listeners_restore()`
```c
void eth_net_listeners_restore(EthNetListener* listeners, 
                               struct mg_mgr* mgr);
```
Restores all listeners after restart.
- **listeners** - head of listener list
- **mgr** - Mongoose manager for registration

**What it does:**
- Iterates through entire linked list
- For each listener calls corresponding Mongoose function:
  - `mg_http_listen()` for HTTP
  - `mg_listen()` for TCP
- Logs each restoration

### `eth_net_listeners_free()`
```c
void eth_net_listeners_free(EthNetListener** listeners);
```
Frees memory of all listeners.
- **listeners** - pointer to list head

**What it does:**
- Iterates through list and frees URI memory and structures
- Sets list pointer to NULL

### `eth_net_listeners_count()`
```c
size_t eth_net_listeners_count(EthNetListener* listeners);
```
Returns the number of listeners in the list.
- **listeners** - list head
- **return** - number of elements

## Usage

### Listener Registration
Works automatically when called:
```c
eth_net_start_http(eth_net, "http://0.0.0.0:80", my_http_handler, context);
eth_net_start_listener(eth_net, "tcp://0.0.0.0:502", my_tcp_handler, context);
```

Internally calls automatically:
```c
eth_net_listeners_add(&app->listeners, EthNetListenerTypeHttp, uri, fn, context);
```

### Restart with Restoration
When restarting network manager:
```c
eth_net_restart_manager(eth_net);
```

Automatically performs:
1. Closing all current connections
2. Updating IP address (if changed)
3. **Restoring all listeners** from buffer:
```c
eth_net_listeners_restore(app->listeners, app->mgr);
```

### Direct API Usage
You can use the API directly for more flexible control:

```c
// Create list
EthNetListener* my_listeners = NULL;

// Add listeners
eth_net_listeners_add(&my_listeners, EthNetListenerTypeHttp, 
                      "http://0.0.0.0:80", handler, ctx);
eth_net_listeners_add(&my_listeners, EthNetListenerTypeTcp, 
                      "tcp://0.0.0.0:502", handler, ctx);

// Get count
size_t count = eth_net_listeners_count(my_listeners);
printf("Total listeners: %zu\n", count);

// Restore all
eth_net_listeners_restore(my_listeners, mgr);

// Free memory
eth_net_listeners_free(&my_listeners);
```

## Advantages

✅ **Modularity** - API extracted to separate files, easy to use in other modules  
✅ **Dynamic memory allocation** - list grows as listeners are added  
✅ **Automatic restoration** - all listeners are recreated on restart  
✅ **Context preservation** - all parameters (URI, callbacks, context) are saved  
✅ **Transparency** - main API unchanged, functionality added internally  
✅ **Logging** - every action logged via `MG_INFO()`  
✅ **Reusability** - can be used in other projects

## Log Example

```
[INFO] Listener added: type=0, uri=http://0.0.0.0:80
[INFO] HTTP server started on http://0.0.0.0:80
[INFO] Listener added: type=1, uri=tcp://0.0.0.0:502
[INFO] TCP server started on tcp://0.0.0.0:502
[INFO] Restoring listeners...
[INFO] Restored HTTP server on http://0.0.0.0:80
[INFO] Restored TCP server on tcp://0.0.0.0:502
```

## Memory Consumption

Per listener:
- `sizeof(EthNetListener)` ≈ 24-32 bytes (depends on architecture)
- `strlen(uri) + 1` bytes for URI string
- **Total**: approximately 50-100 bytes per listener

## Operation Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    EthNet Service                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  eth_net_start_http() ──────┐                               │
│  eth_net_start_listener() ──┼──>  eth_net_listeners_add()   │
│                             │                               │
│                             v                               │
│                    ┌─────────────────┐                      │
│                    │ EthNetListener  │                      │
│                    │   - type        │                      │
│                    │   - uri         │                      │
│                    │   - fn          │                      │
│                    │   - context     │                      │
│                    │   - next ───────┼───> ...              │
│                    └─────────────────┘                      │
│                                                             │
│  eth_net_restart_manager()                                  │
│         │                                                   │
│         v                                                   │
│  eth_net_listeners_restore() ──> Restores all               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Future Improvements

- [ ] Duplicate protection (check existing URIs before adding)
- [ ] Function to remove specific listener by URI
- [ ] Thread-safety (mutex for list access from different threads)
- [ ] Save to non-volatile memory for full restoration after reboot
- [ ] Listener priority support
- [ ] Callback on listener add/remove
