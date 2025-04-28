#include <rhs_hal_version.h>
#include <rhs_hal_usb_i.h>
#include <rhs_hal_usb.h>
#include <rhs_hal_power.h>

#include <rhs.h>

#if defined(STM32F103xE)
#    include <stm32f1xx_ll_pwr.h>
#    include <stm32f1xx_ll_rcc.h>
#    include <stm32f1xx_ll_gpio.h>
#endif

#include "usb.h"

#define TAG "RHSHalUsb"

#define USB_RECONNECT_DELAY 500

typedef enum
{
    UsbApiEventTypeSetConfig,
    UsbApiEventTypeGetConfig,
    UsbApiEventTypeLock,
    UsbApiEventTypeUnlock,
    UsbApiEventTypeIsLocked,
    UsbApiEventTypeEnable,
    UsbApiEventTypeDisable,
    UsbApiEventTypeReinit,
    UsbApiEventTypeSetStateCallback,
} UsbApiEventType;

typedef struct
{
    RHSHalUsbStateCallback callback;
    void*                  context;
} UsbApiEventDataStateCallback;

typedef struct
{
    RHSHalUsbInterface* interface;
    void*               context;
} UsbApiEventDataInterface;

typedef union
{
    UsbApiEventDataStateCallback state_callback;
    UsbApiEventDataInterface     interface;
} UsbApiEventData;

typedef union
{
    bool  bool_value;
    void* void_value;
} UsbApiEventReturnData;

typedef struct
{
    RHSApiLock             lock;
    UsbApiEventType        type;
    UsbApiEventData        data;
    UsbApiEventReturnData* return_data;
} UsbApiEventMessage;

typedef struct
{
    RHSThread*             thread;
    RHSMessageQueue*       queue;
    bool                   enabled;
    bool                   connected;
    bool                   mode_lock;
    bool                   request_pending;
    RHSHalUsbInterface*    interface;
    void*                  interface_context;
    RHSHalUsbStateCallback callback;
    void*                  callback_context;
} UsbSrv;

typedef enum
{
    UsbEventReset   = (1 << 0),
    UsbEventRequest = (1 << 1),
    UsbEventMessage = (1 << 2),
} UsbEvent;

#define USB_SRV_ALL_EVENTS (UsbEventReset | UsbEventRequest | UsbEventMessage)

static UsbSrv   usb = {0};
static uint32_t ubuf[0x20];
usbd_device     udev;

static const struct usb_string_descriptor dev_lang_desc = USB_ARRAY_DESC(USB_LANGID_ENG_US);

static int32_t      rhs_hal_usb_thread(void* context);
static usbd_respond usb_descriptor_get(usbd_ctlreq* req, void** address, uint16_t* length);
static void         reset_evt(usbd_device* dev, uint8_t event, uint8_t ep);
static void         susp_evt(usbd_device* dev, uint8_t event, uint8_t ep);
static void         wkup_evt(usbd_device* dev, uint8_t event, uint8_t ep);

/* Low-level init */
void rhs_hal_usb_init(void)
{
    GPIO_InitTypeDef         GPIO_InitStruct     = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    /** Initializes the peripherals clock
     */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;

    rhs_assert(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK);
    __HAL_RCC_USB_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    usbd_init(&udev, &usbd_hw, USB_EP0_SIZE, ubuf, sizeof(ubuf));

    taskENTER_CRITICAL();
    usbd_enable(&udev, true);
    taskEXIT_CRITICAL();

    usbd_reg_descr(&udev, usb_descriptor_get);
    usbd_reg_event(&udev, usbd_evt_susp, susp_evt);
    usbd_reg_event(&udev, usbd_evt_wkup, wkup_evt);
    // Reset callback will be enabled after first mode change to avoid getting false reset events

    usb.enabled   = false;
    usb.interface = NULL;
    NVIC_SetPriority(USB_LP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_SetPriority(USB_HP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(USB_LP_IRQn);
    NVIC_EnableIRQ(USB_HP_IRQn);

    usb.queue  = rhs_message_queue_alloc(1, sizeof(UsbApiEventMessage));
    usb.thread = rhs_thread_alloc_service("UsbDriver", 1024, rhs_hal_usb_thread, NULL);
    rhs_thread_start(usb.thread);

    RHS_LOG_I(TAG, "Init OK");
}

static void rhs_hal_usb_send_message(UsbApiEventMessage* message)
{
    rhs_message_queue_put(usb.queue, message, RHSWaitForever);
    rhs_thread_flags_set(rhs_thread_get_id(usb.thread), UsbEventMessage);
    api_lock_wait_unlock_and_free(message->lock);
}

bool rhs_hal_usb_set_config(RHSHalUsbInterface* new_if, void* ctx)
{
    UsbApiEventReturnData return_data = {
        .bool_value = false,
    };

    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeSetConfig,
        .data.interface =
            {
                .interface = new_if,
                .context   = ctx,
            },
        .return_data = &return_data,
    };

    rhs_hal_usb_send_message(&msg);
    return return_data.bool_value;
}

RHSHalUsbInterface* rhs_hal_usb_get_config(void)
{
    UsbApiEventReturnData return_data = {
        .void_value = NULL,
    };

    UsbApiEventMessage msg = {
        .lock        = api_lock_alloc_locked(),
        .type        = UsbApiEventTypeGetConfig,
        .return_data = &return_data,
    };

    rhs_hal_usb_send_message(&msg);
    return return_data.void_value;
}

void rhs_hal_usb_lock(void)
{
    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeLock,
    };

    rhs_hal_usb_send_message(&msg);
}

