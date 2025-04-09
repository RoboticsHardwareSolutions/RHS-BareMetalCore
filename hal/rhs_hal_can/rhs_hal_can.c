#include "stdbool.h"
#include "rcan.h"
#include "rhs.h"
#include "rhs_hal_can.h"
#include "rhs_hal.h"

#define TAG "rhs_hal_can"

#define CAN_LOG_E(...) RHS_LOG_E(TAG, __VA_ARGS__)
#ifndef CAN_LOG_E
#    define CAN_LOG_E(...)
#endif

typedef struct
{
    bool                      enabled;
    rcan                      rcan;
    uint8_t                   lec;
    uint8_t                   tec;
    uint8_t                   rec;
    RHSHalCANAsyncRxCallback  rx_callback;
    void*                     rx_context;
    RHSHalCANAsyncTxCallback  tx_callback;
    void*                     tx_context;
    RHSHalCANAsyncSCECallback sce_callback;
    void*                     sce_context;
} RHSHalCAN;

static RHSHalCAN rhs_hal_can[RHSHalCANIdMax] = {0};

static uint32_t HAL_RCC_CAN1_CLK_ENABLED = 0;

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
#ifdef STM32F765xx
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (canHandle->Instance == CAN1)
    {
        __HAL_RCC_CAN1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**CAN1 GPIO Configuration
        PA11     ------> CAN1_RX
        PA12     ------> CAN1_TX
        */
        GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
#elif STM32F407xx
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (canHandle->Instance == CAN1)
    {
        HAL_RCC_CAN1_CLK_ENABLED++;
        if (HAL_RCC_CAN1_CLK_ENABLED == 1)
        {
            __HAL_RCC_CAN1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOD_CLK_ENABLE();
        GPIO_InitStruct.Pin       = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    }
    else if (canHandle->Instance == CAN2)
    {
        __HAL_RCC_CAN2_CLK_ENABLE();
        HAL_RCC_CAN1_CLK_ENABLED++;
        if (HAL_RCC_CAN1_CLK_ENABLED == 1)
        {
            __HAL_RCC_CAN1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
#else
#    error "No processor defined or not implemented"
#endif
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{
#ifdef STM32F765xx
    if (canHandle->Instance == CAN1)
    {
        /* Peripheral clock disable */
        __HAL_RCC_CAN1_CLK_DISABLE();

        /**CAN1 GPIO Configuration
        PA11     ------> CAN1_RX
        PA12     ------> CAN1_TX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    }
#elif STM32F407xx
    if (canHandle->Instance == CAN1)
    {
        HAL_RCC_CAN1_CLK_ENABLED--;
        if (HAL_RCC_CAN1_CLK_ENABLED == 0)
        {
            __HAL_RCC_CAN1_CLK_DISABLE();
        }
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0 | GPIO_PIN_1);
    }
    else if (canHandle->Instance == CAN2)
    {
        __HAL_RCC_CAN2_CLK_DISABLE();
        HAL_RCC_CAN1_CLK_ENABLED--;
        if (HAL_RCC_CAN1_CLK_ENABLED == 0)
        {
            __HAL_RCC_CAN1_CLK_DISABLE();
        }
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5 | GPIO_PIN_6);
    }
#else
#    error "No processor defined or not implemented"
#endif
}

static uint8_t get_can_num_interface(CAN_TypeDef* can)
{
    if (can == CAN1)
    {
        return RHSHalCANId1;
    }
#if !defined(RPLC_XL) && !defined(RPLC_L) && !defined(RPLC_M)
    else if (can == CAN2)
    {
        return RHSHalCANId2;
    }
#endif
    else
    {
        rhs_crash("No CAN interface");
    }
}

static void print_ecr(CAN_TypeDef* can)
{
    uint32_t ecr     = can->ESR;
    uint8_t  tec     = (ecr >> 16) & 0xFF;
    uint8_t  rec     = (ecr >> 24) & 0xFF;
    uint8_t  lec     = (ecr >> 4) & 0x03;
    uint8_t  can_num = get_can_num_interface(can);

    if (lec == 0b000 && rec == 0 && tec == 0)
    {
        return;
    }

    if (lec == 0b001)
    {
        CAN_LOG_E("CAN%d Stuff error", can_num);
        /* Following the bit stuffing process, if more than five consecutive bits of
    the same level occur on the Bus, a "Stuff Error" is signalled. */
    }
    if (lec == 0b010)
    {
        CAN_LOG_E("CAN%d Form error", can_num);
        /* Form check checks the CAN frame sent/received for a standard
    format. Violation of fixed bit format results in a Form Error. */
    }
    if (lec == 0b011)
    {
        CAN_LOG_E("CAN%d Acknowledgment error", can_num);
        /* An acknowledgment error happens when the transmitter
    doesn’t receive an Acknowledgment (ACK) bit from other nodes
    after transmitting a data frame. */
    }
    if (lec == 0b100)
    {
        RHS_LOG_E(TAG, "CAN%d Bit recessive error", can_num);
        /* This occurs when the transmitter sends a dominant bit (logic ‘0’), but
    sees a recessive bit (logic ‘1’) on the bus. In CAN, a dominant bit
    “overwrites” a recessive one. So, if a transmitter trying to send ‘0’
    sees ‘1’, either another node is transmitting ‘1’ simultaneously, or
    there’s an issue with the transmitter itself.  */
    }
    if (lec == 0b101)
    {
        CAN_LOG_E("CAN%d Bit dominant error", can_num);
        /* This happens when the transmitter sends a recessive bit
    (logic ‘1’), but sees a dominant bit (logic ‘0’) on the bus. This
    means another node is simultaneously transmitting a ‘0’ (which
    dominates on the bus), or there’s a bus driver problem. */
    }
    if (lec == 0b110)
    {
        CAN_LOG_E("CAN%d CRC error", can_num);
        /*  A Cyclic Redundancy Check (CRC) error. */
    }
    if (lec == 0b111)
    {
        CAN_LOG_E("CAN%d Set by software", can_num);
        /* This error isn’t a hardware error detected by the CAN controller
    itself. It’s set by the software for purposes like debugging, testing,
    or handling user-defined exception situations. For instance, it might
    signal invalid data, a timeout, or other software problems. */
    }
    CAN_LOG_E("CAN%d rec: %d, tec: %d", can_num, rec, tec);
}

static void can_sce_callback(void* context)
{
    rhs_assert(context);
    RHSHalCAN*        can        = (RHSHalCAN*) context;
    CAN_TypeDef*      can_handle = can->rcan.handle.Instance;
    uint8_t           can_num    = get_can_num_interface(can_handle);
    RHSHalCANSCEEvent event      = 0;

    if (can_handle->MSR & CAN_MSR_ERRI)
    {
        CAN_LOG_E("CAN%d ERRI", can_num);
        can_handle->MSR = CAN_MSR_ERRI;
        event |= RHSHalCANSCEEventError;
    }
    if (can_handle->MSR & CAN_MSR_WKUI)
    {
        CAN_LOG_E("CAN%d WKUI", can_num);
        can_handle->MSR = CAN_MSR_WKUI;
    }
    if (can_handle->ESR & CAN_ESR_BOFF)
    {
        CAN_LOG_E("CAN%d BOFF", can_num);
        event |= RHSHalCANSCEEventBusOff;
    }
    if (can_handle->TSR & CAN_TSR_ALST0 || can_handle->TSR & CAN_TSR_ALST1 || can_handle->TSR & CAN_TSR_ALST2)
    {
        CAN_LOG_E("CAN%d Arbitration lost", can_num);
    }
    if (can_handle->TSR & CAN_TSR_TERR0 || can_handle->TSR & CAN_TSR_TERR1 || can_handle->TSR & CAN_TSR_TERR2)
    {
        CAN_LOG_E("CAN%d Transmition error", can_num);
        if (((can_handle->ESR >> 16) & 0xFF) >= 0x80)
        {
            // can_handle->TSR = CAN_TSR_ABRQ0;
            // can_handle->TSR = CAN_TSR_ABRQ1;
            // can_handle->TSR = CAN_TSR_ABRQ2;
            event |= RHSHalCANSCEEventTXPassive;
        }
    }

    can->lec = (can_handle->ESR >> 4) & 0x03;
    print_ecr(can_handle);

    if (can->sce_callback)
    {
        can->sce_callback(can_num, event, can->sce_context);
    }
}

static void can_rx_callback(void* context)
{
    rhs_assert(context);
    RHSHalCAN*   can        = (RHSHalCAN*) context;
    CAN_TypeDef* can_handle = can->rcan.handle.Instance;
    uint8_t      can_num    = get_can_num_interface(can_handle);
    if (can->rx_callback)
    {
        can->rx_callback(can_num, can->rx_context);
    }
}

static void can_tx_callback(void* context)
{
    rhs_assert(context);
    RHSHalCAN*   can        = (RHSHalCAN*) context;
    CAN_TypeDef* can_handle = can->rcan.handle.Instance;

    if (can_handle->TSR & CAN_TSR_RQCP0)
    {
        can_handle->TSR = CAN_TSR_RQCP0;
    }
    if (can_handle->TSR & CAN_TSR_RQCP1)
    {
        can_handle->TSR = CAN_TSR_RQCP1;
    }
    if (can_handle->TSR & CAN_TSR_RQCP2)
    {
        can_handle->TSR = CAN_TSR_RQCP2;
    }

    if (can->tx_callback)
    {
        can->tx_callback(can->tx_context);
    }
}

/*********************************** CAN INIT ************************************/

void* rhs_hal_can_get_handle(RHSHalCANId id)
{
    return &rhs_hal_can[id].rcan.handle;
}

void rhs_hal_can_init(RHSHalCANId id, uint32_t baud)
{
    rhs_assert(rhs_hal_can[id].enabled == false);

    switch (id)
    {
    case RHSHalCANId1:
        rhs_assert(rcan_start(&rhs_hal_can[id].rcan, (uint32_t) CAN1, baud) == true);
        rhs_hal_can[id].enabled = true;
        break;
#if !defined(RPLC_XL) && !defined(RPLC_L) && !defined(RPLC_M)
    case RHSHalCANId2:
        rhs_assert(rcan_start(&rhs_hal_can[id].rcan, (uint32_t) CAN2, baud) == true);
        rhs_hal_can[id].enabled = true;
        break;
#endif
    case RHSHalCANIdMax:
    default:
        rhs_crash("No CAN interface");
    }
}

void rhs_hal_can_deinit(RHSHalCANId id)
{
    rhs_assert(rhs_hal_can[id].enabled == true);
    switch (id)
    {
    case RHSHalCANId1:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN1SCE, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN1Tx, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN1Rx0, NULL, NULL);
        break;
#if !defined(RPLC_XL) && !defined(RPLC_L) && !defined(RPLC_M)
    case RHSHalCANId2:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN2SCE, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN2Tx, NULL, NULL);
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN2Rx0, NULL, NULL);
        break;
#endif
    case RHSHalCANIdMax:
    default:
        rhs_crash("No CAN interface");
    }
    rcan_stop(&rhs_hal_can[id].rcan);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_ERROR);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_LAST_ERROR_CODE);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_BUSOFF);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_ERROR_PASSIVE);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_ERROR_WARNING);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_TX_MAILBOX_EMPTY);
    HAL_CAN_DeactivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_RX_FIFO0_MSG_PENDING);
    rhs_hal_can[id].enabled = false;
}

