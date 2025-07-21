#include "stdlib.h"
#include <string.h>
#include <assert.h>
#include "rhs_hal_network.h"
#include "lwip/netif.h"

extern struct netif gnetif;  // Net struct

static char s_ip[32];
static char s_mask[32];
static char s_gw[32];

uint32_t ipv4_str_to_u32(const char* str_ip)
{
    uint32_t ip         = 0;
    char     part_ip[3] = {0};
    int      i_ip       = 0;
    int      i_part_ip  = 0;
    for (int i = 0; i < strlen(str_ip) + 1; i++)
    {
        if (str_ip[i] == '.' || str_ip[i] == '\0')
        {
            ip |= (atoi(part_ip) << (8 * i_part_ip++));
            memset(part_ip, 0, 3);
            i_ip = 0;
        }
        else
            part_ip[i_ip++] = str_ip[i];
    }
    return ip;
}

void rhs_hal_network_init(void) {}

void rhs_hal_network_get_settings(RHSNetSettingsType* settings)
{
    assert(settings);
    const ip4_addr_t* ip   = netif_ip4_addr(&gnetif);
    const ip4_addr_t* mask = netif_ip4_netmask(&gnetif);
    const ip4_addr_t* gw   = netif_ip4_gw(&gnetif);

    settings->ip_v4   = s_ip;
    settings->mask_v4 = s_mask;
    settings->gw_v4   = s_gw;

    sprintf(settings->ip_v4,
            "%lu.%lu.%lu.%lu",
            (ip->addr >> 0) & 0xFF,
            (ip->addr >> 8) & 0xFF,
            (ip->addr >> 16) & 0xFF,
            (ip->addr >> 24) & 0xFF);
    sprintf(settings->mask_v4,
            "%lu.%lu.%lu.%lu",
            (mask->addr >> 0) & 0xFF,
            (mask->addr >> 8) & 0xFF,
            (mask->addr >> 16) & 0xFF,
            (mask->addr >> 24) & 0xFF);
    sprintf(settings->gw_v4,
            "%lu.%lu.%lu.%lu",
            (gw->addr >> 0) & 0xFF,
            (gw->addr >> 8) & 0xFF,
            (gw->addr >> 16) & 0xFF,
            (gw->addr >> 24) & 0xFF);
}

int rhs_hal_network_set_settings(RHSNetSettingsType* settings)  // TODO check if ip is correct
{
    ip4_addr_t* ip   = &gnetif.ip_addr;
    ip4_addr_t* mask = &gnetif.netmask;
    ip4_addr_t* gw   = &gnetif.gw;
    ip->addr         = ipv4_str_to_u32(settings->ip_v4);
    mask->addr       = ipv4_str_to_u32(settings->mask_v4);
    gw->addr         = ipv4_str_to_u32(settings->gw_v4);

    netif_set_ipaddr(&gnetif, ip);
    netif_set_netmask(&gnetif, mask);
    netif_set_gw(&gnetif, gw);
}
