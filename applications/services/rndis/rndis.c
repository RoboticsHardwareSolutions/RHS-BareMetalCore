#include "rhs.h"
#include "rhs_hal.h"
#include "rndis.h"
#include "rndis_i.h"
#include "mongoose.h"
#include "tusb.h"
#include "hal.h"

#define TAG "rndis"
static struct mg_tcpip_if* s_ifp;
uint8_t                    tud_network_mac_address[6] = {2, 2, 0x84, 0x6A, 0x96, 0};

static Rndis* rndis_alloc(void)
{
    Rndis* rndis = malloc(sizeof(Rndis));
    return rndis;
}

bool tud_network_recv_cb(const uint8_t* buf, uint16_t len)
{
    mg_tcpip_qwrite((void*) buf, len, s_ifp);
    // MG_INFO(("RECV %hu", len));
    // mg_hexdump(buf, len);
    tud_network_recv_renew();
    return true;
}

void tud_network_init_cb(void) {}

uint16_t tud_network_xmit_cb(uint8_t* dst, void* ref, uint16_t arg)
{
    // MG_INFO(("SEND %hu", arg));
    memcpy(dst, ref, arg);
    return arg;
}

static size_t usb_tx(const void* buf, size_t len, struct mg_tcpip_if* ifp)
{
    if (!tud_ready())
        return 0;
    while (!tud_network_can_xmit(len))
        tud_task();
    tud_network_xmit((void*) buf, len);
    (void) ifp;
    return len;
}

static inline void usb_init()
{
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);
    rhs_delay_ms(40);  // Wait 4ms
    gpio_init(PIN('A', 11), GPIO_MODE_INPUT_FLOATING);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_INPUT_FLOATING);  // D+
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;                  // Enable USB clock

    NVIC_SetPriority(USB_LP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_SetPriority(USB_HP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(USB_LP_IRQn);
    NVIC_EnableIRQ(USB_HP_IRQn);
    // Note: STM32F103 doesn't have USB_OTG, it has USB FS device only
    // USB peripheral will be configured by TinyUSB
}

static bool usb_poll(struct mg_tcpip_if* ifp, bool s1)
{
    (void) ifp;
    tud_task();
    return s1 ? tud_inited() && tud_ready() && tud_connected() : false;
}

bool mg_random(void* buf, size_t len)
{  // Use on-board RNG
    rhs_hal_random_fill_buf(buf, len);
    return true;
}

static void fn(struct mg_connection* c, int ev, void* ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message* hm = (struct mg_http_message*) ev_data;
        if (mg_match(hm->uri, mg_str("/api/debug"), NULL))
        {
            int level = mg_json_get_long(hm->body, "$.level", MG_LL_DEBUG);
            mg_log_set(level);
            mg_http_reply(c, 200, "", "Debug level set to %d\n", level);
        }
        else
        {
            mg_http_reply(c, 200, "", "hi!!!\n");
        }
    }
}

int32_t rndis_service(void* context)
{
    struct mg_mgr mgr;        // Initialise
    mg_mgr_init(&mgr);        // Mongoose event manager
    mg_log_set(MG_LL_DEBUG);  // Set log level

    const uint8_t* uid = rhs_hal_version_uid();

    MG_INFO(("Init TCP/IP stack ..."));
    struct mg_tcpip_driver driver = {.tx = usb_tx, .poll = usb_poll};
    struct mg_tcpip_if     mif    = {.mac                = GENERATE_LOCALLY_ADMINISTERED_MAC(uid),
                                     .ip                 = mg_htonl(MG_U32(192, 168, 3, 1)),
                                     .mask               = mg_htonl(MG_U32(255, 255, 255, 0)),
                                     .enable_dhcp_server = true,
                                     .driver             = &driver,
                                     .recv_queue.size    = 4096};
    s_ifp                         = &mif;
    mg_tcpip_init(&mgr, &mif);
    mg_http_listen(&mgr, "tcp://0.0.0.0:80", fn, &mgr);

    MG_INFO(("Init USB ..."));
    usb_init();
    tusb_init();

    MG_INFO(("Init done, starting main loop ..."));
    for (;;)
    {
        mg_mgr_poll(&mgr, 0);
    }
}