void rhs_hal_can_async_sce(RHSHalCANId id, RHSHalCANAsyncSCECallback callback, void* context)
{
    rhs_assert(rhs_hal_can[id].enabled == true);

    rhs_hal_can[id].sce_callback = callback;
    rhs_hal_can[id].sce_context  = context;

    switch (id)
    {
    case RHSHalCANId1:
        rhs_hal_interrupt_set_isr_ex(RHSHalInterruptIdCAN1SCE, RHSHalInterruptPriorityLow, can_sce_callback, &rhs_hal_can[id]);
        break;
#if !defined(RPLC_XL) && !defined(RPLC_L) && !defined(RPLC_M)
    case RHSHalCANId2:
        rhs_hal_interrupt_set_isr_ex(RHSHalInterruptIdCAN2SCE, RHSHalInterruptPriorityLow, can_sce_callback, &rhs_hal_can[id]);
        break;
#endif
    case RHSHalCANIdMax:
    default:
        rhs_crash("No CAN interface");
    }

    HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_ERROR);
    HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_LAST_ERROR_CODE);
    HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_BUSOFF);
    // HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_ERROR_PASSIVE);
    // HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_ERROR_WARNING);
}

bool rhs_hal_can_tx(RHSHalCANId id, RHSHalCANFrameType* frame)
{
    rhs_assert(rhs_hal_can[id].enabled == true);
    static uint8_t tec = 0;

    tec = (rhs_hal_can[id].rcan.handle.Instance->ESR >> 16) & 0xFF;
    if (rhs_hal_can[id].tec != tec)
    {
        print_ecr(rhs_hal_can[id].rcan.handle.Instance);
        rhs_hal_can[id].tec = tec;
    }
    rcan_frame rcan_frame = {0};

    rcan_frame.id   = frame->id;
    rcan_frame.len  = frame->len;
    rcan_frame.type = frame->type;
    rcan_frame.rtr  = frame->rtr;
    memcpy(rcan_frame.payload, frame->payload, frame->len);
    return rcan_send(&rhs_hal_can[id].rcan, &rcan_frame);
}

