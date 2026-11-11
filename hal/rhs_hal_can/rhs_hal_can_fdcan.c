/* rhs_hal_can for the FDCAN peripheral (STM32G0B1), classic CAN frames only.
 * rcan (bx_canfd backend) configures, starts and stops the peripheral and moves frames in and out of the message RAM.
 * Interrupts do not go through HAL_FDCAN_IRQHandler: rhs_hal_interrupt calls can_isr(), which keeps the same
 * callback model as the bxCAN implementation (rx callback -> rhs_hal_can_rx() per pending frame). */
#include "rcan.h"

#if defined(RCAN_BACKEND_BXCANFD)

#    include "stdbool.h"
#    include "rhs.h"
#    include "rhs_hal_can.h"
#    include "rhs_hal.h"

#    define TAG "rhs_hal_can"

/* G0B1 board: FDCAN1 RX PB8, TX PB9 (AF3) */
static const RHSGpioPin can1_rx_pin = {.port = GPIOB, .pin = LL_GPIO_PIN_8};
static const RHSGpioPin can1_tx_pin = {.port = GPIOB, .pin = LL_GPIO_PIN_9};

#    define CAN_RX_ITS (FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_MESSAGE_LOST)
#    define CAN_TX_ITS (FDCAN_IT_TX_COMPLETE)
#    define CAN_SCE_ITS (FDCAN_IT_BUS_OFF | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ARB_PROTOCOL_ERROR)

typedef struct
{
    bool                      enabled;
    bool                      isr_set;
    rcan                      rcan;
    uint8_t                   lec;
    RHSHalCANAsyncRxCallback  rx_callback;
    void*                     rx_context;
    RHSHalCANAsyncTxCallback  tx_callback;
    void*                     tx_context;
    RHSHalCANAsyncSCECallback sce_callback;
    void*                     sce_context;
    RHSHalCANStatistic        statistic;
} RHSHalCAN;

static RHSHalCAN rhs_hal_can[RHSHalCANIdMax] = {0};

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* handle)
{
    if (handle->Instance == FDCAN1)
    {
        /* bx_canfd computes the bit timing from PCLK1 */
        __HAL_RCC_FDCAN_CONFIG(RCC_FDCANCLKSOURCE_PCLK1);
        __HAL_RCC_FDCAN_CLK_ENABLE();

        rhs_hal_gpio_init_ex(&can1_rx_pin, RHSGpioModeAltFunctionPushPull, RHSGpioPullNo, RHSGpioSpeedHigh, RHSGpioAltFnUnused);
        rhs_hal_gpio_init_ex(&can1_tx_pin, RHSGpioModeAltFunctionPushPull, RHSGpioPullNo, RHSGpioSpeedHigh, RHSGpioAltFnUnused);
        LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF_3);
        LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_3);
    }
}

void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* handle)
{
    if (handle->Instance == FDCAN1)
    {
        __HAL_RCC_FDCAN_CLK_DISABLE();
        rhs_hal_gpio_init_simple(&can1_rx_pin, RHSGpioModeAnalog);
        rhs_hal_gpio_init_simple(&can1_tx_pin, RHSGpioModeAnalog);
    }
}

static void can_isr(void* context)
{
    rhs_assert(context);
    RHSHalCAN*           can      = (RHSHalCAN*) context;
    FDCAN_GlobalTypeDef* instance = can->rcan.handle.Instance;

    uint32_t ir  = instance->IR & instance->IE;
    instance->IR = ir;  // write 1 to clear, before the callbacks so a frame arriving meanwhile raises the IRQ again

    if (ir & FDCAN_IR_RF0L)
    {
        can->statistic.rx_ovfs++;
    }
    if (ir & FDCAN_IR_TC)
    {
        can->statistic.tx_msgs++;
        if (can->tx_callback)
        {
            can->tx_callback(can->tx_context);
        }
    }
    if (ir & (FDCAN_IR_BO | FDCAN_IR_EP | FDCAN_IR_PEA))
    {
        uint32_t          psr   = instance->PSR;
        RHSHalCANSCEEvent event = 0;

        can->lec = (uint8_t) (psr & FDCAN_PSR_LEC);
        if (psr & FDCAN_PSR_BO)
        {
            event |= RHSHalCANSCEEventBusOff;
        }
        if (psr & FDCAN_PSR_EP)
        {
            event |= (instance->ECR & FDCAN_ECR_RP) ? RHSHalCANSCEEventRXPassive : RHSHalCANSCEEventTXPassive;
        }
        if (ir & FDCAN_IR_PEA)
        {
            event |= RHSHalCANSCEEventError;
            if ((psr & FDCAN_PSR_ACT) == FDCAN_PSR_ACT)  // ACT = 11: node was transmitting
            {
                can->statistic.tx_errs++;
            }
            else
            {
                can->statistic.rx_errs++;
            }
        }
        if (can->sce_callback)
        {
            can->sce_callback(RHSHalCANId1, event, can->sce_context);
        }
    }
    if (ir & FDCAN_IR_RF0N)
    {
        /* One RF0N for any number of frames: let the callback read every frame that is in the FIFO now */
        uint32_t pending = instance->RXF0S & FDCAN_RXF0S_F0FL;
        for (uint32_t i = 0; i < pending && can->rx_callback; i++)
        {
            can->rx_callback(RHSHalCANId1, can->rx_context);
        }
    }
}

