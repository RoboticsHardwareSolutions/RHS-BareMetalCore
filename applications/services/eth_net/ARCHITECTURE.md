# Architecture of eth_net Module with Dynamic Listener Buffer

## Module Structure

```
┌────────────────────────────────────────────────────────────────────┐
│                         Public API                                 │
├────────────────────────────────────────────────────────────────────┤
│                         eth_net.h                                  │
│  • eth_net_start_http()                                            │
│  • eth_net_start_listener()                                        │
│  • eth_net_restart_manager()                                       │
└────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────┐
│                    Service Implementation                          │
├────────────────────────────────────────────────────────────────────┤
│                    eth_net_srv.c / .h                              │
│  • EthNet structure                                                │
│  • eth_net_service()  - main loop                                  │
│  • eth_net_cli_command() - CLI interface                           │
│  • ethernet_init() / deinit()                                      │
└────────────────────────────────────────────────────────────────────┘
                                   │
                                   │ uses
                                   ▼
┌────────────────────────────────────────────────────────────────────┐
│              Listener Management Module                            │
├────────────────────────────────────────────────────────────────────┤
│               eth_net_listeners.c / .h                             │
│  • eth_net_listeners_add()      - add listener                     │
│  • eth_net_listeners_restore()  - restore all                      │
│  • eth_net_listeners_free()     - free memory                      │
│  • eth_net_listeners_count()    - count                            │
└────────────────────────────────────────────────────────────────────┘
                                   │
                                   │ manages
                                   ▼
┌────────────────────────────────────────────────────────────────────┐
│                      Data Structure                                │
├────────────────────────────────────────────────────────────────────┤
│                     EthNetListener                                 │
│                                                                    │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐          │
│  │ Listener #1  │───▶│ Listener #2  │───▶│ Listener #3  │───▶      │
│  ├──────────────┤    ├──────────────┤    ├──────────────┤          │
│  │ type: HTTP   │    │ type: TCP    │    │ type: HTTP   │          │
│  │ uri: ":80"   │    │ uri: ":502"  │    │ uri: ":8080" │          │
│  │ fn: handler  │    │ fn: modbus   │    │ fn: api_h    │          │
│  │ context: ctx │    │ context: ctx │    │ context: ctx │          │
│  │ next: ───────┤    │ next: ───────┤    │ next: NULL   │          │
│  └──────────────┘    └──────────────┘    └──────────────┘          │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
                                   │
                                   │ uses
                                   ▼
┌────────────────────────────────────────────────────────────────────┐
│                      Mongoose library                              │
├────────────────────────────────────────────────────────────────────┤
│                        mongoose.h                                  │
│  • mg_mgr                                                          │
│  • mg_http_listen()                                                │
│  • mg_listen()                                                     │
│  • mg_tcpip_init()                                                 │
└────────────────────────────────────────────────────────────────────┘
```

## Data Flow When Adding a Listener

```
Application
    │
    │ eth_net_start_http(eth_net, "http://0.0.0.0:80", handler, ctx)
    ▼
┌───────────────────────────┐
│   eth_net_srv.c           │
│   • Create message        │
│   • Send to queue         │
└───────────────────────────┘
    │
    │ EthNetApiEventMessage
    ▼
┌───────────────────────────┐
│   eth_net_service()       │
│   • Receive message       │
│   • mg_http_listen()      │◄─── Register in Mongoose
│   • listeners_add()       │◄─── Save to buffer
└───────────────────────────┘
    │
    ▼
┌───────────────────────────┐
│  eth_net_listeners.c      │
│  • malloc(EthNetListener) │
│  • malloc(uri)            │
│  • strcpy(uri)            │
│  • Add to list            │
└───────────────────────────┘
```

## Data Flow During Restart

```
CLI command: "eth -restart"
    │
    ▼
┌───────────────────────────┐
│  eth_net_cli_command()    │
│  • eth_net_restart_mgr()  │
└───────────────────────────┘
    │
    │ EthNetApiEventTypeRestart
    ▼
┌───────────────────────────┐
│   eth_net_service()       │
│   • Close connections     │
│   • Update IP             │
│   • listeners_restore()   │─────┐
└───────────────────────────┘     │
                                  │
                                  ▼
┌─────────────────────────────────────────────────┐
│  eth_net_listeners_restore(listeners, mgr)      │
│                                                 │
│  Iterate through list:                          │
│  ┌──────────────────────────────────────────┐   │
│  │ for (listener = head; ...; next)         │   │
│  │ {                                        │   │
│  │   if (HTTP) mg_http_listen()             │───┼──▶ Mongoose
│  │   if (TCP)  mg_listen()                  │   │
│  │ }                                        │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

## Listener Lifecycle

```
1. CREATION
   ┌─────────────────────────┐
   │ eth_net_start_http()    │
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ mg_http_listen()        │ ──▶ Active listener in Mongoose
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ listeners_add()         │ ──▶ Saved in buffer
   └─────────────────────────┘

2. OPERATION
   ┌─────────────────────────┐
   │ mg_mgr_poll()           │ ──▶ Event processing
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ Call handler()          │ ──▶ Request handling
   └─────────────────────────┘

3. RESTART
   ┌─────────────────────────┐
   │ eth_net_restart_mgr()   │
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ Close connections       │
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ listeners_restore()     │
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ mg_http_listen()        │ ──▶ Listener restored
   └─────────────────────────┘

4. CLEANUP (optional)
   ┌─────────────────────────┐
   │ listeners_free()        │
   └─────────────────────────┘
              │
              ▼
   ┌─────────────────────────┐
   │ free(uri)               │
   │ free(listener)          │
   └─────────────────────────┘
```

## Component Dependencies

```
┌─────────────┐
│ Application │
└─────────────┘
      │
      │ uses
      ▼
┌─────────────┐     includes    ┌────────────────────┐
│  eth_net.h  │◀────────────────│  eth_net_srv.h     │
└─────────────┘                 └────────────────────┘
                                         │
                                         │ includes
                                         ▼
                                ┌────────────────────┐
                                │eth_net_listeners.h │
                                └────────────────────┘
                                         │
                                         │ uses
                                         ▼
                                ┌────────────────────┐
                                │   mongoose.h       │
                                └────────────────────┘
                                         │
                                         │ uses
                                         ▼
                                ┌────────────────────┐
                                │      rhs.h         │
                                └────────────────────┘
```

## Resource Consumption

### Memory (example with 3 listeners)

```
Stack (static memory):
  └─ EthNet structure:          ~200 bytes
      └─ EthNetListener* head:    4-8 bytes (pointer)

Heap (dynamic memory):
  ├─ Listener #1:               ~80 bytes
  │   ├─ EthNetListener:         24-32 bytes
  │   └─ URI string:             ~40 bytes ("http://0.0.0.0:80")
  │
  ├─ Listener #2:               ~80 bytes
  │   ├─ EthNetListener:         24-32 bytes
  │   └─ URI string:             ~40 bytes ("tcp://0.0.0.0:502")
  │
  └─ Listener #3:               ~90 bytes
      ├─ EthNetListener:         24-32 bytes
      └─ URI string:             ~50 bytes ("http://0.0.0.0:8080")

TOTAL Heap: ~250 bytes for 3 listeners
```

### Execution Time

```
Add listener:                 O(n) - search for end of list
Restore all:                  O(n) - iterate through list
Count elements:               O(n) - iterate through list
Free memory:                  O(n) - iterate through list

where n - number of listeners (typically < 10)
```
