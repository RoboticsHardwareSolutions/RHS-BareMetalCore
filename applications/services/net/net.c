#include "net.h"
#include "net_i.h"
#include "net_listeners.h"

static void net_cli_command(char* args, void* context)
{
    Net* net = (Net*) context;
    if (args == NULL)
    {
        struct mg_tcpip_if* ifp = net->mgr->ifp;
        printf("Status:\n");
        printf("  Link:    %s\n", (ifp->state >= MG_TCPIP_STATE_UP) ? "UP" : "DOWN");
        printf("  IP:      %u.%u.%u.%u\n",
               ifp->ip & 0xFF,
               (ifp->ip >> 8) & 0xFF,
               (ifp->ip >> 16) & 0xFF,
               (ifp->ip >> 24) & 0xFF);
        printf("  Mask:    %u.%u.%u.%u\n",
               ifp->mask & 0xFF,
               (ifp->mask >> 8) & 0xFF,
               (ifp->mask >> 16) & 0xFF,
               (ifp->mask >> 24) & 0xFF);
        printf("  Gateway: %u.%u.%u.%u\n",
               ifp->gw & 0xFF,
               (ifp->gw >> 8) & 0xFF,
               (ifp->gw >> 16) & 0xFF,
               (ifp->gw >> 24) & 0xFF);
        printf("  MAC:     %02X:%02X:%02X:%02X:%02X:%02X\n",
               ifp->mac[0],
               ifp->mac[1],
               ifp->mac[2],
               ifp->mac[3],
               ifp->mac[4],
               ifp->mac[5]);
        return;
    }
    else
    {
        char* separator = strchr(args, ' ');
        if (separator == NULL || *(separator + 1) == 0)
        {
            if (strstr(args, "--restart") == args)
            {
                // Restart network manager
                net_set_config(net, net->config);
                return;
            }
            printf("Invalid argument\n");
            return;
        }
        else if (strstr(args, "--ip") == args)
        {
            // Parse IP address from string like "192.168.1.100"
            char*        ip_str = separator + 1;
            unsigned int a, b, c, d;

            if (string_to_ip(ip_str, &a, &b, &c, &d) == 0)
            {
                // Update IP address in config
                net->config->ip = MG_IPV4(a, b, c, d);

                printf("IP address will be changed to %u.%u.%u.%u\n", a, b, c, d);
                printf("Restarting network manager...\n");

                // Restart network manager to apply new IP
                net_set_config(net, net->config);
                return;
            }

            printf("Invalid IP address format. Expected: eth --ip 192.168.1.100\n");
            return;
        }
        printf("Invalid argument\n");
    }
}

static void net_mdns_start(Net* net)
{
    const char* name = rhs_thread_get_name(rhs_thread_get_id(net->thread));
    cli_add_command(net->cli, name, net_cli_command, net);

    // Example of mDNS
    struct mg_connection* c = mg_mdns_listen(net->mgr, NULL, (char*) name);
    if (c != NULL)
    {
        uint32_t response_ip = net->mgr->ifp->ip;
        memcpy(c->data, &response_ip, sizeof(response_ip));
        MG_INFO(("mDNS on http://%s.local started", name));
    }
}

void net_start_http(Net* net, const char* uri, mg_event_handler_t fn, void* context)
{
    NetApiEventMessage msg = {.lock = api_lock_alloc_locked(),
                              .type = NetApiEventTypeSetHttp,
                              .data = {.interface = {.uri = (char*) uri, .fn = fn, .context = context}}};
    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);
    api_lock_wait_unlock_and_free(msg.lock);
}

