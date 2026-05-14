/**
 * Pebble Trivia
 * Category → Difficulty → Questions with automatic answer selection and scoring.
 *
 * STATE_SELECTING: UP/DN move choice highlight, SELECT/TAP confirm
 * STATE_RESULT   : auto-advances after 2s (green=correct, red=wrong)
 * Back from trivia resets score and returns to difficulty picker.
 *
 * Difficulty encoding: message value = cat_id + difficulty * 200
 * (no valid cat_id == 1, so REQUEST_NEXT=1 is unambiguous)
 *
 * Touch (emery/gabbro only):
 *   Menus:   swipe up/down moves highlight, tap = select
 *   Trivia:  swipe up/down moves choice highlight, tap = confirm
 */

#include <pebble.h>

/* ── Message keys ──────────────────────────────────────────── */
#define MSG_KEY_CATEGORY     0
#define MSG_KEY_QUESTION     1
#define MSG_KEY_ANSWER       2   /* sent by JS, unused by C (kept for compat) */
#define MSG_KEY_REQUEST_NEXT 3
#define MSG_KEY_CORRECT_IDX  4
#define REQUEST_NEXT         1

/* Touch guard — emery and gabbro only */
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define HAS_TOUCHSCREEN
#endif

#define SCROLL_STEP      30
#define SWIPE_THRESHOLD  20

/* ── Layout ────────────────────────────────────────────────── */
#ifdef PBL_ROUND
#  define LABEL_H    28
#  define HINT_H     24
#  define HPAD       20
#  define CHOICES_H  104  /* 4 rows × 26px, GOTHIC_18_BOLD */
#else
#  define LABEL_H    22
#  define HINT_H     20
#  define HPAD       0
#  define CHOICES_H  104  /* 4 rows × 26px, GOTHIC_18_BOLD */
#endif

/* ── Types ─────────────────────────────────────────────────── */
typedef enum { DIFF_ANY=0, DIFF_EASY=1, DIFF_MEDIUM=2, DIFF_HARD=3 } Difficulty;
typedef enum { STATE_LOADING, STATE_SELECTING, STATE_RESULT } AppState;
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
static const uint8_t CAT_BG[NUM_CATEGORIES] = {
  GColorCobaltBlueARGB8,
  GColorSunsetOrangeARGB8,
  GColorJaegerGreenARGB8,
  GColorBulgarianRoseARGB8,
  GColorLibertyARGB8,
  GColorMagentaARGB8,
  GColorRedARGB8,
  GColorIslamicGreenARGB8,
  GColorVividCeruleanARGB8,
  GColorRajahARGB8,
};
static const bool CAT_WHITE_TXT[NUM_CATEGORIES] = {
  true, true, true, true, true, true, true, true, false, false
};
#endif

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
static void request_next(void);

/* ── Shared touch state ────────────────────────────────────── */
#ifdef HAS_TOUCHSCREEN
static GPoint s_touch_start;
static bool   s_touch_active = false;
#endif

/* ══ Category menu window ══════════════════════════════════════ */
static Window    *s_cat_win;
static MenuLayer *s_cat_menu;

static uint16_t cat_n_sec(MenuLayer *l, void *c)                { return 1; }
static uint16_t cat_n_rows(MenuLayer *l, uint16_t s, void *c)   { return NUM_CATEGORIES; }
static int16_t  cat_cell_h(MenuLayer *l, MenuIndex *i, void *c) { return 36; }
static int16_t  cat_hdr_h(MenuLayer *l, uint16_t s, void *c)    { return 16; }

static void cat_draw_hdr(GContext *ctx, const Layer *cl, uint16_t s, void *c) {
  menu_cell_basic_header_draw(ctx, cl, "Choose Category");
}

