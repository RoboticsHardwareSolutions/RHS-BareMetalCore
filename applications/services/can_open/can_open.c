#include <stdarg.h>
#include "can_open.h"
#include "can_open_i.h"
#include "cli.h"

static CanOpenApp* can_open_app_alloc(void)
{
    CanOpenApp* app = malloc(sizeof(CanOpenApp));
    memset(app, 0, sizeof(CanOpenApp));
    app->srv_event  = rhs_event_flag_alloc();
    app->sdo_event  = rhs_event_flag_alloc();
    app->sdo_mutex  = rhs_mutex_alloc(RHSMutexTypeNormal);
    app->rx_queue   = rhs_message_queue_alloc(32, sizeof(CanOpenAppMessage));
    app->tx_queue   = rhs_message_queue_alloc(32, sizeof(CanOpenAppMessage));
    TimerInit();

    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "can_open", can_open_cli, app);
    rhs_record_close(RECORD_CLI);

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

int32_t can_open_srv(void* context)
{
    CanOpenApp*        app  = can_open_app_alloc();
    uint32_t           flag = 0;
    CanOpenAppMessage  msg;
    RHSHalCANFrameType frame = {0};

    rhs_record_create(RECORD_CAN_OPEN, app);

    while (1)
    {
        flag = rhs_event_flag_wait(app->srv_event,
                                   CanOpenAppEventTypeRX | CanOpenAppEventTypeTX | CanOpenAppEventTypePDO,
                                   RHSFlagWaitAny,
                                   RHSWaitForever);
        if (flag & RHSFlagError)
        {
            continue;
        }

        /* Several events may arrive at once: handle each set bit, not only a single one */
        if (flag & CanOpenAppEventTypeRX)
        {
            while (rhs_message_queue_get(app->rx_queue, &msg, 0) == RHSStatusOk)
            {
                rhs_kernel_lock();
                rhs_assert(msg.od);
                canDispatch(msg.od, &msg.data);
                rhs_kernel_unlock();
            }
        }
        if (flag & CanOpenAppEventTypePDO)
        {
            /* sendPDOevent() queues frames to tx_queue, they are sent below */
            for (uint8_t i = 0; i < app->counter_od; i++)
            {
                sendPDOevent(app->handler[i].od);
            }
        }
        if (flag & (CanOpenAppEventTypeTX | CanOpenAppEventTypePDO))
        {
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
        }
    }
    can_open_app_free(app);
    return 0;
}
