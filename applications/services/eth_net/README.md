This example demonstrates how to create a new application that uses the `eth_net` service to start a TCP listener on port 2345. The `_ev_handler` function will be called whenever data is received on that port, allowing you to process incoming connections.

### Creating a new application

0. Set the `eth_net` service in your toolchain `set(RHS_SERVICE_ETH_NET ON)`
1. Create a new folder in the project
2. Create a new CMakeLists.txt file in the folder with the following content:
```cmake
cmake_minimum_required(VERSION 3.24)
project(new_app C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC new_app.c) # Replace `new_app.c` with your source file

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
) # Include the current directory for the new app

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
        eth_net
)

service(new_app "new_app" 1024) # Registers the new application as a service with a stack size of 1024 bytes. You can adjust the stack size as needed. This means that rhs core will create a thread for this service and run the `new_app` function in that thread. You not need to call `new_app` function anywhere, it will be called automatically when the service starts.
``` 

3. Create a new source file in the same folder with the same name as the project (e.g., `new_app.c`) and implement your application logic.

```c
#include "rhs.h"
#include "rhs_hal.h"
#include "eth_net.h"

#define TAG "new_app"

struct NewApp
{
    EthNet*  net; // Network manager instance
    RHSevent* event; // Event for synchronization
    // Add other members as needed
};

NewApp* app_alloc(void)
{
    NewApp* app = malloc(sizeof(NewApp)); // Allocate memory for the application structure
    app->net    = rhs_record_open(RECORD_ETH_NET); // Get a network manager instance. Your application will be waiting while EthNet service is not ready, so you can be sure that this call will succeed.
    app->event  = rhs_event_alloc(); // Allocate an event for synchronization
    return app;
}

static void _ev_handler(struct mg_connection* c, int ev, void* ev_data)
{
    NewApp* app = (NewApp*) c->fn_data; // This is the context we passed when starting the listener
    rhs_assert(app); // Ensure we have a valid application context if you use it

    if (ev == MG_EV_READ)
    {
        // c->recv.buf contains the received data
        // c->recv.len is the length of the data
        uint8_t* data = (uint8_t*) c->recv.buf;
        size_t data_len = c->recv.len;
        // Process the data as needed
        // For example, send a response back
        const char* response = "Received data!";
        size_t response_len = strlen(response);
        mg_send(c, response, response_len);
        rhs_event_flag_set(app->event, EthNetEventDataReceived); // Set an event flag if you want to signal the main loop
    }
}

int32_t new_app(void* context)
{
    NewApp* app = app_alloc();

    // Example: Log a message
    RHS_LOG_I(TAG, "Hello from new_app!");

    // Start a TCP listener on port 2345 with the event handler and pass the application context
    eth_net_start_listener(app->net, "tcp://0.0.0.0:2345", _ev_handler, app);

    for (;;)
    {
        uint32_t flags = rhs_event_flag_wait(app->event, EthNetEventDataReceived, RHSFlagWaitAny, RHSWaitForever);
        if (flags & EthNetEventDataReceived)
        {
            // Handle the event (e.g., process received data)
            RHS_LOG_I(TAG, "Data received event handled in main loop");
        }
        // Your application logic here
    }
}
```