static void cat_draw_row(GContext *ctx, const Layer *cl, MenuIndex *idx, void *c) {
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

#ifdef HAS_TOUCHSCREEN
static void cat_touch_handler(const TouchEvent *event, void *ctx) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      s_touch_start.x = event->x;
      s_touch_start.y = event->y;
      s_touch_active  = true;
      break;
    case TouchEvent_Liftoff: {
      if (!s_touch_active) break;
      s_touch_active = false;
      int dy = (int)event->y - (int)s_touch_start.y;
      MenuIndex idx = menu_layer_get_selected_index(s_cat_menu);
      if (dy < -SWIPE_THRESHOLD) {
        if (idx.row > 0) {
          idx.row--;
          menu_layer_set_selected_index(s_cat_menu, idx, MenuRowAlignCenter, true);
        }
      } else if (dy > SWIPE_THRESHOLD) {
        if (idx.row < NUM_CATEGORIES - 1) {
          idx.row++;
          menu_layer_set_selected_index(s_cat_menu, idx, MenuRowAlignCenter, true);
        }
      } else {
        cat_select(s_cat_menu, &idx, NULL);
      }
      break;
    }
    default: break;
  }
}
#endif

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
#ifdef HAS_TOUCHSCREEN
  touch_service_subscribe(cat_touch_handler, NULL);
#endif
}

static void cat_win_unload(Window *w) {
#ifdef HAS_TOUCHSCREEN
  touch_service_unsubscribe();
#endif
  menu_layer_destroy(s_cat_menu);
}

/* ══ Difficulty menu window ════════════════════════════════════ */
static Window    *s_diff_win;
static MenuLayer *s_diff_menu;

static uint16_t diff_n_sec(MenuLayer *l, void *c)                { return 1; }
static uint16_t diff_n_rows(MenuLayer *l, uint16_t s, void *c)   { return NUM_DIFF; }
static int16_t  diff_cell_h(MenuLayer *l, MenuIndex *i, void *c) { return 36; }
static int16_t  diff_hdr_h(MenuLayer *l, uint16_t s, void *c)    { return 20; }

static void diff_draw_hdr(GContext *ctx, const Layer *cl, uint16_t s, void *c) {
#ifdef PBL_COLOR
  GColor bg = (GColor){.argb = CAT_BG[s_cat_idx]};
  GColor fg = CAT_WHITE_TXT[s_cat_idx] ? GColorWhite : GColorBlack;
  GRect b = layer_get_bounds(cl);
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);
  GRect tr = GRect(4, 2, b.size.w - 8, b.size.h);
  graphics_draw_text(ctx, CATEGORIES[s_cat_idx].name,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     tr, GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
#else
  menu_cell_basic_header_draw(ctx, cl, CATEGORIES[s_cat_idx].name);
#endif
}

static void diff_draw_row(GContext *ctx, const Layer *cl, MenuIndex *idx, void *c) {
  menu_cell_basic_draw(ctx, cl, DIFF_NAMES[idx->row], NULL, NULL);
}

static void diff_select(MenuLayer *l, MenuIndex *idx, void *c) {
  s_diff = (Difficulty)idx->row;
  trivia_window_push();
  if (s_retry) app_timer_cancel(s_retry);
  s_retry = app_timer_register(300, send_cat_now, NULL);
}

#ifdef HAS_TOUCHSCREEN
static void diff_touch_handler(const TouchEvent *event, void *ctx) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      s_touch_start.x = event->x;
      s_touch_start.y = event->y;
      s_touch_active  = true;
      break;
    case TouchEvent_Liftoff: {
      if (!s_touch_active) break;
      s_touch_active = false;
      int dy = (int)event->y - (int)s_touch_start.y;
      MenuIndex idx = menu_layer_get_selected_index(s_diff_menu);
      if (dy < -SWIPE_THRESHOLD) {
        if (idx.row > 0) {
          idx.row--;
          menu_layer_set_selected_index(s_diff_menu, idx, MenuRowAlignCenter, true);
        }
      } else if (dy > SWIPE_THRESHOLD) {
        if (idx.row < NUM_DIFF - 1) {
          idx.row++;
          menu_layer_set_selected_index(s_diff_menu, idx, MenuRowAlignCenter, true);
        }
      } else {
        diff_select(s_diff_menu, &idx, NULL);
      }
      break;
    }
    default: break;
  }
}
#endif

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
#ifdef HAS_TOUCHSCREEN
  touch_service_subscribe(diff_touch_handler, NULL);
