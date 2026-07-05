#include "data.h"

static ScheduleEntry s_schedule[MAX_SCHEDULE];
static int s_schedule_count = 0;
static bool s_loading = false;
static time_t s_last_updated = 0;

void data_init(void) {
  s_schedule_count = 0;
  s_loading = false;
  memset(s_schedule, 0, sizeof(s_schedule));
}

int data_get_schedule_count(void) {
  return s_schedule_count;
}

void data_set_schedule_count(int count) {
  if (count > MAX_SCHEDULE)
    count = MAX_SCHEDULE;
  s_schedule_count = count;
  memset(s_schedule, 0, sizeof(s_schedule));
}

ScheduleEntry *data_get_schedule(int index) {
  if (index < 0 || index >= s_schedule_count)
    return NULL;
  return &s_schedule[index];
}

bool data_is_loading(void) {
  return s_loading;
}

void data_set_loading(bool loading) {
  s_loading = loading;
}

time_t data_get_last_updated(void) {
  return s_last_updated;
}

void data_set_last_updated(time_t t) {
  s_last_updated = t;
}

#define CACHE_KEY_HEADER 2
#define CACHE_KEY_ENTRIES 3
#define CACHE_VERSION 1

bool data_load_cache(void) {
  if (!persist_exists(CACHE_KEY_HEADER))
    return false;
  struct {
    uint8_t version;
    uint8_t count;
    time_t last_updated;
  } header;
  if (persist_read_data(CACHE_KEY_HEADER, &header, sizeof(header)) != sizeof(header))
    return false;
  if (header.version != CACHE_VERSION)
    return false;
  if (header.count == 0 || header.count > MAX_SCHEDULE)
    return false;

  s_schedule_count = header.count;
  s_last_updated = header.last_updated;
  for (int i = 0; i < header.count; i++) {
    persist_read_data(CACHE_KEY_ENTRIES + i, &s_schedule[i], sizeof(ScheduleEntry));
  }
  return true;
}

void data_save_cache(void) {
  struct {
    uint8_t version;
    uint8_t count;
    time_t last_updated;
  } header = {
      .version = CACHE_VERSION,
      .count = (uint8_t)s_schedule_count,
      .last_updated = s_last_updated,
  };
  persist_write_data(CACHE_KEY_HEADER, &header, sizeof(header));
  for (int i = 0; i < s_schedule_count; i++) {
    persist_write_data(CACHE_KEY_ENTRIES + i, &s_schedule[i], sizeof(ScheduleEntry));
  }
}
