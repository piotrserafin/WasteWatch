#pragma once

#include <pebble.h>

#define TOKEN_LEN 40
#define PROXY_URL_LEN 96
#define STREET_NAME_LEN 48
#define NUMBER_NAME_LEN 16

typedef struct {
  char token[TOKEN_LEN];
  char proxy_url[PROXY_URL_LEN];
  char street_name[STREET_NAME_LEN];
  char number_name[NUMBER_NAME_LEN];
} Settings;

void settings_init(void);
void settings_save(void);
Settings *settings_get(void);

bool settings_has_token(void);
bool settings_has_address(void);
