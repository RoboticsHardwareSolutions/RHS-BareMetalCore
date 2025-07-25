#include <stdarg.h>
#include "can_open.h"
#include "can_open_srv.h"
#include "cli.h"

extern void can_open_cli(char* args, void* context);

static CanOpenApp* can_open_app_alloc(void)
{
    CanOpenApp* app = malloc(sizeof(CanOpenApp));
    app->srv_event  = rhs_event_flag_alloc();
    app->sdo_event  = rhs_event_flag_alloc();
    app->sdo_mutex  = rhs_mutex_alloc(RHSMutexTypeNormal);
    app->rx_queue   = rhs_message_queue_alloc(32, sizeof(CanOpenAppMessage));
    app->tx_queue   = rhs_message_queue_alloc(32, sizeof(CanOpenAppMessage));
    TimerInit();

    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "can_open", can_open_cli, app);
    rhs_record_close(RECORD_CLI);
    
    extern CanOpenApp *can_open_app; /* co_stack and rtimer use callbacks without context. It makes me use this trash */
    can_open_app = app;

    return app;
}

static void can_open_app_free(CanOpenApp* app)
{
    rhs_event_flag_free(app->srv_event);
    rhs_message_queue_free(app->rx_queue);
    rhs_message_queue_free(app->tx_queue);
    rhs_event_flag_free(app->sdo_event);
    rhs_mutex_free(app->sdo_mutex);
    free(app);
}

int32_t can_open_service(void* context)
{
    CanOpenApp*        app  = can_open_app_alloc();
    uint32_t           flag = 0;
    CanOpenAppMessage  msg;
    RHSHalCANFrameType frame = {0};

    rhs_record_create(RECORD_CAN_OPEN, app);

    while (1)
    {
        if (flag = rhs_event_flag_wait(app->srv_event,
                                       CanOpenAppEventTypeRX | CanOpenAppEventTypeTX | CanOpenAppEventTypePDO,
                                       RHSFlagWaitAny,
                                       RHSWaitForever))
        {
            switch (flag)
            {
            case CanOpenAppEventTypeRX:
                while (rhs_message_queue_get(app->rx_queue, &msg, 0) == RHSStatusOk)
                {
                    rhs_kernel_lock();
                    rhs_assert(msg.od);
                    canDispatch(msg.od, &msg.data);
                    rhs_kernel_unlock();
                }
                break;
            case CanOpenAppEventTypePDO:
                for (uint8_t i = 0; i < app->counter_od; i++)
                {
                    sendPDOevent(app->handler[i].od);
                }
            case CanOpenAppEventTypeTX:
                while (rhs_message_queue_get(app->tx_queue, &msg, 0) == RHSStatusOk)
                {
                    uint8_t try_tx = 3;
                    frame.id       = msg.data.cob_id;
                    frame.type     = FrameTypeStdID;
                    frame.len      = msg.data.len;
                    frame.rtr      = msg.data.rtr;
                    memcpy(frame.payload, msg.data.data, msg.data.len);
                    while (!rhs_hal_can_tx(msg.can_id, &frame) && try_tx)
                    {
                        rhs_delay_ms(1);
                        try_tx--;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    can_open_app_free(app);
}
