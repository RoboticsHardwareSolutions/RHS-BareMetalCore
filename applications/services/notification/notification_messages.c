#include "notification.h"
#include "notification_messages_notes.h"
#include <stddef.h>

/*********************************** Messages **********************************/

// Reset
const NotificationMessage message_do_not_reset = {
    .type = NotificationMessageTypeDoNotReset,
};

// Delay
const NotificationMessage message_delay_1 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 1,
};

const NotificationMessage message_delay_10 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 10,
};

const NotificationMessage message_delay_25 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 25,
};

const NotificationMessage message_delay_50 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 50,
};

const NotificationMessage message_delay_100 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 100,
};

const NotificationMessage message_delay_250 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 250,
};

const NotificationMessage message_delay_500 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 500,
};

const NotificationMessage message_delay_1000 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 1000,
};

const NotificationMessage message_delay_150 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 150,
};

const NotificationMessage message_delay_350 = {
    .type = NotificationMessageTypeDelay,
    .data.delay.length = 350,
};

// Sound
const NotificationMessage message_sound_off = {
    .type = NotificationMessageTypeSoundOff,
};

/****************************** Message sequences ******************************/

const NotificationSequence sequence_reset_sound = {
    &message_sound_off,
    NULL,
};

const NotificationSequence sequence_success = {
    &message_note_c5,
    &message_delay_50,
    &message_note_e5,
    &message_delay_50,
    &message_note_g5,
    &message_delay_50,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

const NotificationSequence sequence_error = {
    &message_note_c5,
    &message_delay_100,
    &message_sound_off,
    &message_delay_100,
    &message_note_c5,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

const NotificationSequence sequence_imperial_march = {
    // First phrase: G-G-G-Eb-Bb-G-Eb-Bb-G
    &message_note_g4,
    &message_delay_500,
    &message_sound_off,
    &message_delay_50,
    &message_note_g4,
    &message_delay_500,
    &message_sound_off,
    &message_delay_50,
    &message_note_g4,
    &message_delay_500,
    &message_note_ds4,
    &message_delay_350,
    &message_note_as4,
    &message_delay_150,
    &message_note_g4,
    &message_delay_500,
    &message_note_ds4,
    &message_delay_350,
    &message_note_as4,
    &message_delay_150,
    &message_note_g4,
    &message_delay_1000,
    
    // Second phrase: D-D-D-Eb-Bb-F#-Eb-Bb-G
    &message_note_d5,
    &message_delay_500,
    &message_sound_off,
    &message_delay_50,
    &message_note_d5,
    &message_delay_500,
    &message_sound_off,
    &message_delay_50,
    &message_note_d5,
    &message_delay_500,
    &message_note_ds5,
    &message_delay_350,
    &message_note_as4,
    &message_delay_150,
    &message_note_fs4,
    &message_delay_500,
    &message_note_ds4,
    &message_delay_350,
    &message_note_as4,
    &message_delay_150,
    &message_note_g4,
    &message_delay_1000,
    
    &message_sound_off,
    NULL,
};
