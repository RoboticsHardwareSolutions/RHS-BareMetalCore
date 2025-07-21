#pragma once
#include "stdint.h"

typedef struct
{
    char* ip_v4;
    char* mask_v4;
    char* gw_v4;
    uint32_t port;
} RHSNetSettingsType;

uint32_t ipv4_str_to_u32(const char* str_ip);

void rhs_hal_network_init(void);

void rhs_hal_network_get_settings(RHSNetSettingsType* settings);

int rhs_hal_network_set_settings(RHSNetSettingsType* settings);
