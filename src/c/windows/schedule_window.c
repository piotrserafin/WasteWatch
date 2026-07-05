#include "schedule_window.h"

#include "../modules/data.h"
#include "../modules/settings.h"

#define HEADER_HEIGHT 52
#define CELL_HEIGHT 52
#define PADDING 4
#define LINE_HEIGHT 26
#define DARK_THRESHOLD 4

static Window *s_window;
static MenuLayer *s_menu_layer;
static char s_header_text[96];

#ifdef PBL_COLOR
static GColor color_for_type(const char *type) {
  if (strstr(type, "Bio"))
    return GColorWindsorTan;
  if (strstr(type, "Papier"))
    return GColorBlue;
  if (strstr(type, "Plastik") || strstr(type, "Metal"))
    return GColorYellow;
  if (strstr(type, "Szk"))
    return GColorGreen;
  if (strstr(type, "Zielone"))
    return GColorLightGray;
  if (strstr(type, "Zmiesz"))
    return GColorDarkGray;
  return GColorWhite;
}
#endif

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  int count = data_get_schedule_count();
  return count > 0 ? (uint16_t)count : 1;
}

static int16_t menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return HEADER_HEIGHT;
}

static void menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index,
                             void *data) {
  Settings *settings = settings_get();
  GRect bounds = layer_get_bounds(cell_layer);

#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
#endif

  // Address
  snprintf(s_header_text, sizeof(s_header_text), "%s %s", settings->street_name,
           settings->number_name);
  graphics_draw_text(ctx, s_header_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(PADDING, 0, bounds.size.w - PADDING * 2, LINE_HEIGHT + 2),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Updated at
  time_t updated = data_get_last_updated();
  if (updated) {
    struct tm *t = localtime(&updated);
    char time_str[24];
    snprintf(time_str, sizeof(time_str), "Pobrano: %d.%02d %02d:%02d", t->tm_mday, t->tm_mon + 1,
             t->tm_hour, t->tm_min);
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, GColorLightGray);
#endif
    graphics_draw_text(ctx, time_str, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(PADDING, LINE_HEIGHT + 2, bounds.size.w - PADDING * 2, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                          void *data) {
  if (data_get_schedule_count() == 0) {
    menu_cell_basic_draw(ctx, cell_layer, "No data", NULL, NULL);
    return;
  }

  ScheduleEntry *entry = data_get_schedule(cell_index->row);
  if (!entry)
    return;

  char title[36];
  snprintf(title, sizeof(title), "%s %s", entry->date, entry->day);

#ifdef PBL_COLOR
  GColor bg = color_for_type(entry->type);
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, layer_get_bounds(cell_layer), 0, GCornerNone);

  bool dark_bg = (bg.r + bg.g + bg.b) < DARK_THRESHOLD;
  graphics_context_set_text_color(ctx, dark_bg ? GColorWhite : GColorBlack);

  GRect bounds = layer_get_bounds(cell_layer);
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(PADDING, 0, bounds.size.w - PADDING * 2, LINE_HEIGHT),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, entry->type, fonts_get_system_font(FONT_KEY_GOTHIC_24),
                     GRect(PADDING, LINE_HEIGHT - 2, bounds.size.w - PADDING * 2, LINE_HEIGHT),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
#else
  menu_cell_basic_draw(ctx, cell_layer, title, entry->type, NULL);
#endif
}

static int16_t menu_get_cell_height(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return CELL_HEIGHT;
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL,
                           (MenuLayerCallbacks){
                               .get_num_rows = menu_get_num_rows,
                               .get_header_height = menu_get_header_height,
                               .draw_header = menu_draw_header,
                               .draw_row = menu_draw_row,
                               .get_cell_height = menu_get_cell_height,
                           });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
#ifdef PBL_COLOR
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
#endif
  layer_add_child(root, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void schedule_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  window_stack_push(s_window, true);
}

void schedule_window_reload(void) {
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}
