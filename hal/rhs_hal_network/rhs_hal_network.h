#pragma once
#include "stdint.h"

typedef struct
{
    char* ip_v4;
    char* mask_v4;
    char* gw_v4;
    uint32_t port;
} RHSNetSettingsType;

void rhs_hal_network_init(void);

void rhs_hal_network_get_settings(RHSNetSettingsType* settings);

int rhs_hal_network_set_settings(RHSNetSettingsType* settings);
