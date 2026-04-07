/**
 * Pebble Trivia
 * Category → Difficulty → Questions with streak counter and colours.
 *
 * STATE_QUESTION : UP/DOWN scroll   SELECT = reveal answer
 * STATE_ANSWER   : UP = correct     DOWN = missed   SELECT = skip
 * Back from trivia resets streak and returns to difficulty picker.
 * Back from difficulty returns to category picker.
 *
 * Difficulty encoding: message value = cat_id + difficulty * 200
 * (no valid cat_id == 1, so REQUEST_NEXT=1 is unambiguous)
 */

#include <pebble.h>

/* ── Message keys ──────────────────────────────────────────── */
#define MSG_KEY_CATEGORY     0
#define MSG_KEY_QUESTION     1
#define MSG_KEY_ANSWER       2
#define MSG_KEY_REQUEST_NEXT 3
#define REQUEST_NEXT         1   /* value=1 → next question */

#define SCROLL_STEP 30

/* ── Layout ────────────────────────────────────────────────── */
#ifdef PBL_ROUND
#  define LABEL_H  28
#  define HINT_H   24
#  define HPAD     20
#else
#  define LABEL_H  22
#  define HINT_H   20
#  define HPAD     0
#endif

/* ── Types ─────────────────────────────────────────────────── */
typedef enum { DIFF_ANY=0, DIFF_EASY=1, DIFF_MEDIUM=2, DIFF_HARD=3 } Difficulty;
typedef enum { STATE_LOADING, STATE_QUESTION, STATE_ANSWER } AppState;
typedef struct { const char *name; uint8_t id; } Category;

/* ── Category data ─────────────────────────────────────────── */
static const Category CATEGORIES[] = {
  { "All Topics",    0   },
  { "General",       9   },
  { "Geography",     22  },
  { "History",       23  },
  { "Science",       17  },
  { "Entertainment", 11  },
  { "Sports",        21  },
  { "Animals",       27  },
  { "True / False",  102 },
  { "Easy (Kids)",   101 },
};
#define NUM_CATEGORIES 10

#ifdef PBL_COLOR
/* ARGB8 background colour per category */
static const uint8_t CAT_BG[NUM_CATEGORIES] = {
  GColorCobaltBlueARGB8,      /* All Topics    */
  GColorSunsetOrangeARGB8,    /* General       */
  GColorJaegerGreenARGB8,     /* Geography     */
  GColorBulgarianRoseARGB8,   /* History       */
  GColorLibertyARGB8,         /* Science       */
  GColorMagentaARGB8,         /* Entertainment */
  GColorRedARGB8,             /* Sports        */
  GColorIslamicGreenARGB8,    /* Animals       */
  GColorVividCeruleanARGB8,   /* True / False  */
  GColorRajahARGB8,           /* Easy (Kids)   */
};
/* true → white text on that background */
static const bool CAT_WHITE_TXT[NUM_CATEGORIES] = {
  true, true, true, true, true, true, true, true, false, false
};
#endif /* PBL_COLOR */

/* ── Difficulty data ───────────────────────────────────────── */
static const char *DIFF_NAMES[] = {
  "Any Difficulty", "Easy", "Medium", "Hard"
};
#define NUM_DIFF 4

/* ── Pending / retry state ─────────────────────────────────── */
static uint8_t    s_cat_idx = 0;
static Difficulty s_diff    = DIFF_ANY;
static AppTimer  *s_retry   = NULL;

static void send_cat_now(void *unused);

static void schedule_retry(void) {
  if (s_retry) app_timer_cancel(s_retry);
  s_retry = app_timer_register(1000, send_cat_now, NULL);
}

static void send_cat_now(void *unused) {
  s_retry = NULL;
  DictionaryIterator *it;
  if (app_message_outbox_begin(&it) == APP_MSG_OK) {
    int32_t val = (int32_t)CATEGORIES[s_cat_idx].id + (int32_t)s_diff * 200;
    dict_write_int32(it, MSG_KEY_REQUEST_NEXT, val);
    app_message_outbox_send();
  } else {
    schedule_retry();
  }
}

static void outbox_failed(DictionaryIterator *it, AppMessageResult r, void *ctx) {
  schedule_retry();
}

static void outbox_sent(DictionaryIterator *it, void *ctx) {
  if (s_retry) { app_timer_cancel(s_retry); s_retry = NULL; }
}

/* ── Forward declarations ──────────────────────────────────── */
static void trivia_window_push(void);
static void diff_window_push(void);
static void update_display(void);

/* ══ Category menu window ══════════════════════════════════════ */
static Window    *s_cat_win;
static MenuLayer *s_cat_menu;