void rhs_hal_usb_unlock(void)
{
    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeUnlock,
    };

    rhs_hal_usb_send_message(&msg);
}

bool rhs_hal_usb_is_locked(void)
{
    UsbApiEventReturnData return_data = {
        .bool_value = false,
    };

    UsbApiEventMessage msg = {
        .lock        = api_lock_alloc_locked(),
        .type        = UsbApiEventTypeIsLocked,
        .return_data = &return_data,
    };

    rhs_hal_usb_send_message(&msg);
    return return_data.bool_value;
}

void rhs_hal_usb_disable(void)
{
    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeDisable,
    };

    rhs_hal_usb_send_message(&msg);
}

void rhs_hal_usb_enable(void)
{
    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeEnable,
    };

    rhs_hal_usb_send_message(&msg);
}

void rhs_hal_usb_reinit(void)
{
    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeReinit,
    };

    rhs_hal_usb_send_message(&msg);
}

void rhs_hal_usb_set_state_callback(RHSHalUsbStateCallback cb, void* ctx)
{
    UsbApiEventMessage msg = {
        .lock = api_lock_alloc_locked(),
        .type = UsbApiEventTypeSetStateCallback,
        .data.state_callback =
            {
                .callback = cb,
                .context  = ctx,
            },
    };

    rhs_hal_usb_send_message(&msg);
}

/* Get device / configuration descriptors */
static usbd_respond usb_descriptor_get(usbd_ctlreq* req, void** address, uint16_t* length)
{
    const uint8_t dtype   = req->wValue >> 8;
    const uint8_t dnumber = req->wValue & 0xFF;
    const void*   desc;
    uint16_t      len = 0;
    if (usb.interface == NULL)
        return usbd_fail;

    switch (dtype)
    {
    case USB_DTYPE_DEVICE:
        rhs_thread_flags_set(rhs_thread_get_id(usb.thread), UsbEventRequest);
        if (usb.callback != NULL)
        {
            usb.callback(RHSHalUsbStateEventDescriptorRequest, usb.callback_context);
        }
        desc = usb.interface->dev_descr;
        break;
    case USB_DTYPE_CONFIGURATION:
        desc = usb.interface->cfg_descr;
        len  = ((struct usb_string_descriptor*) (usb.interface->cfg_descr))->wString[0];
        break;
    case USB_DTYPE_STRING:
        if (dnumber == UsbDevLang)
        {
            desc = &dev_lang_desc;
        }
        else if ((dnumber == UsbDevManuf) && (usb.interface->str_manuf_descr != NULL))
        {
            desc = usb.interface->str_manuf_descr;
        }
        else if ((dnumber == UsbDevProduct) && (usb.interface->str_prod_descr != NULL))
        {
            desc = usb.interface->str_prod_descr;
        }
        else if ((dnumber == UsbDevSerial) && (usb.interface->str_serial_descr != NULL))
        {
            desc = usb.interface->str_serial_descr;
        }
        else
            return usbd_fail;
        break;
    default:
        return usbd_fail;
    }
    if (desc == NULL)
        return usbd_fail;

    if (len == 0)
    {
        len = ((struct usb_header_descriptor*) desc)->bLength;
    }
    *address = (void*) desc;
    *length  = len;
    return usbd_ack;
}

static void reset_evt(usbd_device* dev, uint8_t event, uint8_t ep)
{
    UNUSED(dev);
    UNUSED(event);
    UNUSED(ep);
    rhs_thread_flags_set(rhs_thread_get_id(usb.thread), UsbEventReset);
    if (usb.callback != NULL)
    {
        usb.callback(RHSHalUsbStateEventReset, usb.callback_context);
    }
}

static void susp_evt(usbd_device* dev, uint8_t event, uint8_t ep)
{
    UNUSED(dev);
    UNUSED(event);
    UNUSED(ep);
    if ((usb.interface != NULL) && (usb.connected == true))
    {
        usb.connected = false;
        usb.interface->suspend(&udev);

        // rhs_hal_power_insomnia_exit();
    }
    if (usb.callback != NULL)
    {
        usb.callback(RHSHalUsbStateEventSuspend, usb.callback_context);
    }
}

static void wkup_evt(usbd_device* dev, uint8_t event, uint8_t ep)
{
    UNUSED(dev);
    UNUSED(event);
    UNUSED(ep);
    if ((usb.interface != NULL) && (usb.connected == false))
    {
        usb.connected = true;
        usb.interface->wakeup(&udev);

        // rhs_hal_power_insomnia_enter();
    }
    if (usb.callback != NULL)
    {
        usb.callback(RHSHalUsbStateEventWakeup, usb.callback_context);
    }
}

