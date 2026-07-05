#include "messaging.h"

#include "data.h"
#include "settings.h"

static MessagingScheduleReadyCallback s_schedule_ready_callback;
static MessagingErrorCallback s_error_callback;
static MessagingSettingsUpdatedCallback s_settings_updated_callback;

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *token_tuple = dict_find(iter, MESSAGE_KEY_Token);

  if (token_tuple && !dict_find(iter, MESSAGE_KEY_ScheduleCount) &&
      !dict_find(iter, MESSAGE_KEY_ScheduleIndex)) {
    Settings *settings = settings_get();

    strncpy(settings->token, token_tuple->value->cstring, TOKEN_LEN - 1);
    settings->token[TOKEN_LEN - 1] = '\0';

    Tuple *t = dict_find(iter, MESSAGE_KEY_ProxyUrl);
    if (t) {
      strncpy(settings->proxy_url, t->value->cstring, PROXY_URL_LEN - 1);
      settings->proxy_url[PROXY_URL_LEN - 1] = '\0';
    }

    t = dict_find(iter, MESSAGE_KEY_StreetName);
    if (t) {
      strncpy(settings->street_name, t->value->cstring, STREET_NAME_LEN - 1);
      settings->street_name[STREET_NAME_LEN - 1] = '\0';
    }

    t = dict_find(iter, MESSAGE_KEY_NumberName);
    if (t) {
      strncpy(settings->number_name, t->value->cstring, NUMBER_NAME_LEN - 1);
      settings->number_name[NUMBER_NAME_LEN - 1] = '\0';
    }

    settings_save();
    if (s_settings_updated_callback)
      s_settings_updated_callback();
    return;
  }

  Tuple *error_tuple = dict_find(iter, MESSAGE_KEY_ErrorMessage);
  if (error_tuple) {
    data_set_loading(false);
    if (s_error_callback)
      s_error_callback(error_tuple->value->cstring);
    return;
  }

  Tuple *count_tuple = dict_find(iter, MESSAGE_KEY_ScheduleCount);
  if (count_tuple) {
    int count = count_tuple->value->int32;
    data_set_schedule_count(count);
    if (count == 0) {
      data_set_loading(false);
      if (s_error_callback)
        s_error_callback("Empty schedule");
    }
    return;
  }

  Tuple *idx_tuple = dict_find(iter, MESSAGE_KEY_ScheduleIndex);
  if (idx_tuple) {
    int idx = idx_tuple->value->int32;
    ScheduleEntry *entry = data_get_schedule(idx);
    if (!entry)
      return;

    Tuple *t = dict_find(iter, MESSAGE_KEY_ScheduleType);
    if (t) {
      strncpy(entry->type, t->value->cstring, SCHEDULE_TYPE_LEN - 1);
      entry->type[SCHEDULE_TYPE_LEN - 1] = '\0';
    }

    t = dict_find(iter, MESSAGE_KEY_ScheduleDate);
    if (t) {
      strncpy(entry->date, t->value->cstring, SCHEDULE_DATE_LEN - 1);
      entry->date[SCHEDULE_DATE_LEN - 1] = '\0';
    }

    t = dict_find(iter, MESSAGE_KEY_ScheduleDay);
    if (t) {
      strncpy(entry->day, t->value->cstring, SCHEDULE_DAY_LEN - 1);
      entry->day[SCHEDULE_DAY_LEN - 1] = '\0';
    }

    if (idx == data_get_schedule_count() - 1) {
      data_set_loading(false);
      data_set_last_updated(time(NULL));
      data_save_cache();
      if (s_schedule_ready_callback)
        s_schedule_ready_callback();
    }
    return;
  }
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason,
                                  void *context) {
  data_set_loading(false);
  if (s_error_callback)
    s_error_callback("Send failed");
}

void messaging_init(void) {
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(1024, 512);
}

void messaging_request_schedule(void) {
  if (data_is_loading())
    return;

  if (!settings_has_token()) {
    if (s_error_callback)
      s_error_callback("No token.\nOpen Settings.");
    return;
  }
  if (!settings_has_address()) {
    if (s_error_callback)
      s_error_callback("No address.\nOpen Settings.");
    return;
  }

  data_set_loading(true);

  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) != APP_MSG_OK) {
    data_set_loading(false);
    if (s_error_callback)
      s_error_callback("Msg error");
    return;
  }

  Settings *settings = settings_get();
  dict_write_int32(out, MESSAGE_KEY_RequestType, REQUEST_SCHEDULE);
  dict_write_cstring(out, MESSAGE_KEY_Token, settings->token);
  if (settings->proxy_url[0] != '\0')
    dict_write_cstring(out, MESSAGE_KEY_ProxyUrl, settings->proxy_url);
  dict_write_cstring(out, MESSAGE_KEY_StreetName, settings->street_name);
  dict_write_cstring(out, MESSAGE_KEY_NumberName, settings->number_name);

  if (app_message_outbox_send() != APP_MSG_OK) {
    data_set_loading(false);
    if (s_error_callback)
      s_error_callback("Send error");
  }
}

void messaging_set_schedule_ready_callback(MessagingScheduleReadyCallback cb) {
  s_schedule_ready_callback = cb;
}
void messaging_set_error_callback(MessagingErrorCallback cb) {
  s_error_callback = cb;
}
void messaging_set_settings_updated_callback(MessagingSettingsUpdatedCallback cb) {
  s_settings_updated_callback = cb;
}
