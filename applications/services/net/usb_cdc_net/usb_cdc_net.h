#include "stdint.h"
#include "net_utils.h"

#define RECORD_CDC_NET "cdc_net"

typedef struct CdcNet CdcNet;

CdcNet* usb_cdc_net_enable(void);