#endif
}

static void diff_win_unload(Window *w) {
#ifdef HAS_TOUCHSCREEN
  touch_service_unsubscribe();
#endif
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
static Layer       *s_choices_layer;

static AppState s_state         = STATE_LOADING;
static int      s_streak        = 0;
static int      s_session_right = 0;
static int      s_session_total = 0;
static char     s_category[64];
static char     s_question[512];   /* full message text (question + embedded choices) */
static char     s_q_display[512];  /* question text only, for scroll area */
static char     s_choices[4][160]; /* parsed choice texts */
static int      s_num_choices   = 0;
static int      s_correct_idx   = 0;
static int      s_selected_idx  = 0;
static bool     s_last_correct  = false;
static AppTimer *s_result_timer = NULL;
static char     s_label_buf[80];

/* Marquee */
#define MARQUEE_DELAY_MS  1000
#define MARQUEE_TICK_MS     50
#define MARQUEE_SPEED        3   /* px per tick */
#define MARQUEE_HOLD_TICKS  30   /* frames to hold at end before looping */
static int       s_marquee_offset = 0;
static int       s_marquee_max    = 0;
static int       s_marquee_hold   = 0;
static int       s_row_width      = 0;
static AppTimer *s_marquee_delay  = NULL;
static AppTimer *s_marquee_tick   = NULL;

/* Split "question\nA) opt\nB) opt..." into s_q_display + s_choices[] */
static void parse_question(void) {
  char *pa = strstr(s_question, "\nA) ");
  if (!pa) {
    strncpy(s_q_display, s_question, sizeof(s_q_display) - 1);
    s_q_display[sizeof(s_q_display) - 1] = '\0';
    s_num_choices = 0;
    return;
  }
  int qlen = (int)(pa - s_question);
  if (qlen >= (int)sizeof(s_q_display)) qlen = (int)sizeof(s_q_display) - 1;
  strncpy(s_q_display, s_question, qlen);
  s_q_display[qlen] = '\0';

  const char *labels[] = {"\nA) ", "\nB) ", "\nC) ", "\nD) "};
  s_num_choices = 0;
  for (int i = 0; i < 4; i++) {
    char *p = strstr(s_question, labels[i]);
    if (!p) break;
    p += 4;
    char *end = strchr(p, '\n');
    int len = end ? (int)(end - p) : (int)strlen(p);
    if (len >= (int)sizeof(s_choices[i])) len = (int)sizeof(s_choices[i]) - 1;
    strncpy(s_choices[i], p, len);
    s_choices[i][len] = '\0';
    s_num_choices++;
  }
}

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

static void stop_marquee(void) {
  if (s_marquee_delay) { app_timer_cancel(s_marquee_delay); s_marquee_delay = NULL; }
  if (s_marquee_tick)  { app_timer_cancel(s_marquee_tick);  s_marquee_tick  = NULL; }
  s_marquee_offset = 0;
  s_marquee_max    = 0;
}

static void marquee_tick_cb(void *data);

static void marquee_tick_cb(void *data) {
  s_marquee_tick = NULL;
  if (s_state != STATE_SELECTING || !s_choices_layer) return;
  if (s_marquee_offset >= s_marquee_max) {
    if (--s_marquee_hold > 0) {
      /* hold at end */
    } else {
      s_marquee_offset = 0;
      s_marquee_hold   = MARQUEE_HOLD_TICKS;
    }
  } else {
    s_marquee_offset += MARQUEE_SPEED;
    if (s_marquee_offset > s_marquee_max) s_marquee_offset = s_marquee_max;
  }
  layer_mark_dirty(s_choices_layer);
  s_marquee_tick = app_timer_register(MARQUEE_TICK_MS, marquee_tick_cb, NULL);
}

static void marquee_delay_cb(void *data) {
  s_marquee_delay = NULL;
  if (s_state != STATE_SELECTING || !s_choices_layer || s_marquee_max == 0) return;
  s_marquee_hold = MARQUEE_HOLD_TICKS;
  marquee_tick_cb(NULL);
}

static void start_marquee_if_needed(void) {
  stop_marquee();
  if (s_num_choices == 0 || s_row_width == 0) return;
  char buf[170];
  snprintf(buf, sizeof(buf), "%c) %s", 'A' + s_selected_idx, s_choices[s_selected_idx]);
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  GSize sz = graphics_text_layout_get_content_size(
    buf, font, GRect(0, 0, 4000, 50),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);
  if (sz.w > s_row_width) {
    s_marquee_max   = sz.w - s_row_width + 4;
    s_marquee_delay = app_timer_register(MARQUEE_DELAY_MS, marquee_delay_cb, NULL);
  }
}

static void choices_layer_draw(Layer *layer, GContext *ctx) {
  if (s_num_choices == 0) return;
  GRect bounds = layer_get_bounds(layer);
  int row_h = bounds.size.h / s_num_choices;

  for (int i = 0; i < s_num_choices; i++) {
    GRect row = GRect(0, i * row_h, bounds.size.w, row_h);

    bool is_correct = (s_state == STATE_RESULT && i == s_correct_idx);
    bool is_wrong   = (s_state == STATE_RESULT && i == s_selected_idx && !s_last_correct);
    bool is_sel     = (s_state == STATE_SELECTING && i == s_selected_idx);

    GColor bg, fg;
#ifdef PBL_COLOR
    if (is_correct)    { bg = GColorGreen; fg = GColorBlack; }
    else if (is_wrong) { bg = GColorRed;   fg = GColorWhite; }
    else if (is_sel)   { bg = GColorBlack; fg = GColorWhite; }
    else               { bg = GColorWhite; fg = GColorBlack; }
#else
    if (is_correct || is_sel) { bg = GColorBlack; fg = GColorWhite; }
    else                      { bg = GColorWhite; fg = GColorBlack; }
#endif

    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, row, 0, GCornerNone);

    char buf[170];
    snprintf(buf, sizeof(buf), "%c) %s", 'A' + i, s_choices[i]);
    graphics_context_set_text_color(ctx, fg);
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    if (is_sel && s_marquee_offset > 0) {
      /* Scroll text left — layer clips anything outside its bounds */
      GRect tr = GRect(4 - s_marquee_offset, row.origin.y + 2, 4000, row_h - 2);
      graphics_draw_text(ctx, buf, font, tr,
                         GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    } else {
      GRect tr = GRect(4, row.origin.y + 2, row.size.w - 8, row_h - 2);
      graphics_draw_text(ctx, buf, font, tr,
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }

    if (i < s_num_choices - 1) {
#ifdef PBL_COLOR
      graphics_context_set_stroke_color(ctx, GColorLightGray);
#else
      graphics_context_set_stroke_color(ctx, GColorBlack);
#endif
      graphics_draw_line(ctx,
        GPoint(0, (i + 1) * row_h - 1),
        GPoint(bounds.size.w, (i + 1) * row_h - 1));
    }
  }
}

static void result_timer_cb(void *data) {
  s_result_timer = NULL;
  request_next();
}

static void show_result(bool correct) {
  s_last_correct = correct;
  s_session_total++;
  if (correct) {
    s_session_right++;
    s_streak++;
    vibes_short_pulse();
  } else {
    s_streak = 0;
    vibes_double_pulse();
  }
  s_state = STATE_RESULT;
  update_display();
  if (s_result_timer) app_timer_cancel(s_result_timer);
  s_result_timer = app_timer_register(2000, result_timer_cb, NULL);
}

static void confirm_selection(void) {
  if (s_state != STATE_SELECTING || s_num_choices == 0) return;
  stop_marquee();
  show_result(s_selected_idx == s_correct_idx);
}

static void move_selection(int delta) {
  if (s_state != STATE_SELECTING || s_num_choices == 0) return;
  s_selected_idx += delta;
  if (s_selected_idx < 0) s_selected_idx = 0;
  if (s_selected_idx >= s_num_choices) s_selected_idx = s_num_choices - 1;
  layer_mark_dirty(s_choices_layer);
  start_marquee_if_needed();
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
      layer_set_hidden(s_choices_layer, true);
      break;

    case STATE_SELECTING:
      apply_cat_color();
      if (s_session_total > 0) {
        if (s_streak > 0)
          snprintf(s_label_buf, sizeof(s_label_buf),
                   "%s  %d/%d x%d", s_category,
                   s_session_right, s_session_total, s_streak);
        else
          snprintf(s_label_buf, sizeof(s_label_buf),
                   "%s  %d/%d", s_category,
                   s_session_right, s_session_total);
      } else {
        snprintf(s_label_buf, sizeof(s_label_buf), "%s", s_category);
      }
      text_layer_set_text(s_label_layer, s_label_buf);
      text_layer_set_font(s_main_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      set_scroll_text(s_q_display);
      layer_set_hidden(s_choices_layer, false);
      layer_mark_dirty(s_choices_layer);
#ifdef HAS_TOUCHSCREEN
      text_layer_set_text(s_hint_layer, "swipe=move  tap=pick");
#else
      text_layer_set_text(s_hint_layer, "UP/DN=move  SEL=pick");
#endif
      break;

    case STATE_RESULT: {
      GColor result_bg, result_fg;
#ifdef PBL_COLOR
      result_bg = s_last_correct ? GColorGreen : GColorRed;
      result_fg = s_last_correct ? GColorBlack : GColorWhite;
#else
      result_bg = GColorBlack;
      result_fg = GColorWhite;
#endif
      text_layer_set_background_color(s_label_layer, result_bg);
      text_layer_set_text_color(s_label_layer, result_fg);
      if (s_last_correct)
        snprintf(s_label_buf, sizeof(s_label_buf),
                 "CORRECT!  %d/%d x%d",
                 s_session_right, s_session_total, s_streak);
      else
        snprintf(s_label_buf, sizeof(s_label_buf),
                 "WRONG  %d/%d", s_session_right, s_session_total);
      text_layer_set_text(s_label_layer, s_label_buf);
      set_scroll_text(s_q_display);
      layer_set_hidden(s_choices_layer, false);
      layer_mark_dirty(s_choices_layer);
#ifdef HAS_TOUCHSCREEN
      text_layer_set_text(s_hint_layer, "tap=next");
#else
      text_layer_set_text(s_hint_layer, "SEL=next");
#endif
      break;
    }
  }
}

static void request_next(void) {
  stop_marquee();
  DictionaryIterator *it;
  if (app_message_outbox_begin(&it) == APP_MSG_OK) {
    dict_write_int32(it, MSG_KEY_REQUEST_NEXT, REQUEST_NEXT);
    app_message_outbox_send();
  }
  s_state        = STATE_LOADING;
  s_selected_idx = 0;
  update_display();
}

static void up_click(ClickRecognizerRef r, void *ctx) {
  if (s_state == STATE_SELECTING) {
    move_selection(-1);
  } else if (s_state == STATE_RESULT) {
    if (s_result_timer) { app_timer_cancel(s_result_timer); s_result_timer = NULL; }
    request_next();
  } else {
    GPoint off = scroll_layer_get_content_offset(s_scroll_layer);
    off.y += SCROLL_STEP;
    if (off.y > 0) off.y = 0;
    scroll_layer_set_content_offset(s_scroll_layer, off, true);
  }
}

static void down_click(ClickRecognizerRef r, void *ctx) {
  if (s_state == STATE_SELECTING) {
    move_selection(1);
  } else if (s_state == STATE_RESULT) {
    if (s_result_timer) { app_timer_cancel(s_result_timer); s_result_timer = NULL; }
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
  if (s_state == STATE_SELECTING) {
    confirm_selection();
  } else if (s_state == STATE_RESULT) {
    if (s_result_timer) { app_timer_cancel(s_result_timer); s_result_timer = NULL; }
    request_next();
  }
}

static void click_config(void *ctx) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP,   150, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

#ifdef HAS_TOUCHSCREEN
static void trivia_touch_handler(const TouchEvent *event, void *ctx) {
  if (!s_scroll_layer) return;
  switch (event->type) {
    case TouchEvent_Touchdown:
      s_touch_start.x = event->x;
      s_touch_start.y = event->y;
      s_touch_active  = true;
      break;
    case TouchEvent_Liftoff: {
      if (!s_touch_active) break;
      s_touch_active = false;
      int dy = (int)event->y - (int)s_touch_start.y;
      if (s_state == STATE_SELECTING) {
        if (dy < -SWIPE_THRESHOLD)     move_selection(-1);
        else if (dy > SWIPE_THRESHOLD) move_selection(1);
        else                           confirm_selection();
      } else if (s_state == STATE_RESULT) {
        if (s_result_timer) { app_timer_cancel(s_result_timer); s_result_timer = NULL; }
        request_next();
      }
      break;
    }
    default: break;
  }
}
#endif

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
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_label_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(root, text_layer_get_layer(s_label_layer));

  /* Choices canvas (above hint bar) */
  GRect cr = GRect(HPAD, H - HINT_H - CHOICES_H, W - 2*HPAD, CHOICES_H);
  s_row_width = cr.size.w - 8;   /* usable text width per row */
  s_choices_layer = layer_create(cr);
  layer_set_update_proc(s_choices_layer, choices_layer_draw);
  layer_add_child(root, s_choices_layer);
  layer_set_hidden(s_choices_layer, true);

  /* Scroll area (between label and choices) */
  int scroll_top = LABEL_H + 2;
  int scroll_h   = cr.origin.y - scroll_top - 2;
  GRect sr = GRect(HPAD, scroll_top, W - 2*HPAD, scroll_h);
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

#ifdef HAS_TOUCHSCREEN
  touch_service_subscribe(trivia_touch_handler, NULL);
#endif
}

static void trivia_win_unload(Window *w) {
#ifdef HAS_TOUCHSCREEN
  touch_service_unsubscribe();
#endif
  stop_marquee();
  if (s_result_timer) { app_timer_cancel(s_result_timer); s_result_timer = NULL; }
  text_layer_destroy(s_label_layer);
  text_layer_destroy(s_main_layer);
  scroll_layer_destroy(s_scroll_layer);
  text_layer_destroy(s_hint_layer);
  layer_destroy(s_choices_layer);
  s_scroll_layer  = NULL;
  s_main_layer    = NULL;
  s_label_layer   = NULL;
  s_hint_layer    = NULL;
  s_choices_layer = NULL;
  s_state         = STATE_LOADING;
  s_streak        = 0;
  s_session_right = 0;
  s_session_total = 0;
  s_selected_idx  = 0;
  s_num_choices   = 0;
  if (s_retry) { app_timer_cancel(s_retry); s_retry = NULL; }
}

static void trivia_window_push(void) {
  window_stack_push(s_trivia_win, true);
}

/* ── AppMessage ──────────────────────────────────────────────── */
static void inbox_received(DictionaryIterator *it, void *ctx) {
  Tuple *cat_t = dict_find(it, MSG_KEY_CATEGORY);
  Tuple *q_t   = dict_find(it, MSG_KEY_QUESTION);
  Tuple *idx_t = dict_find(it, MSG_KEY_CORRECT_IDX);

  if (cat_t) strncpy(s_category, cat_t->value->cstring, sizeof(s_category) - 1);
  if (q_t) {
    strncpy(s_question, q_t->value->cstring, sizeof(s_question) - 1);
    parse_question();
    s_correct_idx  = idx_t ? (int)idx_t->value->int32 : 0;
    s_selected_idx = 0;
    s_state        = STATE_SELECTING;
    update_display();
    start_marquee_if_needed();
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
  if (s_result_timer) app_timer_cancel(s_result_timer);
  window_destroy(s_trivia_win);
  window_destroy(s_diff_win);
  window_destroy(s_cat_win);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
