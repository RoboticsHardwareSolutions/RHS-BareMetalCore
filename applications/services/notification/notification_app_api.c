#include <rhs_hal.h>
#include "notification.h"
#include "notification_app.h"
#include "rhs.h"

extern NotificationApp* notification_app;

void notification_message(const NotificationSequence* sequence) {
    rhs_assert(notification_app);
    rhs_assert(sequence);

    NotificationAppMessage m = {
        .type = NotificationLayerMessage, .sequence = sequence};
    rhs_assert(rhs_message_queue_put(notification_app->queue, &m, RHSWaitForever) == RHSStatusOk);
}
