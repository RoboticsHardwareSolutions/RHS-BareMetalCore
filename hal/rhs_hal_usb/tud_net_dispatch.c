#include "tud_net_dispatch.h"
#include "stddef.h"

/* Defined here — single definition across the whole binary.
 * Both usb_cdc_net and usb_eth_bridge write to this before tusb_init(). */
uint8_t tud_network_mac_address[6] = {2, 2, 0x84, 0x6A, 0x96, 0};

static const TudNetOps* s_ops = NULL;

void tud_net_dispatch_set(const TudNetOps* ops)
{
    s_ops = ops;
}

void tud_net_dispatch_clear(void)
{
    s_ops = NULL;
}

/* -------------------------------------------------------------------------
 * TinyUSB network callbacks — single definitions for the whole binary.
 * ------------------------------------------------------------------------- */

bool tud_network_recv_cb(const uint8_t* buf, uint16_t len)
{
    if(s_ops && s_ops->recv)
        return s_ops->recv(buf, len);
    return false;
}

void tud_network_init_cb(void)
{
    if(s_ops && s_ops->init)
        s_ops->init();
}

uint16_t tud_network_xmit_cb(uint8_t* dst, void* ref, uint16_t arg)
{
    if(s_ops && s_ops->xmit)
        return s_ops->xmit(dst, ref, arg);
    return 0;
}