void rhs_hal_can_tx_cmplt_cb(RHSHalCANId id, RHSHalCANAsyncTxCallback callback, void* context)
{
    rhs_assert(rhs_hal_can[id].enabled == true);
    rhs_assert(callback);
    rhs_assert(context);

    rhs_hal_can[id].tx_callback = callback;
    rhs_hal_can[id].tx_context  = context;

    switch (id)
    {
    case RHSHalCANId1:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN1Tx, can_tx_callback, &rhs_hal_can[id]);
        break;
#if !defined(RPLC_XL) && !defined(RPLC_L) && !defined(RPLC_M)
    case RHSHalCANId2:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN2Tx, can_tx_callback, &rhs_hal_can[id]);
        break;
#endif
    case RHSHalCANIdMax:
    default:
        rhs_crash("No CAN interface");
    }

    HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_TX_MAILBOX_EMPTY);
}

void rhs_hal_can_async_rx_start(RHSHalCANId id, RHSHalCANAsyncRxCallback callback, void* context)
{
    rhs_assert(rhs_hal_can[id].enabled == true);
    rhs_assert(callback);
    rhs_assert(context);

    rhs_hal_can[id].rx_callback = callback;
    rhs_hal_can[id].rx_context  = context;

    switch (id)
    {
    case RHSHalCANId1:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN1Rx0, can_rx_callback, &rhs_hal_can[id]);
        break;
#if !defined(RPLC_XL) && !defined(RPLC_L) && !defined(RPLC_M)
    case RHSHalCANId2:
        rhs_hal_interrupt_set_isr(RHSHalInterruptIdCAN2Rx0, can_rx_callback, &rhs_hal_can[id]);
        break;
#endif
    case RHSHalCANIdMax:
    default:
        rhs_crash("No CAN interface");
    }

    HAL_CAN_ActivateNotification(&rhs_hal_can[id].rcan.handle, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void rhs_hal_can_rx(RHSHalCANId id, RHSHalCANFrameType* frame)
{
    rhs_assert(rhs_hal_can[id].enabled == true);
    static uint8_t rec = 0;

    rec = (rhs_hal_can[id].rcan.handle.Instance->ESR >> 24) & 0xFF;
    if (rhs_hal_can[id].rec != rec)
    {
        print_ecr(rhs_hal_can[id].rcan.handle.Instance);
        rhs_hal_can[id].rec = rec;
    }

    rcan_frame rcan_frame = {0};

    rcan_receive(&rhs_hal_can[id].rcan, &rcan_frame);
    frame->id   = rcan_frame.id;
    frame->len  = rcan_frame.len;
    frame->type = rcan_frame.type;
    frame->rtr  = rcan_frame.rtr;
    memcpy(frame->payload, rcan_frame.payload, rcan_frame.len);
}
