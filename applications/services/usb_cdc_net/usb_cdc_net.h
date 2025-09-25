#include "stdint.h"

#define RECORD_CDC_NET "cdc_net"

typedef struct CdcNet CdcNet;

CdcNet* usb_cdc_net_enable(void);

void usb_cdc_net_disable(CdcNet* cdc_net);
