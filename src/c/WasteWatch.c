#include <pebble.h>

#include "modules/data.h"
#include "modules/messaging.h"
#include "modules/settings.h"
#include "windows/schedule_window.h"
#include "windows/status_window.h"

static char s_status_text[128];
static bool s_schedule_showing = false;

static void show_status(const char *text) {
  if (!s_schedule_showing) {
    snprintf(s_status_text, sizeof(s_status_text), "%s", text);
    status_window_set_text(s_status_text);
  }
}

static void on_schedule_ready(void) {
  if (!s_schedule_showing) {
    s_schedule_showing = true;
    schedule_window_push();
    status_window_remove();
  } else {
    schedule_window_reload();
  }
}

static void on_error(const char *error) {
  show_status(error);
}

static void on_settings_updated(void) {
  if (settings_has_token() && settings_has_address()) {
    show_status("Loading...");
  } else if (settings_has_token()) {
    show_status("Token saved.\n\nOpen Settings\nto set address.");
  } else {
    show_status("Open Settings\nin Pebble app\non your phone.");
  }
}

static void request_schedule_callback(void *data) {
  messaging_request_schedule();
}

static void timeout_callback(void *data) {
  if (data_is_loading()) {
    data_set_loading(false);
    show_status("Timeout.\nRestart app\nto retry.");
  }
}

static void init(void) {
  settings_init();
  data_init();
  messaging_init();

  messaging_set_schedule_ready_callback(on_schedule_ready);
  messaging_set_error_callback(on_error);
  messaging_set_settings_updated_callback(on_settings_updated);

  status_window_push();

  if (!settings_has_token()) {
    status_window_set_text("Open Settings\nin Pebble app\non your phone.");
  } else {
    if (data_load_cache()) {
      s_schedule_showing = true;
      schedule_window_push();
      status_window_remove();
    } else {
      status_window_set_text("Loading...");
    }
    app_timer_register(3000, request_schedule_callback, NULL);
    app_timer_register(30000, timeout_callback, NULL);
  }
}

int main(void) {
  init();
  app_event_loop();
}
