#include "net.h"
#include "net_i.h"
#include "net_listeners.h"

#define TAG "Net"

static void net_cli_command(char* args, void* context)
{
    Net* net = (Net*) context;
    if (args == NULL)
    {
        struct mg_tcpip_if* ifp = net->mgr->ifp;
        // clang-format off
        printf("Status:\n");
        printf("  Link:    %s\n", (ifp->state >= MG_TCPIP_STATE_UP) ? "UP" : "DOWN");
        printf("  IP:      %u.%u.%u.%u\n", ifp->ip & 0xFF, (ifp->ip >> 8) & 0xFF, (ifp->ip >> 16) & 0xFF, (ifp->ip >> 24) & 0xFF);
        printf("  Mask:    %u.%u.%u.%u\n", ifp->mask & 0xFF, (ifp->mask >> 8) & 0xFF, (ifp->mask >> 16) & 0xFF, (ifp->mask >> 24) & 0xFF);
        printf("  Gateway: %u.%u.%u.%u\n", ifp->gw & 0xFF, (ifp->gw >> 8) & 0xFF, (ifp->gw >> 16) & 0xFF, (ifp->gw >> 24) & 0xFF);
        printf("  MAC:     %02X:%02X:%02X:%02X:%02X:%02X\n", ifp->mac[0], ifp->mac[1], ifp->mac[2], ifp->mac[3], ifp->mac[4], ifp->mac[5]);
        // clang-format on
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
                ip_to_string(a, b, c, d, net->config->ip);

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
        RHS_LOG_I(TAG, "mDNS on http://%s.local started", name);
    }
    else
    {
        RHS_LOG_E(TAG, "Failed to start mDNS");
    }
}

static void net_mdns_stop(Net* net)
{
    const char* name = rhs_thread_get_name(rhs_thread_get_id(net->thread));
    cli_remove_command(net->cli, name);
}

void net_start_http(Net* net, const char* uri, mg_event_handler_t fn, void* context)
{
    rhs_assert(net);
    NetApiEventMessage msg = {.lock = api_lock_alloc_locked(),
                              .type = NetApiEventTypeSetHttp,
                              .data = {.interface = {.uri = (char*) uri, .fn = fn, .context = context}}};
    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);
    api_lock_wait_unlock_and_free(msg.lock);
}

void net_start_listener(Net* net, const char* uri, mg_event_handler_t fn, void* context)
{
    rhs_assert(net);
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
    rhs_assert(net);
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

void net_set_config(Net* net, const NetConfig* config)
{
    rhs_assert(net);
    RHSThread* thread = rhs_thread_get_current();
    RHSApiLock lock   = NULL;

    if (thread != net->thread)
    {
        lock = api_lock_alloc_locked();
    }

    NetApiEventMessage msg = {.lock = lock, .type = NetApiEventTypeRestart};
    memcpy(&msg.data.config, config, sizeof(NetConfig));
    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);

    if (thread != net->thread)
    {
        api_lock_wait_unlock_and_free(msg.lock);
    }
}

void net_get_config(Net* net, NetConfig* config)
{
    rhs_assert(net && config);

    // Copy current configuration to the provided config structure
    uint8_t* ip_b = (uint8_t*) &net->mgr->ifp->ip;
    uint8_t* mk_b = (uint8_t*) &net->mgr->ifp->mask;
    uint8_t* gw_b = (uint8_t*) &net->mgr->ifp->gw;
    ip_to_string(ip_b[0], ip_b[1], ip_b[2], ip_b[3], config->ip);
    ip_to_string(mk_b[0], mk_b[1], mk_b[2], mk_b[3], config->mask);
    ip_to_string(gw_b[0], gw_b[1], gw_b[2], gw_b[3], config->gateway);
}

void net_stop(Net* net)
{
    rhs_assert(net);
    NetApiEventMessage msg = {.type = NetApiEventTypeStop};
    rhs_message_queue_put(net->queue, &msg, RHSWaitForever);
}

int32_t net_worker(void* context)
{
    Net* net = (Net*) context;
    net->cli = rhs_record_open(RECORD_CLI);

    net_mdns_start(net);

    NetApiEventMessage msg;
    RHS_LOG_I(TAG, "Starting event loop");

    for (;;)
    {
        mg_mgr_poll(net->mgr, 0);  // Infinite event loop

        if (rhs_message_queue_get(net->queue, &msg, 0) == RHSStatusOk)
        {
            if (msg.type == NetApiEventTypeSetHttp)
            {
                mg_http_listen(net->mgr, msg.data.interface.uri, msg.data.interface.fn, msg.data.interface.context);
                RHS_LOG_I(TAG, "HTTP server started on %s", msg.data.interface.uri);

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
                RHS_LOG_I(TAG, "TCP server started on %s", msg.data.interface.uri);

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
                            RHS_LOG_I(TAG, "ip %s, port %d", c->loc.addr.ip, mg_htons(c->loc.port));
                            c->is_closing = 1;
                        }
                    }

                    // Process the closing - this will actually close the sockets
                    mg_mgr_poll(net->mgr, 0);

                    // Remove listener from the stored list
                    if (net_listeners_remove(&net->listeners, msg.data.interface.uri) != true)
                    {
                        RHS_LOG_E(TAG, "Listener not found in stored list: %s", msg.data.interface.uri);
                    }
                }
                else
                {
                    RHS_LOG_E(TAG, "invalid listening URL: %s", msg.data.interface.uri);
                }
            }
            else if (msg.type == NetApiEventTypeRestart)
            {
                struct mg_connection* c;
                unsigned int          pa, pb, pc, pd;
                if (string_to_ip(msg.data.config.ip, &pa, &pb, &pc, &pd) == 0)
                    net->mgr->ifp->ip = MG_IPV4(pa, pb, pc, pd);
                if (string_to_ip(msg.data.config.mask, &pa, &pb, &pc, &pd) == 0)
                    net->mgr->ifp->mask = MG_IPV4(pa, pb, pc, pd);
                if (string_to_ip(msg.data.config.gateway, &pa, &pb, &pc, &pd) == 0)
                    net->mgr->ifp->gw = MG_IPV4(pa, pb, pc, pd);

                // Close all existing connections
                for (c = net->mgr->conns; c != NULL; c = c->next)
                    c->is_closing = 1;
                mg_mgr_poll(net->mgr, 0);

                // Restore all registered listeners
                RHS_LOG_I(TAG, "Restoring listeners...");
                for (NetListener* listener = net->listeners; listener != NULL; listener = listener->next)
                {
                    if (listener->type == NetListenerTypeHttp)
                    {
                        mg_http_listen(net->mgr, listener->uri, listener->fn, listener->context);
                        RHS_LOG_I(TAG, "Restored HTTP server on %s", listener->uri);
                    }
                    else if (listener->type == NetListenerTypeTcp)
                    {
                        mg_listen(net->mgr, listener->uri, listener->fn, listener->context);
                        RHS_LOG_I(TAG, "Restored TCP server on %s", listener->uri);
                    }
                }
            }
            else if (msg.type == NetApiEventTypeStop)
            {
                RHS_LOG_W(TAG, "%s Stopping network manager...", rhs_thread_get_name(rhs_thread_get_id(net->thread)));
                break;
            }

            if (msg.lock)
                api_lock_unlock(msg.lock);
        }
    }

    for (NetListener* listener = net->listeners; listener != NULL; listener = listener->next)
    {
        net_listeners_remove(&net->listeners, listener->uri);
    }
    net_mdns_stop(net);
    rhs_record_close(RECORD_CLI);
}
