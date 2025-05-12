#include "can_open.h"
#include "can_open_srv.h"
#include "rtimer.h"

extern CanOpenApp* can_open_app;

static void InitNodes(CO_Data* d, UNS32 id)
{
    /****************************** INITIALISATION MASTER *******************************/
    /* Defining the node Id */
    setNodeId(d, id);
    setState(d, Initialisation);
}

static void can_rx_irq_cb(RHSHalCANId can_id, void* context)
{
    rhs_assert(context);

    extern FilterId*   filter_list;
    CanOpenApp*        app = context;
    Message            rxm = {0};
    RHSHalCANFrameType frame;

    if (rhs_hal_can_rx(can_id, &frame) == false)
    {
        return;
    }

    for (FilterId* current = filter_list; current != NULL; current = current->next)
    {
        if (frame.id == current->id)
        {
            RHS_LOG_D(TAG,
                      "CAN%d: ID : %8x : LEN %d : %02x%02x%02x%02x%02x%02x%02x%02x",
                      can_id,
                      frame.id,
                      frame.len,
                      frame.payload[0],
                      frame.payload[1],
                      frame.payload[2],
                      frame.payload[3],
                      frame.payload[4],
                      frame.payload[5],
                      frame.payload[6],
                      frame.payload[7]);
            break;
        }
    }

    rxm.cob_id = frame.id;
    rxm.rtr    = frame.rtr;
    rxm.len    = frame.len;
    memcpy(rxm.data, frame.payload, frame.len);

    for (uint8_t i = 0; i < can_open_app->counter_od; i++)
    {
        if (can_id == can_open_app->handler[i].can_id)
        {
            CanOpenAppMessage msg = {.od = can_open_app->handler[i].od, .can_id = can_id, .data = rxm};
            rhs_message_queue_put(app->rx_queue, &msg, 0);
            rhs_event_flag_set(app->srv_event, CanOpenAppEventTypeRX);
            return;
        }
    }
}

static void can_tx_irq_cb(void* context)
{
    rhs_assert(context);
    CanOpenApp* app = context;
    if (app->tx_queue)
    {
        rhs_thread_flags_set(rhs_thread_get_id(app->thread), WORKER_TX_COMPLETED);
    }
}

static void can_sce_irq_cb(RHSHalCANId can_id, RHSHalCANSCEEvent event, void* context)
{
    rhs_assert(context);
    CanOpenApp* app = context;
    if (event & RHSHalCANSCEEventTXPassive)  // It means that line is empty
    {
        for (uint8_t i = 0; i < can_open_app->counter_od; i++)
        {
            if (can_id == can_open_app->handler[i].can_id)
            {
                setState(app->handler[i].od, Stopped);
                rhs_hal_can_deinit(app->handler[i].can_id);
                RHS_LOG_W(TAG, "CAN %d is stopped", app->handler[i].can_id);
                return;
            }
        }
    }
}

UNS8 GetSDOClientFromNodeId(CO_Data* d, UNS8 nodeId);

static void can_open_sdo_cb(CO_Data* d, UNS8 nodeId)
{
    UNS8       CliNbr;
    UNS8       line;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult;

    rhs_assert(can_open_app);

    CliNbr = GetSDOClientFromNodeId(d, nodeId);
    getSDOlineToClose(d, CliNbr, SDO_CLIENT, &line);

    if (d->transfers[line].state != SDO_ABORTED_INTERNAL && d->transfers[line].state != SDO_ABORTED_RCV)
    {
        if (can_open_app->sdo_callback)

            can_open_app->sdo_callback(d,
                                       nodeId,
                                       d->transfers[line].index,
                                       d->transfers[line].subIndex,
                                       d->transfers[line].count,
                                       d->transfers[line].data);
    }
    else
    {
        if (d->transfers[line].state == SDO_ABORTED_RCV)
        {
            RHS_LOG_W(TAG,
                      "node 0x%02X index 0x%04X sub 0x%02X response 0x%08X",
                      nodeId,
                      d->transfers[line].index,
                      d->transfers[line].subIndex,
                      d->transfers[line].abortCode);
        }
        closeSDOtransfer(d, nodeId, SDO_CLIENT);
    }

    rhs_event_flag_set(can_open_app->sdo_event, EVENT_FLAG_SDO);
}

uint8_t can_open_read_sdo(CO_Data* d, uint8_t node_id, uint16_t index, uint8_t subindex, sdo_cb cb)
{
    uint8_t result;
    rhs_assert(can_open_app);
    rhs_assert(rhs_mutex_acquire(can_open_app->sdo_mutex, RHSWaitForever) == RHSStatusOk);

    can_open_app->sdo_callback = cb;
    uint32_t time = rhs_get_tick();
    result                     = readNetworkDictCallback(d, node_id, index, subindex, 0, can_open_sdo_cb, 0);
    if (result == CANOPEN_SDO_STATE_OK)
    {
        if (rhs_event_flag_wait(can_open_app->sdo_event, EVENT_FLAG_SDO, RHSFlagWaitAny, SDO_TIMEOUT_MS) ==
            EVENT_FLAG_SDO)
        {
            rhs_mutex_release(can_open_app->sdo_mutex);
            return result;
        }
    }
    resetSDO(d);
    can_open_app->sdo_callback = NULL;
    RHS_LOG_E(TAG, "SDO read in %d error %d for 0x%02X, i=0x%04X s=0x%02X", time, result, node_id, index, subindex);
    rhs_mutex_release(can_open_app->sdo_mutex);
    return result ? result : -1;
}

