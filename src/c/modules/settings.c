#include "settings.h"

#define SETTINGS_KEY 1

static Settings s_settings;

void settings_init(void) {
  memset(&s_settings, 0, sizeof(s_settings));
  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  }
}

void settings_save(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

Settings *settings_get(void) {
  return &s_settings;
}

bool settings_has_token(void) {
  return s_settings.token[0] != '\0';
}

bool settings_has_address(void) {
  return s_settings.street_name[0] != '\0' && s_settings.number_name[0] != '\0';
}
