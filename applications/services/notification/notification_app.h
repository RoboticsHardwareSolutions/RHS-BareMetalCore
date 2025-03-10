#include "stdint.h"
#include "rhs.h"

typedef enum {
    NotificationLayerMessage,
} NotificationAppMessageType;

typedef struct {
    const NotificationSequence* sequence;
    NotificationAppMessageType type;
} NotificationAppMessage;

struct NotificationApp {
    RHSMessageQueue* queue;
};