static uint16_t cat_n_sec(MenuLayer *l, void *c)                { return 1; }
static uint16_t cat_n_rows(MenuLayer *l, uint16_t s, void *c)   { return NUM_CATEGORIES; }
static int16_t  cat_cell_h(MenuLayer *l, MenuIndex *i, void *c) { return 36; }
static int16_t  cat_hdr_h(MenuLayer *l, uint16_t s, void *c)    { return 16; }

static void cat_draw_hdr(GContext *ctx, const Layer *cl,
                          uint16_t s, void *c) {
  menu_cell_basic_header_draw(ctx, cl, "Choose Category");
}

static void cat_draw_row(GContext *ctx, const Layer *cl,
                          MenuIndex *idx, void *c) {
#ifdef PBL_COLOR
  if (menu_cell_layer_is_highlighted(cl)) {
    GColor bg = (GColor){.argb = CAT_BG[idx->row]};
    GColor fg = CAT_WHITE_TXT[idx->row] ? GColorWhite : GColorBlack;
    GRect b = layer_get_bounds(cl);
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    graphics_context_set_text_color(ctx, fg);
    GRect tr = GRect(5, 5, b.size.w - 10, b.size.h - 5);
    graphics_draw_text(ctx, CATEGORIES[idx->row].name,
                       fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       tr, GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    return;
  }
#endif
  menu_cell_basic_draw(ctx, cl, CATEGORIES[idx->row].name, NULL, NULL);
}

static void cat_select(MenuLayer *l, MenuIndex *idx, void *c) {
  s_cat_idx = (uint8_t)idx->row;
  diff_window_push();
}

static void cat_win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_cat_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_cat_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections  = cat_n_sec,
    .get_num_rows      = cat_n_rows,
    .get_cell_height   = cat_cell_h,
    .get_header_height = cat_hdr_h,
    .draw_header       = cat_draw_hdr,
    .draw_row          = cat_draw_row,
    .select_click      = cat_select,
  });
  menu_layer_set_click_config_onto_window(s_cat_menu, w);
  layer_add_child(root, menu_layer_get_layer(s_cat_menu));
}

static void cat_win_unload(Window *w) {
  menu_layer_destroy(s_cat_menu);
}

/* ══ Difficulty menu window ════════════════════════════════════ */
static Window    *s_diff_win;
static MenuLayer *s_diff_menu;

static uint16_t diff_n_sec(MenuLayer *l, void *c)                { return 1; }
static uint16_t diff_n_rows(MenuLayer *l, uint16_t s, void *c)   { return NUM_DIFF; }
static int16_t  diff_cell_h(MenuLayer *l, MenuIndex *i, void *c) { return 36; }
static int16_t  diff_hdr_h(MenuLayer *l, uint16_t s, void *c)    { return 20; }

static void diff_draw_hdr(GContext *ctx, const Layer *cl,
                           uint16_t s, void *c) {
#ifdef PBL_COLOR
  GColor bg = (GColor){.argb = CAT_BG[s_cat_idx]};
  GColor fg = CAT_WHITE_TXT[s_cat_idx] ? GColorWhite : GColorBlack;
  GRect b = layer_get_bounds(cl);
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);
  GRect tr = GRect(4, 2, b.size.w - 8, b.size.h);
  graphics_draw_text(ctx, CATEGORIES[s_cat_idx].name,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     tr, GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
#else
  menu_cell_basic_header_draw(ctx, cl, CATEGORIES[s_cat_idx].name);
#endif
}

static void diff_draw_row(GContext *ctx, const Layer *cl,
                           MenuIndex *idx, void *c) {
  menu_cell_basic_draw(ctx, cl, DIFF_NAMES[idx->row], NULL, NULL);
}

static void diff_select(MenuLayer *l, MenuIndex *idx, void *c) {
  s_diff = (Difficulty)idx->row;
  trivia_window_push();
  if (s_retry) app_timer_cancel(s_retry);
  s_retry = app_timer_register(300, send_cat_now, NULL);
}

static void diff_win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_diff_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_diff_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections  = diff_n_sec,
    .get_num_rows      = diff_n_rows,
    .get_cell_height   = diff_cell_h,
    .get_header_height = diff_hdr_h,
    .draw_header       = diff_draw_hdr,
    .draw_row          = diff_draw_row,
    .select_click      = diff_select,
  });
  menu_layer_set_click_config_onto_window(s_diff_menu, w);
  layer_add_child(root, menu_layer_get_layer(s_diff_menu));
}

static void diff_win_unload(Window *w) {
  menu_layer_destroy(s_diff_menu);
}

static void diff_window_push(void) {
  window_stack_push(s_diff_win, true);
}

