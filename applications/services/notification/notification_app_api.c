#include <rhs_hal.h>
#include "notification.h"
#include "notification_app.h"
#include "rhs.h"

void notification_message(NotificationApp* app, const NotificationSequence* sequence)
{
    rhs_assert(sequence);

    NotificationAppMessage m = {.type = NotificationLayerMessage, .sequence = sequence};
    rhs_assert(rhs_message_queue_put(app->queue, &m, RHSWaitForever) == RHSStatusOk);
}
