# loader

System start sequence. Always on.

- Task: `loader_srv`, stack 2048
- Record name: `RECORD_LOADER` (reserved, the record is not created yet)

After all services are started, the loader:

1. runs the functions registered with `test()` (`RHS_TESTS`);
2. runs the startup hooks registered with `start_up()` (`RHS_START_UP`);
3. waits on its message queue (reserved for loading applications).