/* ══ Trivia window ═════════════════════════════════════════════ */
static Window      *s_trivia_win;
static TextLayer   *s_label_layer;
static ScrollLayer *s_scroll_layer;
static TextLayer   *s_main_layer;
static TextLayer   *s_hint_layer;

static AppState s_state  = STATE_LOADING;
static int      s_streak = 0;
static char     s_category[64];
static char     s_question[512];
static char     s_answer[256];
static char     s_label_buf[80];

static void apply_cat_color(void) {
#ifdef PBL_COLOR
  if (!s_label_layer) return;
  text_layer_set_background_color(s_label_layer,
    (GColor){.argb = CAT_BG[s_cat_idx]});
  text_layer_set_text_color(s_label_layer,
    CAT_WHITE_TXT[s_cat_idx] ? GColorWhite : GColorBlack);
#endif
}

static void set_scroll_text(const char *text) {
  if (!s_scroll_layer) return;
  GRect sf = layer_get_frame(scroll_layer_get_layer(s_scroll_layer));
  int cw = sf.size.w;
  text_layer_set_size(s_main_layer, GSize(cw, 2000));
  text_layer_set_text(s_main_layer, text);
  GSize ts = text_layer_get_content_size(s_main_layer);
  int ch = ts.h + 8;
  if (ch < sf.size.h) ch = sf.size.h;
  text_layer_set_size(s_main_layer, GSize(cw, ch));
  scroll_layer_set_content_size(s_scroll_layer, GSize(sf.size.w, ch));
  scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, 0), false);
}

static void update_display(void) {
  if (!s_scroll_layer) return;
  switch (s_state) {
    case STATE_LOADING:
      text_layer_set_background_color(s_label_layer, GColorBlack);
      text_layer_set_text_color(s_label_layer, GColorWhite);
      text_layer_set_text(s_label_layer, "TRIVIA");
      text_layer_set_font(s_main_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      set_scroll_text("Loading...");
      text_layer_set_text(s_hint_layer, "");
      break;

    case STATE_QUESTION:
      apply_cat_color();
      if (s_streak > 0)
        snprintf(s_label_buf, sizeof(s_label_buf),
                 "%s  x%d", s_category, s_streak);
      else
        snprintf(s_label_buf, sizeof(s_label_buf), "%s", s_category);
      text_layer_set_text(s_label_layer, s_label_buf);
      text_layer_set_font(s_main_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      set_scroll_text(s_question);
      text_layer_set_text(s_hint_layer, "SEL=reveal answer");
      break;

    case STATE_ANSWER:
      apply_cat_color();
      if (s_streak > 0)
        snprintf(s_label_buf, sizeof(s_label_buf),
                 "ANSWER  x%d", s_streak);
      else
        snprintf(s_label_buf, sizeof(s_label_buf), "ANSWER");
      text_layer_set_text(s_label_layer, s_label_buf);
      text_layer_set_font(s_main_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      set_scroll_text(s_answer);
      text_layer_set_text(s_hint_layer, "UP=correct  DN=missed");
      break;
  }
}

static void request_next(void) {
  DictionaryIterator *it;
  if (app_message_outbox_begin(&it) == APP_MSG_OK) {
    dict_write_uint8(it, MSG_KEY_REQUEST_NEXT, REQUEST_NEXT);
    app_message_outbox_send();
  }
  s_state = STATE_LOADING;
  update_display();
}

static void up_click(ClickRecognizerRef r, void *ctx) {
  if (s_state == STATE_ANSWER) {
    s_streak++;
    request_next();
  } else {
    GPoint off = scroll_layer_get_content_offset(s_scroll_layer);
    off.y += SCROLL_STEP;
    if (off.y > 0) off.y = 0;
    scroll_layer_set_content_offset(s_scroll_layer, off, true);
  }
}

static void down_click(ClickRecognizerRef r, void *ctx) {
  if (s_state == STATE_ANSWER) {
    s_streak = 0;
    request_next();
  } else {
    GPoint off = scroll_layer_get_content_offset(s_scroll_layer);
    GSize  cs  = scroll_layer_get_content_size(s_scroll_layer);
    GRect  fr  = layer_get_frame(scroll_layer_get_layer(s_scroll_layer));
    int min_y  = -(cs.h - fr.size.h);
    if (min_y > 0) min_y = 0;
    off.y -= SCROLL_STEP;
    if (off.y < min_y) off.y = min_y;
    scroll_layer_set_content_offset(s_scroll_layer, off, true);
  }
}

static void select_click(ClickRecognizerRef r, void *ctx) {
  switch (s_state) {
    case STATE_LOADING: break;
    case STATE_QUESTION:
      s_state = STATE_ANSWER;
      update_display();
      vibes_short_pulse();
      break;
    case STATE_ANSWER:
      request_next();   /* skip — no streak change */
      break;
  }
}

static void click_config(void *ctx) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP,   150, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

static void trivia_win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  GRect b = layer_get_bounds(root);
  int W = b.size.w, H = b.size.h;

#ifdef PBL_COLOR
  window_set_background_color(w, GColorWhite);
#endif

  /* Label bar */
  s_label_layer = text_layer_create(GRect(HPAD, 0, W - 2*HPAD, LABEL_H));
  text_layer_set_background_color(s_label_layer, GColorBlack);
  text_layer_set_text_color(s_label_layer, GColorWhite);
  text_layer_set_font(s_label_layer,
    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_label_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(root, text_layer_get_layer(s_label_layer));

  /* Scroll area */
  GRect sr = GRect(HPAD, LABEL_H + 2, W - 2*HPAD, H - LABEL_H - HINT_H - 4);
  s_scroll_layer = scroll_layer_create(sr);
  scroll_layer_set_shadow_hidden(s_scroll_layer, true);
  layer_add_child(root, scroll_layer_get_layer(s_scroll_layer));

  s_main_layer = text_layer_create(GRect(4, 2, sr.size.w - 8, sr.size.h));
  text_layer_set_background_color(s_main_layer, GColorClear);
  text_layer_set_text_color(s_main_layer, GColorBlack);
  text_layer_set_font(s_main_layer,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_overflow_mode(s_main_layer, GTextOverflowModeWordWrap);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_main_layer));

  /* Hint bar */
  s_hint_layer = text_layer_create(GRect(HPAD, H - HINT_H, W - 2*HPAD, HINT_H));
  text_layer_set_background_color(s_hint_layer, GColorBlack);
  text_layer_set_text_color(s_hint_layer, GColorWhite);
  text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_hint_layer));

  s_state = STATE_LOADING;
  update_display();
}