static void usb_process_mode_start(RHSHalUsbInterface* interface, void* context)
{
    if (usb.interface != NULL)
    {
        usb.interface->deinit(&udev);
    }

    __disable_irq();
    usb.interface         = interface;
    usb.interface_context = context;
    __enable_irq();

    if (interface != NULL)
    {
        interface->init(&udev, interface, context);
        usbd_reg_event(&udev, usbd_evt_reset, reset_evt);
        RHS_LOG_I(TAG, "USB Mode change done");
        usb.enabled = true;
    }
}

static void usb_process_mode_change(RHSHalUsbInterface* interface, void* context)
{
    if ((interface != usb.interface) || (context != usb.interface_context))
    {
        if (usb.enabled)
        {
            // Disable current interface
            susp_evt(&udev, 0, 0);
            usbd_connect(&udev, false);
            usb.enabled = false;
            rhs_delay_ms(USB_RECONNECT_DELAY);
        }
        usb_process_mode_start(interface, context);
    }
}

static void usb_process_mode_reinit(void)
{
    // Temporary disable callback to avoid getting false reset events
    usbd_reg_event(&udev, usbd_evt_reset, NULL);
    RHS_LOG_I(TAG, "USB Reinit");
    susp_evt(&udev, 0, 0);
    usbd_connect(&udev, false);
    usb.enabled = false;

    taskENTER_CRITICAL();
    usbd_enable(&udev, false);
    usbd_enable(&udev, true);
    taskEXIT_CRITICAL();

    rhs_delay_ms(USB_RECONNECT_DELAY);
    usb_process_mode_start(usb.interface, usb.interface_context);
}

static bool usb_process_set_config(RHSHalUsbInterface* interface, void* context)
{
    if (usb.mode_lock)
    {
        return false;
    }
    else
    {
        usb_process_mode_change(interface, context);
        return true;
    }
}

static void usb_process_enable(bool enable)
{
    if (enable)
    {
        if ((!usb.enabled) && (usb.interface != NULL))
        {
            usbd_connect(&udev, true);
            usb.enabled = true;
            RHS_LOG_I(TAG, "USB Enable");
        }
    }
    else
    {
        if (usb.enabled)
        {
            susp_evt(&udev, 0, 0);
            usbd_connect(&udev, false);
            usb.enabled         = false;
            usb.request_pending = false;
            RHS_LOG_I(TAG, "USB Disable");
        }
    }
}

static void usb_process_message(UsbApiEventMessage* message)
{
    switch (message->type)
    {
    case UsbApiEventTypeSetConfig:
        message->return_data->bool_value =
            usb_process_set_config(message->data.interface.interface, message->data.interface.context);
        break;
    case UsbApiEventTypeGetConfig:
        message->return_data->void_value = usb.interface;
        break;
    case UsbApiEventTypeLock:
        RHS_LOG_I(TAG, "Mode lock");
        usb.mode_lock = true;
        break;
    case UsbApiEventTypeUnlock:
        RHS_LOG_I(TAG, "Mode unlock");
        usb.mode_lock = false;
        break;
    case UsbApiEventTypeIsLocked:
        message->return_data->bool_value = usb.mode_lock;
        break;
    case UsbApiEventTypeDisable:
        usb_process_enable(false);
        break;
    case UsbApiEventTypeEnable:
        usb_process_enable(true);
        break;
    case UsbApiEventTypeReinit:
        usb_process_mode_reinit();
        break;
    case UsbApiEventTypeSetStateCallback:
        usb.callback         = message->data.state_callback.callback;
        usb.callback_context = message->data.state_callback.context;
        break;
    }

    api_lock_unlock(message->lock);
}

static int32_t rhs_hal_usb_thread(void* context)
{
    UNUSED(context);
    uint8_t usb_wait_time = 0;

    if (rhs_message_queue_get_count(usb.queue) > 0)
    {
        rhs_thread_flags_set(rhs_thread_get_id(usb.thread), UsbEventMessage);
    }

    while (true)
    {
        uint32_t flags = rhs_thread_flags_wait(USB_SRV_ALL_EVENTS, RHSFlagWaitAny, 500);

        {
            UsbApiEventMessage message;
            if (rhs_message_queue_get(usb.queue, &message, 0) == RHSStatusOk)
            {
                usb_process_message(&message);
            }
        }

        if ((flags & RHSFlagError) == 0)
        {
            if (flags & UsbEventReset)
            {
                if (usb.enabled)
                {
                    usb.request_pending = true;
                    usb_wait_time       = 0;
                }
            }
            if (flags & UsbEventRequest)
            {
                usb.request_pending = false;
            }
        }
        else if (usb.request_pending)
        {
            usb_wait_time++;
            if (usb_wait_time > 4)
            {
                usb_process_mode_reinit();
                usb.request_pending = false;
            }
        }
    }
    return 0;
}