void net_start_listener(Net* net, const char* uri, mg_event_handler_t fn, void* context)
{
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    NetApiEventMessage msg = {.lock = lock,
                              .type = NetApiEventTypeSetTcp,
                              .data = {.interface = {.uri = (char*) uri, .fn = fn, .context = context}}};

    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);

    if (thread != net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void net_stop_listener(Net* net, const char* uri)
{
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    NetApiEventMessage msg = {.lock = lock, .type = NetApiEventTypeRstTcp, .data = {.interface = {.uri = (char*) uri}}};

    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);

    if (thread != net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void net_set_config(Net* net, NetConfig* config)
{
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    NetApiEventMessage msg = {.lock = lock, .type = NetApiEventTypeRestart, .data = {.config = *config}};
    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);

    if (thread != net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void net_get_config(Net* net, NetConfig* config)
{
    if (net == NULL || config == NULL)
        return;

    // Copy current configuration to the provided config structure
    config->ip      = net->mgr->ifp->ip;
    config->mask    = net->mgr->ifp->mask;
    config->gateway = net->mgr->ifp->gw;
}

int32_t net_worker(void* context)
{
    Net* net       = (Net*) context;
    net->cli       = rhs_record_open(RECORD_CLI);

    net_mdns_start(net);

    NetApiEventMessage msg;
    MG_INFO(("Starting event loop"));

    for (;;)
    {
        mg_mgr_poll(net->mgr, 0);  // Infinite event loop

        if (rhs_message_queue_get(net->queue, &msg, 0) == RHSStatusOk)
        {
            if (msg.type == NetApiEventTypeSetHttp)
            {
                mg_http_listen(net->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                MG_INFO(("HTTP server started on %s", msg.data.interface.uri));

                // Add listener to the buffer for later restoration
                net_listeners_add(&net->listeners,
                                  NetListenerTypeHttp,
                                  msg.data.interface.uri,
                                  msg.data.interface.fn,
                                  msg.data.interface.context);
            }
            else if (msg.type == NetApiEventTypeSetTcp)
            {
                mg_listen(net->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                MG_INFO(("TCP server started on %s", msg.data.interface.uri));

                // Add listener to the buffer for later restoration
                net_listeners_add(&net->listeners,
                                  NetListenerTypeTcp,
                                  msg.data.interface.uri,
                                  msg.data.interface.fn,
                                  msg.data.interface.context);
            }
            else if (msg.type == NetApiEventTypeRstTcp)
            {
                struct mg_connection* c;
                struct mg_addr        target_addr;

                uint16_t port = mg_url_port(msg.data.interface.uri);

                // Parse the target URL to get address
                if (mg_aton(mg_url_host(msg.data.interface.uri), &target_addr))
                {
                    // Close all connections matching this listener address
                    for (c = net->mgr->conns; c != NULL; c = c->next)
                    {
                        // Check if this is a listening connection with matching address
                        if (c->is_listening && c->loc.port == mg_htons(port))
                        {
                            MG_INFO(("ip %s, port %d", c->loc.addr.ip, mg_htons(c->loc.port)));
                            c->is_closing = 1;
                        }
                    }

                    // Process the closing - this will actually close the sockets
                    mg_mgr_poll(net->mgr, 0);

                    // Remove listener from the stored list
                    if (net_listeners_remove(&net->listeners, msg.data.interface.uri) != true)
                    {
                        MG_ERROR(("Listener not found in stored list: %s", msg.data.interface.uri));
                    }
                }
                else
                {
                    MG_ERROR(("invalid listening URL: %s", msg.data.interface.uri));
                }
            }
            else if (msg.type == NetApiEventTypeRestart)
            {
                struct mg_connection* c;
                net->mgr->ifp->ip   = msg.data.config.ip;
                net->mgr->ifp->mask = msg.data.config.mask;
                net->mgr->ifp->gw   = msg.data.config.gateway;

                // Close all existing connections
                for (c = net->mgr->conns; c != NULL; c = c->next)
                    c->is_closing = 1;
                mg_mgr_poll(net->mgr, 0);

                // Restore all registered listeners
                MG_INFO(("Restoring listeners..."));
                for (NetListener* listener = net->listeners; listener != NULL; listener = listener->next)
                {
                    if (listener->type == NetListenerTypeHttp)
                    {
                        mg_http_listen(net->mgr, listener->uri, listener->fn, listener->context);
                        MG_INFO(("Restored HTTP server on %s", listener->uri));
                    }
                    else if (listener->type == NetListenerTypeTcp)
                    {
                        mg_listen(net->mgr, listener->uri, listener->fn, listener->context);
                        MG_INFO(("Restored TCP server on %s", listener->uri));
                    }
                }
            }

            if (msg.lock)
                api_lock_unlock(msg.lock);
        }
    }
    rhs_record_close(RECORD_CLI);
}

__attribute__((weak)) bool mg_random(void* buf, size_t len)
{  // Use on-board RNG
    rhs_hal_random_fill_buf(buf, len);
    return true;
}
