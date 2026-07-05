#pragma once

#include <pebble.h>

// Request types (must match JS side)
#define REQUEST_SCHEDULE 1

// Callbacks
typedef void (*MessagingScheduleReadyCallback)(void);
typedef void (*MessagingErrorCallback)(const char *error);
typedef void (*MessagingSettingsUpdatedCallback)(void);

void messaging_init(void);
void messaging_request_schedule(void);

void messaging_set_schedule_ready_callback(MessagingScheduleReadyCallback callback);
void messaging_set_error_callback(MessagingErrorCallback callback);
void messaging_set_settings_updated_callback(MessagingSettingsUpdatedCallback callback);