static void can_activate(RHSHalCANId id, uint32_t its, uint32_t buffer_its)
{
    if (!rhs_hal_can[id].isr_set)
    {
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdFDCANIT0, can_isr, &rhs_hal_can[id]);
        rhs_hal_can[id].isr_set = true;
    }
    rhs_assert(HAL_FDCAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, its, buffer_its) == HAL_OK);
}

/*********************************** CAN INIT ************************************/

void* rhs_hal_can_get_handle(RHSHalCANId id)
{
    return &rhs_hal_can[id].rcan.handle;
}

void rhs_hal_can_init(RHSHalCANId id, uint32_t baud)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == false);

    rhs_assert(rcan_start(&rhs_hal_can[id].rcan, (uint32_t) FDCAN1, baud) == true);
    rhs_hal_can[id].enabled = true;
}

void rhs_hal_can_deinit(RHSHalCANId id)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == true);

    if (rhs_hal_can[id].isr_set)
    {
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdFDCANIT0, NULL, NULL);
        rhs_hal_can[id].isr_set = false;
    }
    HAL_FDCAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_RX_ITS | CAN_TX_ITS | CAN_SCE_ITS);
    rcan_stop(&rhs_hal_can[id].rcan);

    rhs_hal_can[id].rx_callback  = NULL;
    rhs_hal_can[id].tx_callback  = NULL;
    rhs_hal_can[id].sce_callback = NULL;
    rhs_hal_can[id].enabled      = false;
}

void rhs_hal_can_async_sce(RHSHalCANId id, RHSHalCANAsyncSCECallback callback, void* context)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == true);

    rhs_hal_can[id].sce_callback = callback;
    rhs_hal_can[id].sce_context  = context;
    can_activate(id, CAN_SCE_ITS, 0);
}

bool rhs_hal_can_tx(RHSHalCANId id, RHSHalCANFrameType* frame)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == true);

    rcan_frame rcan_frame = {0};

    rcan_frame.id   = frame->id;
    rcan_frame.len  = frame->len;
    rcan_frame.type = frame->type;
    rcan_frame.rtr  = frame->rtr;
    memcpy(rcan_frame.payload, frame->payload, frame->len);
    if (rcan_send(&rhs_hal_can[id].rcan, &rcan_frame) == false)
    {
        rhs_hal_can[id].statistic.tx_ovfs++;
        return false;
    }
    return true;
}

void rhs_hal_can_tx_cmplt_cb(RHSHalCANId id, RHSHalCANAsyncTxCallback callback, void* context)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == true);
    rhs_assert(callback);
    rhs_assert(context);

    rhs_hal_can[id].tx_callback = callback;
    rhs_hal_can[id].tx_context  = context;
    can_activate(id, CAN_TX_ITS, FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2);
}

void rhs_hal_can_async_rx_start(RHSHalCANId id, RHSHalCANAsyncRxCallback callback, void* context)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == true);
    rhs_assert(callback);
    rhs_assert(context);

    rhs_hal_can[id].rx_callback = callback;
    rhs_hal_can[id].rx_context  = context;
    can_activate(id, CAN_RX_ITS, 0);
}

bool rhs_hal_can_rx(RHSHalCANId id, RHSHalCANFrameType* frame)
{
    rhs_assert(id == RHSHalCANId1);
    rhs_assert(rhs_hal_can[id].enabled == true);

    rcan_frame rcan_frame = {0};

    if (rcan_receive(&rhs_hal_can[id].rcan, &rcan_frame) == false)
    {
        return false;
    }
    frame->id   = rcan_frame.id;
    frame->len  = rcan_frame.len;
    frame->type = rcan_frame.type;
    frame->rtr  = rcan_frame.rtr;
    memcpy(frame->payload, rcan_frame.payload, rcan_frame.len);
    rhs_hal_can[id].statistic.rx_msgs++;
    return true;
}

RHSHalCANStatistic rhs_hal_can_get_statistic(RHSHalCANId id)
{
    rhs_assert(id < RHSHalCANIdMax);
    return rhs_hal_can[id].statistic;
}

#endif  // defined(RCAN_BACKEND_BXCANFD)
