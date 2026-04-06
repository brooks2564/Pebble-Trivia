/**
 * Pebble Trivia
 *
 * UP/DOWN — scroll question or answer text
 * SELECT  — reveal answer / load next question
 *
 * Questions provided by Open Trivia Database (opentdb.com) — free, no API key.
 */

#include <pebble.h>

// Message keys — must match package.json
#define MSG_KEY_CATEGORY     0
#define MSG_KEY_QUESTION     1
#define MSG_KEY_ANSWER       2
#define MSG_KEY_REQUEST_NEXT 3

#define SCROLL_STEP 30

typedef enum {
  STATE_LOADING,
  STATE_QUESTION,
  STATE_ANSWER,
} AppState;

static Window      *s_window;
static TextLayer   *s_label_layer;   // top bar: category or "ANSWER"
static ScrollLayer *s_scroll_layer;  // scrollable content area
static TextLayer   *s_main_layer;    // question or answer text (inside scroll)
static TextLayer   *s_hint_layer;    // bottom bar: button hint

static AppState s_state = STATE_LOADING;

static char s_category[64];
static char s_question[512];
static char s_answer[256];

// ── Scroll Helpers ─────────────────────────────────────────────

static void set_scroll_text(const char *text) {
  GRect scroll_frame = layer_get_frame(scroll_layer_get_layer(s_scroll_layer));
  int content_w = scroll_frame.size.w - 8;

  // Size the TextLayer tall enough to measure full content
  text_layer_set_size(s_main_layer, GSize(content_w, 2000));
  text_layer_set_text(s_main_layer, text);

  // Measure actual height needed
  GSize text_size = text_layer_get_content_size(s_main_layer);
  int content_h = text_size.h + 8;
  if (content_h < scroll_frame.size.h) content_h = scroll_frame.size.h;

  // Resize and update scroll content size
  text_layer_set_size(s_main_layer, GSize(content_w, content_h));
  scroll_layer_set_content_size(s_scroll_layer, GSize(scroll_frame.size.w, content_h));

  // Always reset to top
  scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, 0), false);
}

// ── Display ────────────────────────────────────────────────────

static void update_display(void) {
  switch (s_state) {
    case STATE_LOADING:
      text_layer_set_text(s_label_layer, "TRIVIA");
      set_scroll_text("Loading questions...");
      text_layer_set_text(s_hint_layer, "");
      break;
    case STATE_QUESTION:
      text_layer_set_text(s_label_layer, s_category);
      set_scroll_text(s_question);
      text_layer_set_text(s_hint_layer, "UP/DN scroll  SEL=answer");
      break;
    case STATE_ANSWER:
      text_layer_set_text(s_label_layer, "ANSWER");
      set_scroll_text(s_answer);
      text_layer_set_text(s_hint_layer, "SELECT > next question");
      break;
  }
}

// ── Button Handlers ────────────────────────────────────────────

static void up_click(ClickRecognizerRef recognizer, void *context) {
  GPoint offset = scroll_layer_get_content_offset(s_scroll_layer);
  offset.y += SCROLL_STEP;
  if (offset.y > 0) offset.y = 0;
  scroll_layer_set_content_offset(s_scroll_layer, offset, true);
}

static void down_click(ClickRecognizerRef recognizer, void *context) {
  GPoint offset = scroll_layer_get_content_offset(s_scroll_layer);
  GSize content_size = scroll_layer_get_content_size(s_scroll_layer);
  GRect frame = layer_get_frame(scroll_layer_get_layer(s_scroll_layer));
  int min_y = -(content_size.h - frame.size.h);
  if (min_y > 0) min_y = 0;
  offset.y -= SCROLL_STEP;
  if (offset.y < min_y) offset.y = min_y;
  scroll_layer_set_content_offset(s_scroll_layer, offset, true);
}

static void request_next(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MSG_KEY_REQUEST_NEXT, 1);
    app_message_outbox_send();
  }
  s_state = STATE_LOADING;
  update_display();
}

static void select_click(ClickRecognizerRef recognizer, void *context) {
  switch (s_state) {
    case STATE_LOADING:
      break;
    case STATE_QUESTION:
      s_state = STATE_ANSWER;
      update_display();
      vibes_short_pulse();
      break;
    case STATE_ANSWER:
      request_next();
      break;
  }
}

static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP,   150, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

// ── AppMessage ────────────────────────────────────────────────

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *cat_t  = dict_find(iter, MSG_KEY_CATEGORY);
  Tuple *q_t    = dict_find(iter, MSG_KEY_QUESTION);
  Tuple *ans_t  = dict_find(iter, MSG_KEY_ANSWER);

  if (cat_t)  strncpy(s_category, cat_t->value->cstring,  sizeof(s_category) - 1);
  if (q_t)    strncpy(s_question, q_t->value->cstring,    sizeof(s_question) - 1);
  if (ans_t)  strncpy(s_answer,   ans_t->value->cstring,  sizeof(s_answer) - 1);

  if (q_t) {
    s_state = STATE_QUESTION;
    update_display();
  }
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

// ── Window Lifecycle ───────────────────────────────────────────

static void window_load(Window *window) {
  Layer *root  = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int w = bounds.size.w;
  int h = bounds.size.h;

#ifdef PBL_COLOR
  window_set_background_color(window, GColorWhite);
#endif

  // Top label bar
  s_label_layer = text_layer_create(GRect(0, 0, w, 22));
  text_layer_set_background_color(s_label_layer, GColorBlack);
  text_layer_set_text_color(s_label_layer, GColorWhite);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_label_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(root, text_layer_get_layer(s_label_layer));

  // Scrollable content area
  GRect scroll_rect = GRect(0, 24, w, h - 44);
  s_scroll_layer = scroll_layer_create(scroll_rect);
  scroll_layer_set_shadow_hidden(s_scroll_layer, true);
  layer_add_child(root, scroll_layer_get_layer(s_scroll_layer));

  // TextLayer inside the scroll layer
  s_main_layer = text_layer_create(GRect(4, 2, w - 8, scroll_rect.size.h));
  text_layer_set_background_color(s_main_layer, GColorClear);
  text_layer_set_text_color(s_main_layer, GColorBlack);
  text_layer_set_font(s_main_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_overflow_mode(s_main_layer, GTextOverflowModeWordWrap);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_main_layer));

  // Bottom hint bar
  s_hint_layer = text_layer_create(GRect(0, h - 20, w, 20));
  text_layer_set_background_color(s_hint_layer, GColorBlack);
  text_layer_set_text_color(s_hint_layer, GColorWhite);
  text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_hint_layer));

  update_display();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_label_layer);
  text_layer_destroy(s_main_layer);
  scroll_layer_destroy(s_scroll_layer);
  text_layer_destroy(s_hint_layer);
}

// ── Main ──────────────────────────────────────────────────────

static void init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_open(512, 64);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config_provider);
  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
