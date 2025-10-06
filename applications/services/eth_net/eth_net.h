#pragma once

#include "mongoose.h"

#define RECORD_ETH_NET "eth_net"

typedef struct EthNet EthNet;

typedef struct
{
    uint32_t ip;        // IP address (e.g., MG_IPV4(192, 168, 1, 100))
    uint32_t mask;      // Network mask (e.g., MG_IPV4(255, 255, 255, 0))
    uint32_t gateway;   // Gateway address (e.g., MG_IPV4(192, 168, 1, 1))
    uint8_t  mac[6];    // MAC address
    uint8_t  phy_addr;  // PHY address
    uint8_t  mdc_cr;    // MDC clock divider
} EthNetConfig;

/* Example of user-defined function to set custom network configuration on startup. (For example from external flash)
 * void eth_net_set_config_on_startup(EthNetConfig* config)
 * {
 *     // Default weak implementation - does nothing
 *     // User can override this function to set custom network configuration
 *     // Example:
 *     config->ip = MG_IPV4(192, 168, 1, 100);
 *     config->mask = MG_IPV4(255, 255, 255, 0);
 *     config->gateway = MG_IPV4(192, 168, 1, 1);
 *     config->mac[0] = 0x02; config->mac[1] = 0x00; config->mac[2] = 0x00;
 *     config->mac[3] = 0x12; config->mac[4] = 0x34; config->mac[5] = 0x56;
 *     config->phy_addr = 0;
 *     config->mdc_cr = 24;
 * }
 */

__attribute__((weak)) void eth_net_set_config_on_startup(EthNetConfig* config);

void eth_net_start_http(EthNet* eth_net, const char* uri, mg_event_handler_t fn, void* context);

void eth_net_start_listener(EthNet* eth_net, const char* uri, mg_event_handler_t fn, void* context);

void eth_net_set_config(EthNet* eth_net, EthNetConfig* config);

void eth_net_get_config(EthNet* eth_net, EthNetConfig* config);

/**
 * @brief Parse IP address string into four octets
 * @param ip_str Input string in format "192.168.1.100"
 * @param a Pointer to first octet
 * @param b Pointer to second octet
 * @param c Pointer to third octet
 * @param d Pointer to fourth octet
 * @return 0 on success, -1 on error
 */
int parse_ip_address(const char* ip_str, unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);