uint8_t can_open_write_sdo(CO_Data* d,
                           uint8_t  node_id,
                           uint16_t index,
                           uint8_t  subindex,
                           uint32_t count,
                           void*    data,
                           sdo_cb   cb)
{
    uint8_t result;
    rhs_assert(can_open_app);
    rhs_assert(rhs_mutex_acquire(can_open_app->sdo_mutex, RHSWaitForever) == RHSStatusOk);

    can_open_app->sdo_callback = cb;
    uint32_t time = rhs_get_tick();
    result = writeNetworkDictCallBack(d, node_id, index, subindex, count, 0, data, can_open_sdo_cb, 0);
    if (result == CANOPEN_SDO_STATE_OK)
    {
        if (rhs_event_flag_wait(can_open_app->sdo_event, EVENT_FLAG_SDO, RHSFlagWaitAny, SDO_TIMEOUT_MS) ==
            EVENT_FLAG_SDO)
        {
            rhs_mutex_release(can_open_app->sdo_mutex);
            return result;
        }
    }
    resetSDO(d);
    can_open_app->sdo_callback = NULL;
    RHS_LOG_E(TAG, "SDO write in %d error %d for 0x%02X, i=0x%04X s=0x%02X", time, result, node_id, index, subindex);
    rhs_mutex_release(can_open_app->sdo_mutex);
    return result ? result : -1;
}

uint8_t can_open_set_field(CO_Data* d, uint16_t index, uint8_t subIndex, const void* data, uint32_t size_data)
{
    rhs_assert(can_open_app);
    uint32_t error_code = setODentry(d, index, subIndex, (void*) data, &size_data, 1);
    if (error_code == OD_SUCCESSFUL)
    {
        rhs_event_flag_set(can_open_app->srv_event, CanOpenAppEventTypePDO);
    }
    return error_code;
}

int canSend(CAN_PORT port, Message const* m)
{
    rhs_assert(port);
    rhs_assert(m);
    rhs_assert(can_open_app);
    for (uint8_t i = 0; i < can_open_app->counter_od; i++)
    {
        if (port == can_open_app->handler[i].od->canHandle)
        {
            CanOpenAppMessage msg = {.od     = can_open_app->handler[i].od,
                                     .can_id = can_open_app->handler[i].can_id,
                                     .data   = *m};
            rhs_message_queue_put(can_open_app->tx_queue, &msg, 0);
            rhs_event_flag_set(can_open_app->srv_event, CanOpenAppEventTypeTX);
            return 0;
        }
    }
    rhs_crash("CAN_PORT doesn't reg for OD");
}

void can_open_start_node(CO_Data* d, uint8_t node_id, RHSHalCANId id, uint32_t baud)
{
    rhs_assert(can_open_app);
    rhs_assert(can_open_app->counter_od < MAX_OD);

    rhs_hal_can_init(id, baud);
    rhs_hal_can_async_rx_start(id, can_rx_irq_cb, can_open_app);
    rhs_hal_can_tx_cmplt_cb(id, can_tx_irq_cb, can_open_app);
    rhs_hal_can_async_sce(id, can_sce_irq_cb, can_open_app);

    can_open_app->handler[can_open_app->counter_od].can_id        = id;
    can_open_app->handler[can_open_app->counter_od].od            = d;
    can_open_app->handler[can_open_app->counter_od].od->canHandle = rhs_hal_can_get_handle(id);
    can_open_app->counter_od++;
    InitNodes(can_open_app->handler[can_open_app->counter_od - 1].od, node_id);
}

static rtimer timer;

void timer_notify(void)
{
    TimeDispatch();
}

void TimerInit(void)
{
    rhs_assert(rtimer_create(&timer));
}

void TimerCleanup(void)
{
    rhs_assert(rtimer_delete(&timer));
}

void StartTimerLoop(TimerCallback_t init_callback)
{
    rtimer_setup(&timer, 500, timer_notify);
    SetAlarm(NULL, 0, init_callback, MS_TO_TIMEVAL(1500), 0);
}

void StopTimerLoop(TimerCallback_t exitfunction)
{
    rtimer_delete(&timer);
    exitfunction(NULL, 0);
}

void setTimer(TIMEVAL value)
{
    rhs_assert(rtimer_setup(&timer, value, timer_notify));
}

TIMEVAL getElapsedTime(void)
{
    return rtimer_get_elapsed_time(&timer);
}
