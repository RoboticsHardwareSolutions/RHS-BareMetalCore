#include "modbus_tcp.h"
#include <string.h>
#include "rhs.h"

static void handle_modbus_pdu(struct mg_connection* c, uint8_t* buf, size_t len, void* context)
{
    if (context == NULL)
    {
        MG_ERROR(("There isn't modbus context"));
        return;
    }

    ModbusTcpApi* api = (ModbusTcpApi*) context;

    MG_DEBUG(("Received PDU %p len %lu, hexdump:", buf, len));
    mg_hexdump(buf, len);
    // size_t hdr_size = 8, max_data_size = sizeof(response) - hdr_size;
    if (len < 12)
    {
        MG_ERROR(("PDU too small"));
        return;
    }

    uint8_t func         = buf[7];  // Function
    bool    success      = false;
    size_t  response_len = 0;
    uint8_t response[260];
    memcpy(response, buf, 8);
    // uint16_t tid = mg_ntohs(*(uint16_t *) &buf[0]);  // Transaction ID
    // uint16_t pid = mg_ntohs(*(uint16_t *) &buf[0]);  // Protocol ID
    // uint16_t len = mg_ntohs(*(uint16_t *) &buf[4]);  // PDU length
    // uint8_t uid = buf[6];                            // Unit identifier
    if (func == 2)
    {  // Read Discrete Inputs
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t num   = mg_ntohs(*(uint16_t*) &buf[10]);

        if (api->discrete_inputs_data && start < api->discrete_inputs_size &&
            (start + num) <= api->discrete_inputs_size)
        {
            uint8_t bytes_count = (uint8_t) ((num + 7) / 8);  // Calculate number of bytes needed
            if ((size_t) (bytes_count + 9) < sizeof(response))
            {
                uint8_t* input_data    = (uint8_t*) api->discrete_inputs_data;
                uint8_t* response_data = &response[9];

                // Call callback if provided to allow filling data before read
                if (api->discrete_inputs_cb)
                {
                    api->discrete_inputs_cb(start, input_data, num, api->discrete_inputs_context);
                }

                // Clear response data bytes
                memset(response_data, 0, bytes_count);

                // Pack discrete inputs into response bytes
                for (uint16_t i = 0; i < num; i++)
                {
                    uint16_t input_addr = start + i;
                    uint8_t  byte_index = i / 8;
                    uint8_t  bit_index  = i % 8;
                    uint8_t  bit        = input_data[input_addr] ? 1 : 0;

                    response_data[byte_index] |= (bit << bit_index);
                }

                response[8]  = bytes_count;
                response_len = 9 + bytes_count;
                success      = true;
                MG_DEBUG(("Read discrete inputs: start=%u, num=%u, bytes=%u", start, num, bytes_count));
            }
            else
            {
                MG_ERROR(("Response too large for discrete inputs: bytes=%u", bytes_count));
                success = false;
            }
        }
        else
        {
            MG_ERROR(
                ("Invalid discrete inputs request: start=%u, num=%u, size=%u", start, num, api->discrete_inputs_size));
            success = false;
        }
    }
    else if (func == 1)
    {  // Read Coils
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t num   = mg_ntohs(*(uint16_t*) &buf[10]);

        if (api->coils_data && start < api->coils_size && (start + num) <= api->coils_size)
        {
            uint8_t bytes_count = (uint8_t) ((num + 7) / 8);  // Calculate number of bytes needed
            if ((size_t) (bytes_count + 9) < sizeof(response))
            {
                uint8_t* coils    = (uint8_t*) api->coils_data;
                uint8_t* response_data = &response[9];

                if(api->coils_read_cb)
                {
                    api->coils_read_cb(start, &coils[start], num, api->coils_context);
                }
                // Clear response data bytes
                memset(response_data, 0, bytes_count);

                // Pack coils into response bytes
                for (uint16_t i = 0; i < num; i++)
                {
                    uint16_t coil_addr  = start + i;
                    uint8_t  byte_index = i / 8;
                    uint8_t  bit_index  = i % 8;
                    uint8_t  bit        = coils[coil_addr] ? 1 : 0;

                    response_data[byte_index] |= (bit << bit_index);
                }
                
                response[8]  = bytes_count;
                response_len = 9 + bytes_count;
                success      = true;
                MG_DEBUG(("Read coils: start=%u, num=%u, bytes=%u", start, num, bytes_count));
            }
            else
            {
                MG_ERROR(("Response too large for coils: bytes=%u", bytes_count));
                success = false;
            }
        }
        else
        {
            MG_ERROR(("Invalid coils request: start=%u, num=%u, size=%u", start, num, api->coils_size));
            success = false;
        }
    }
    else if (func == 5)
    {  // Write Single Coil
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t value = mg_ntohs(*(uint16_t*) &buf[10]);

        if (api->coils_data && start < api->coils_size)
        {
            uint8_t* coils = (uint8_t*) api->coils_data;

            // Modbus coil values: 0x0000 = OFF, 0xFF00 = ON
            if (value == 0x0000 || value == 0xFF00)
            {
                coils[start] = (value == 0xFF00) ? 1 : 0;

                // Call callback if provided to notify about write operation
                if (api->coils_write_cb)
                {
                    api->coils_write_cb(start, &coils[start], 1, api->coils_context);
                }

                // Echo back the request
                *(uint16_t*) &response[8]  = mg_htons(start);
                *(uint16_t*) &response[10] = mg_htons(value);
                response_len               = 12;
                success                    = true;
                MG_DEBUG(("Write single coil: addr=%u, value=0x%04X", start, value));
            }
            else
            {
                MG_ERROR(("Invalid coil value: 0x%04X (expected 0x0000 or 0xFF00)", value));
                success = false;
            }
        }
        else
        {
            MG_ERROR(("Invalid coil address: %u, size=%u", start, api->coils_size));
            success = false;
        }
    }
    else if (func == 6)
    {  // write single holding register
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t value = mg_ntohs(*(uint16_t*) &buf[10]);

        // TODO success = api.callback(start, value);
        *(uint16_t*) &response[8]  = mg_htons(start);
        *(uint16_t*) &response[10] = mg_htons(value);
        response_len               = 12;
        MG_ERROR(("func %d NOT IMPLEMENTED", func));
        // MG_DEBUG(("Glue returned %s", success ? "success" : "failure"));
    }
    else if (func == 15)
    {  // Write Multiple Coils
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t num   = mg_ntohs(*(uint16_t*) &buf[10]);

        if (api->coils_data && start < api->coils_size && (start + num) <= api->coils_size)
        {
            uint8_t byte_count     = buf[12];
            uint8_t expected_bytes = (uint8_t) ((num + 7) / 8);
            if (byte_count == expected_bytes && (size_t) (byte_count + 13) <= len)
            {
                uint8_t* coils = (uint8_t*) api->coils_data;

                // Write coil values from received data
                for (uint16_t i = 0; i < num; i++)
                {
                    const uint8_t* coil_data  = &buf[13];
                    uint16_t       coil_addr  = start + i;
                    uint8_t        byte_index = i / 8;
                    uint8_t        bit_index  = i % 8;

                    // Extract bit value from received data
                    uint8_t bit_value = (coil_data[byte_index] >> bit_index) & 0x01;
                    coils[coil_addr]  = bit_value;
                }

                // Call callback if provided to notify about write operation
                if (api->coils_write_cb)
                {
                    api->coils_write_cb(start, &coils[start], num, api->coils_context);
                }

                // Prepare response
                *(uint16_t*) &response[8]  = mg_htons(start);
                *(uint16_t*) &response[10] = mg_htons(num);
                response_len               = 12;
                success                    = true;
                MG_DEBUG(("Write multiple coils: start=%u, num=%u, bytes=%u", start, num, byte_count));
            }
            else
            {
                MG_ERROR(
                    ("Invalid coil data format: expected_bytes=%u, received_bytes=%u", expected_bytes, byte_count));
                success = false;
            }
        }
        else
        {
            MG_ERROR(("Invalid coils write request: start=%u, num=%u, size=%u", start, num, api->coils_size));
            success = false;
        }
    }
    else if (func == 16)
    {  // Write multiple
        // const uint16_t* data  = (uint16_t*) &buf[13];
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t num   = mg_ntohs(*(uint16_t*) &buf[10]);
        if ((size_t) (num * 2 + 10) < sizeof(response))
        {
            for (uint16_t i = 0; i < num; i++)
            {
                // TODO success = api.callback((uint16_t) (start + i), mg_htons(data[i]));
                if (success == false)
                    break;
            }
            *(uint16_t*) &response[8]  = mg_htons(start);
            *(uint16_t*) &response[10] = mg_htons(num);
            response_len               = 12;
            MG_ERROR(("func %d NOT IMPLEMENTED", func));
            // MG_DEBUG(("Glue returned %s", success ? "success" : "failure"));
        }
    }
    else if (func == 3)
    {  // Read Holding Registers
        uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t num   = mg_ntohs(*(uint16_t*) &buf[10]);

        if (api->holdings_data && start < api->holdings_size && (start + num) <= api->holdings_size)
        {
            if ((size_t) (num * 2 + 9) < sizeof(response))
            {
                uint16_t* holdings_data = (uint16_t*) api->holdings_data;
                uint16_t* response_data = (uint16_t*) &response[9];

                // Read holding registers and pack into response
                for (uint16_t i = 0; i < num; i++)
                {
                    uint16_t reg_addr = start + i;
                    uint16_t val      = holdings_data[reg_addr];
                    response_data[i]  = mg_htons(val);
                }

                // Call callback if provided to notify about read operation
                if (api->holdings_cb)
                {
                    api->holdings_cb(start, &holdings_data[start], num, api->holdings_context);
                }

                response[8]  = (uint8_t) (num * 2);  // Byte count
                response_len = 9 + response[8];
                success      = true;
                MG_DEBUG(("Read holding registers: start=%u, num=%u, bytes=%u", start, num, response[8]));
            }
            else
            {
                MG_ERROR(("Response too large for holding registers: num=%u", num));
                success = false;
            }
        }
        else
        {
            MG_ERROR(("Invalid holding registers request: start=%u, num=%u, size=%u", start, num, api->holdings_size));
            success = false;
        }
    }
    else if (func == 4)
    {  // Read Input Registers
        // uint16_t start = mg_ntohs(*(uint16_t*) &buf[8]);
        uint16_t num = mg_ntohs(*(uint16_t*) &buf[10]);
        if ((size_t) (num * 2 + 9) < sizeof(response))
        {
            uint16_t i, val, *data = (uint16_t*) &response[9];
            for (i = 0; i < num; i++)
            {
                // TODO success = api.callback((uint16_t) (start + i), &val);
                if (success == false)
                    break;
                data[i] = mg_htons(val);
            }
            response[8]  = (uint8_t) (num * 2);
            response_len = 9 + response[8];
            MG_ERROR(("func %d NOT IMPLEMENTED", func));
            // MG_DEBUG(("Glue returned %s", success ? "success" : "failure"));
        }
    }

    // Only send generic Server Device Failure if no specific exception was set
    if (success == false && response_len != 9)
    {
        response_len = 9;
        response[7] |= 0x80;
        response[8] = 4;  // Server Device Failure
    }
    *(uint16_t*) &response[4] = mg_htons((uint16_t) (response_len - 6));
    MG_DEBUG(("Sending PDU response %lu:", response_len));
    mg_hexdump(response, response_len);
    mg_send(c, response, response_len);
}

