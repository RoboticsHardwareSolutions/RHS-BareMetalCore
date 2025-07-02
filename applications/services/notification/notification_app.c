#include "notification.h"
#include "notification_app.h"
#include "notification_messages.h"
#include "rhs.h"
#include "rhs_hal.h"

// App alloc
static NotificationApp* notification_app_alloc(void)
{
    NotificationApp* app = malloc(sizeof(NotificationApp));
    app->queue           = rhs_message_queue_alloc(8, sizeof(NotificationAppMessage));
    return app;
}

static void notification_app_free(NotificationApp* app)
{
    rhs_message_queue_free(app->queue);
    free(app);
}

static void notification_sound_on(float freq, float volume)
{
    if (rhs_hal_speaker_is_mine() || rhs_hal_speaker_acquire(30))
    {
        rhs_hal_speaker_start(freq, volume);
    }
}

static void notification_sound_off(void)
{
    if (rhs_hal_speaker_is_mine())
    {
        rhs_hal_speaker_release();
    }
}

// message processing
static void notification_process_notification_message(NotificationApp* app, NotificationAppMessage* message)
{
    uint32_t                   notification_message_index = 0;
    const NotificationMessage* notification_message;
    notification_message = (*message->sequence)[notification_message_index];
    while (notification_message != NULL)
    {
        switch (notification_message->type)
        {
        case NotificationMessageTypeSoundOn:
            notification_sound_on(notification_message->data.sound.frequency, notification_message->data.sound.volume);
            break;
        case NotificationMessageTypeSoundOff:
            notification_sound_off();
            break;
        case NotificationMessageTypeDelay:
            rhs_delay_ms(notification_message->data.delay.length);
            break;
        case NotificationMessageTypeDoNotReset:
            break;
        }
        notification_message_index++;
        notification_message = (*message->sequence)[notification_message_index];
    };
}

int32_t notification_srv(void* context)
{
    NotificationApp* app = notification_app_alloc();

    notification_message(app, &sequence_success);
    notification_sound_off();
    rhs_record_create(RECORD_NOTIFY, app);

    NotificationAppMessage message;

    while (1)
    {
        rhs_assert(rhs_message_queue_get(app->queue, &message, RHSWaitForever) == RHSStatusOk);

        switch (message.type)
        {
        case NotificationLayerMessage:
            notification_process_notification_message(app, &message);
            break;
        }
        // TODO block Feature
    }
    notification_app_free(app);
}
