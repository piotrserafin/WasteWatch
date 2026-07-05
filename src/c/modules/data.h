#pragma once

#include <pebble.h>

// Schedule entries
#define MAX_SCHEDULE 30
#define SCHEDULE_TYPE_LEN 32
#define SCHEDULE_DATE_LEN 16
#define SCHEDULE_DAY_LEN 16

typedef struct {
  char type[SCHEDULE_TYPE_LEN];
  char date[SCHEDULE_DATE_LEN];
  char day[SCHEDULE_DAY_LEN];
} ScheduleEntry;

void data_init(void);

int data_get_schedule_count(void);
void data_set_schedule_count(int count);
ScheduleEntry *data_get_schedule(int index);

bool data_is_loading(void);
void data_set_loading(bool loading);

time_t data_get_last_updated(void);
void data_set_last_updated(time_t t);

bool data_load_cache(void);
void data_save_cache(void);