static void modbus_ev_handler(struct mg_connection* c, int ev, void* ev_data)
{
    // if (ev == MG_EV_OPEN) c->is_hexdumping = 1;
    if (ev == MG_EV_READ)
    {
        uint16_t len;
        if (c->recv.len < 7)
            return;                                    // Less than minimum length, buffer more
        len = mg_ntohs(*(uint16_t*) &c->recv.buf[4]);  // PDU length
        MG_INFO(("Got %lu, expecting %lu", c->recv.len, len + 6));
        if (c->recv.len < len + 6U)
            return;                                              // Partial frame, buffer more
        handle_modbus_pdu(c, c->recv.buf, len + 6, c->fn_data);  // Parse PDU and call user
        mg_iobuf_del(&c->recv, 0, len + 6U);                     // Delete received PDU
    }
    (void) ev_data;
}

void modbus_tcp_start(EthNet* app, ModbusTcpApi* api, uint16_t port)
{
    static uint16_t current_port = 0;
    static char     new_addr[32];

    rhs_assert(api);
    rhs_assert(port > 0);

    if (current_port != 0)
    {
        static char current_addr[32];
        snprintf(current_addr, sizeof(current_addr), "tcp://0.0.0.0:%u", current_port);
        eth_net_stop_listener(app, current_addr);
    }

    snprintf(new_addr, sizeof(new_addr), "tcp://0.0.0.0:%u", port);
    eth_net_start_listener(app, new_addr, modbus_ev_handler, api);

    current_port = port;
}
