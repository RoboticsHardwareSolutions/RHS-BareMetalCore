#pragma once

#include "mongoose.h"
#include "net_utils.h"
#include "net.h"

Net* eth_net_start(void);

void eth_net_stop(Net* net);
