#pragma once
#include "notification.h"
#include "notification_messages_notes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************** Messages **********************************/

// Delay
extern const NotificationMessage message_delay_1;

extern const NotificationMessage message_delay_10;

extern const NotificationMessage message_delay_25;

extern const NotificationMessage message_delay_50;

extern const NotificationMessage message_delay_100;

extern const NotificationMessage message_delay_250;

extern const NotificationMessage message_delay_500;

extern const NotificationMessage message_delay_1000;

// Sound
extern const NotificationMessage message_sound_off;

// Reset
extern const NotificationMessage message_do_not_reset;

/****************************** Message sequences ******************************/
extern const NotificationSequence sequence_reset_sound;

extern const NotificationSequence sequence_success;
extern const NotificationSequence sequence_error;

#ifdef __cplusplus
}
#endif
