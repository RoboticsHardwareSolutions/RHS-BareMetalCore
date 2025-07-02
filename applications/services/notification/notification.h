#pragma once
#include "stdint.h"

#define RECORD_NOTIFY "notify"

typedef struct NotificationApp NotificationApp;

typedef struct
{
    float frequency;
    float volume;
} NotificationMessageDataSound;

typedef struct
{
    uint32_t length;
} NotificationMessageDataDelay;

typedef union
{
    NotificationMessageDataSound sound;
    NotificationMessageDataDelay delay;
} NotificationMessageData;

typedef enum
{
    NotificationMessageTypeSoundOn,
    NotificationMessageTypeSoundOff,

    NotificationMessageTypeDelay,

    NotificationMessageTypeDoNotReset,
} NotificationMessageType;

typedef struct
{
    NotificationMessageType type;
    NotificationMessageData data;
} NotificationMessage;

typedef const NotificationMessage* NotificationSequence[];

void notification_message(NotificationApp* app, const NotificationSequence* sequence);
