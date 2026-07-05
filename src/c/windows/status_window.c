#include "status_window.h"

static Window *s_window;
static TextLayer *s_text_layer;

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_text_layer = text_layer_create(GRect(10, 20, bounds.size.w - 20, bounds.size.h - 40));
  text_layer_set_text(s_text_layer, "WasteWatch");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void status_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  window_stack_push(s_window, true);
}

void status_window_set_text(const char *text) {
  if (s_text_layer) {
    text_layer_set_text(s_text_layer, text);
  }
}

void status_window_remove(void) {
  if (s_window) {
    window_stack_remove(s_window, false);
  }
}
