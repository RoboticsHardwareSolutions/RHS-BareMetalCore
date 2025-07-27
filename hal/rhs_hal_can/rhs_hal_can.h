#pragma once
#include "stdint.h"
#include "stdbool.h"

typedef enum
{
    FrameTypeNO,
    FrameTypeStdID,
    FrameTypeExtID,
} FrameType;

typedef struct
{
    uint32_t  id;
    uint8_t   len;
    FrameType type;
    bool      rtr;
    uint8_t   payload[8];
} RHSHalCANFrameType;

typedef struct
{
    uint32_t tx_msgs;
    uint32_t tx_errs;
    uint32_t tx_ovfs;
    uint32_t rx_msgs;
    uint32_t rx_errs;
    uint32_t rx_ovfs;
} RHSHalCANStatistic;

/**
 * CAN channels
 */
typedef enum
{
    RHSHalCANId1,
#if !defined(BMPLC_XL) && !defined(BMPLC_L) && !defined(BMPLC_M)
    RHSHalCANId2,
#endif
    RHSHalCANIdMax,
} RHSHalCANId;

void* rhs_hal_can_get_handle(RHSHalCANId id);

void rhs_hal_can_init(RHSHalCANId id, uint32_t baud);

void rhs_hal_can_deinit(RHSHalCANId id);

/** SCE events */
typedef enum
{
    RHSHalCANSCEEventTXPassive = (1 << 0),
    RHSHalCANSCEEventRXPassive = (1 << 1),
    RHSHalCANSCEEventBusOff    = (1 << 2),
    RHSHalCANSCEEventError     = (1 << 3),
} RHSHalCANSCEEvent;
/** Events callback
 *
 * @warning    Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 * @param      handle   CAN handle
 * @param      event    RHSHalCANSCEEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalCANAsyncSCECallback)(RHSHalCANId id, RHSHalCANSCEEvent event, void* context);

void rhs_hal_can_async_sce(RHSHalCANId id, RHSHalCANAsyncSCECallback callback, void* context);

/** TRANSMISSION */

bool rhs_hal_can_tx(RHSHalCANId id, RHSHalCANFrameType* frame);

/** Tx callback
 *
 * @warning    Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 * @param      handle   CAN handle
 * @param      event    RHSHalCANTxEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalCANAsyncTxCallback)(void* context);

void rhs_hal_can_tx_cmplt_cb(RHSHalCANId id, RHSHalCANAsyncTxCallback callback, void* context);

/** RECEIVE */

/** Receive callback
 *
 * @warning    Callback will be called in interrupt context, ensure thread
 *             safety on your side.
 * @param      handle   CAN handle
 * @param      event    RHSHalCANRxEvent
 * @param      context  Callback context provided earlier
 */
typedef void (*RHSHalCANAsyncRxCallback)(RHSHalCANId id, void* context);

void rhs_hal_can_async_rx_start(RHSHalCANId id, RHSHalCANAsyncRxCallback callback, void* context);

bool rhs_hal_can_rx(RHSHalCANId id, RHSHalCANFrameType* frame);

RHSHalCANStatistic rhs_hal_can_get_statistic(RHSHalCANId id);
