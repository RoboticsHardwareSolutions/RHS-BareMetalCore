#include "rhs.h"
#include "rhs_hal.h"
#include "rndis.h"
#include "rndis_i.h"
#include "mongoose.h"
#include "tusb.h"
#include "hal.h"

extern int32_t usb_dual_cdc(void* context);

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
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    // GPIOA->ODR &= ~(GPIO_PIN_12 | GPIO_PIN_11);

    rhs_delay_ms(40);  // Wait 4ms
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USBRST;

    gpio_init(PIN('A', 11), GPIO_MODE_INPUT_FLOATING);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_INPUT_FLOATING);  // D+
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;                  // Enable USB clock

    NVIC_SetPriority(USB_LP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_SetPriority(USB_HP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    // NVIC_EnableIRQ(USB_LP_IRQn);
    // NVIC_EnableIRQ(USB_HP_IRQn);
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

static bool finish = false;

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
            finish = true;
        }
        else if (mg_match(hm->uri, mg_str("/api/switch-to-vcp"), NULL))
        {
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                         "{\"status\":\"ok\",\"message\":\"Switching to VCP mode. Web interface will be unavailable.\"}");
            MG_INFO(("Switching to VCP mode - web interface will be unavailable"));
            finish = true;
        }
        else if (mg_match(hm->uri, mg_str("/"), NULL))
        {
            const char* html = 
                "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "    <title>RNDIS Control</title>"
                "    <meta charset='UTF-8'>"
                "    <style>"
                "        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }"
                "        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
                "        h1 { color: #333; text-align: center; }"
                "        .switch-container { margin: 30px 0; text-align: center; }"
                "        .switch { position: relative; display: inline-block; width: 60px; height: 34px; }"
                "        .switch input { opacity: 0; width: 0; height: 0; }"
                "        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }"
                "        .slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }"
                "        input:checked + .slider { background-color: #2196F3; }"
                "        input:checked + .slider:before { transform: translateX(26px); }"
                "        .label { margin: 10px 0; font-size: 18px; font-weight: bold; }"
                "        .warning { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; padding: 15px; border-radius: 5px; margin: 20px 0; display: none; }"
                "    </style>"
                "</head>"
                "<body>"
                "    <div class='container'>"
                "        <h1>RNDIS Control Panel</h1>"
                "        <div class='switch-container'>"
                "            <div class='label'>Switch to VCP Mode</div>"
                "            <label class='switch'>"
                "                <input type='checkbox' id='vcpSwitch' onchange='toggleWarning(this)'>"
                "                <span class='slider'></span>"
                "            </label>"
                "        </div>"
                "        <div id='warning-text' class='warning'>"
                "            <strong>Warning!</strong> Switching to VCP mode will make the web interface unavailable. Are you sure you want to continue?<br><br>"
                "            <button onclick='confirmSwitch()'>Yes, Switch to VCP</button> "
                "            <button onclick='cancelSwitch()'>Cancel</button>"
                "        </div>"
                "    </div>"
                "    <script>"
                "        function toggleWarning(checkbox) {"
                "            var warning = document.getElementById('warning-text');"
                "            warning.style.display = checkbox.checked ? 'block' : 'none';"
                "        }"
                "        function confirmSwitch() {"
                "            fetch('/api/switch-to-vcp', { method: 'POST' })"
                "                .then(function(response) { return response.json(); })"
                "                .then(function(data) { alert('Switching to VCP mode...'); })"
                "                .catch(function(error) { alert('Error switching to VCP mode'); });"
                "        }"
                "        function cancelSwitch() {"
                "            document.getElementById('vcpSwitch').checked = false;"
                "            document.getElementById('warning-text').style.display = 'none';"
                "        }"
                "    </script>"
                "</body>"
                "</html>";
            mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", html);
        }
        else
        {
            mg_http_reply(c, 404, "", "Not found\n");
        }
    }
}

int32_t rndis_service(void* context)
{
    struct mg_mgr mgr;        // Initialise
    // mg_mgr_init(&mgr);        // Mongoose event manager
    // mg_log_set(MG_LL_DEBUG);  // Set log level

    // const uint8_t* uid = rhs_hal_version_uid();

    // MG_INFO(("Init TCP/IP stack ..."));
    // struct mg_tcpip_driver driver = {.tx = usb_tx, .poll = usb_poll};
    // struct mg_tcpip_if     mif    = {.mac                = GENERATE_LOCALLY_ADMINISTERED_MAC(uid),
    //                                  .ip                 = mg_htonl(MG_U32(192, 168, 3, 1)),
    //                                  .mask               = mg_htonl(MG_U32(255, 255, 255, 0)),
    //                                  .enable_dhcp_server = true,
    //                                  .driver             = &driver,
    //                                  .recv_queue.size    = 4096};
    // s_ifp                         = &mif;
    // mg_tcpip_init(&mgr, &mif);
    // mg_http_listen(&mgr, "tcp://0.0.0.0:80", fn, &mgr);

    // MG_INFO(("Init USB ..."));
    // usb_init();
    // tusb_init();

    // MG_INFO(("Init done, starting main loop ..."));
    // while (!finish)
    // {
    //     mg_mgr_poll(&mgr, 0);
    // }

    // MG_INFO(("Finish ..."));
    // mg_mgr_free(&mgr);
    // tusb_deinit(0);

    // RHSThread* thread = rhs_thread_alloc_service("bridge", 4096, usb_dual_cdc, NULL);
    // rhs_thread_start(thread);

    for (;;)
    {
        rhs_delay_ms(1000);
    }
}
