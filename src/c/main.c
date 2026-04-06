/**
 * Pebble Trivia
 *
 * Press SELECT to reveal the answer.
 * Press SELECT again to load the next question.
 *
 * Questions provided by Open Trivia Database (opentdb.com) — free, no API key.
 */

#include <pebble.h>

// Message keys — must match package.json
#define MSG_KEY_CATEGORY     0
#define MSG_KEY_QUESTION     1
#define MSG_KEY_ANSWER       2
#define MSG_KEY_REQUEST_NEXT 3

typedef enum {
  STATE_LOADING,
  STATE_QUESTION,
  STATE_ANSWER,
} AppState;

static Window    *s_window;
static TextLayer *s_label_layer;   // top bar: category or "ANSWER"
static TextLayer *s_main_layer;    // question or answer text
static TextLayer *s_hint_layer;    // bottom bar: button hint

static AppState s_state = STATE_LOADING;

static char s_category[64];
static char s_question[512];
static char s_answer[256];

// ── Display ────────────────────────────────────────────────────

static void update_display(void) {
  switch (s_state) {
    case STATE_LOADING:
      text_layer_set_text(s_label_layer, "TRIVIA");
      text_layer_set_text(s_main_layer, "Loading questions...");
      text_layer_set_text(s_hint_layer, "");
      break;
    case STATE_QUESTION:
      text_layer_set_text(s_label_layer, s_category);
      text_layer_set_text(s_main_layer, s_question);
      text_layer_set_text(s_hint_layer, "SELECT > reveal answer");
      break;
    case STATE_ANSWER:
      text_layer_set_text(s_label_layer, "ANSWER");
      text_layer_set_text(s_main_layer, s_answer);
      text_layer_set_text(s_hint_layer, "SELECT > next question");
      break;
  }
}

// ── Button Handlers ────────────────────────────────────────────

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
  Layer *root   = window_get_root_layer(window);
  GRect bounds  = layer_get_bounds(root);
  int w         = bounds.size.w;
  int h         = bounds.size.h;

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

  // Main question/answer area
  s_main_layer = text_layer_create(GRect(4, 26, w - 8, h - 48));
  text_layer_set_background_color(s_main_layer, GColorClear);
  text_layer_set_text_color(s_main_layer, GColorBlack);
  text_layer_set_font(s_main_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_overflow_mode(s_main_layer, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_main_layer));

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