static void trivia_win_unload(Window *w) {
  text_layer_destroy(s_label_layer);
  text_layer_destroy(s_main_layer);
  scroll_layer_destroy(s_scroll_layer);
  text_layer_destroy(s_hint_layer);
  s_scroll_layer = NULL;
  s_main_layer   = NULL;
  s_label_layer  = NULL;
  s_hint_layer   = NULL;
  /* Reset state so next game starts clean */
  s_state  = STATE_LOADING;
  s_streak = 0;
  if (s_retry) { app_timer_cancel(s_retry); s_retry = NULL; }
}

static void trivia_window_push(void) {
  window_stack_push(s_trivia_win, true);
}

/* ── AppMessage ──────────────────────────────────────────────── */
static void inbox_received(DictionaryIterator *it, void *ctx) {
  Tuple *cat_t = dict_find(it, MSG_KEY_CATEGORY);
  Tuple *q_t   = dict_find(it, MSG_KEY_QUESTION);
  Tuple *ans_t = dict_find(it, MSG_KEY_ANSWER);

  if (cat_t)  strncpy(s_category, cat_t->value->cstring, sizeof(s_category) - 1);
  if (q_t)    strncpy(s_question, q_t->value->cstring,   sizeof(s_question) - 1);
  if (ans_t)  strncpy(s_answer,   ans_t->value->cstring, sizeof(s_answer)   - 1);

  if (q_t) {
    s_state = STATE_QUESTION;
    update_display();
  }
}

static void inbox_dropped(AppMessageResult reason, void *ctx) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Msg dropped: %d", (int)reason);
}

/* ── Init / deinit ───────────────────────────────────────────── */
static void init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_register_outbox_sent(outbox_sent);
  app_message_register_outbox_failed(outbox_failed);
  app_message_open(512, 64);

  s_trivia_win = window_create();
  window_set_window_handlers(s_trivia_win, (WindowHandlers){
    .load   = trivia_win_load,
    .unload = trivia_win_unload,
  });
  window_set_click_config_provider(s_trivia_win, click_config);

  s_diff_win = window_create();
  window_set_window_handlers(s_diff_win, (WindowHandlers){
    .load   = diff_win_load,
    .unload = diff_win_unload,
  });

  s_cat_win = window_create();
  window_set_window_handlers(s_cat_win, (WindowHandlers){
    .load   = cat_win_load,
    .unload = cat_win_unload,
  });
  window_stack_push(s_cat_win, true);
}

static void deinit(void) {
  if (s_retry) app_timer_cancel(s_retry);
  window_destroy(s_trivia_win);
  window_destroy(s_diff_win);
  window_destroy(s_cat_win);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
