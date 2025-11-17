#include <pebble.h>
#include <string.h>

// Morse definitions
#define MORSE_DOT_DURATION 100
#define MORSE_DASH_DURATION 300
#define MORSE_GAP_DURATION 100
#define MORSE_DIGIT_GAP_DURATION 500

// Morse patterns for 0-9
static const char* morse_digits[] = {
  "-----", // 0
  ".----", // 1
  "..---", // 2
  "...--", // 3
  "....-", // 4
  ".....", // 5
  "-....", // 6
  "--...", // 7
  "---..", // 8
  "----."  // 9
};

static Window *s_main_window;
static TextLayer *s_time_layer;

static VibePattern vibe_pattern;
static uint32_t vibe_segments[64];

static void queue_vibration_for_digit(int digit, uint32_t *segment_index) {
    const char* morse_pattern = morse_digits[digit];
    for (size_t i = 0; i < strlen(morse_pattern); i++) {
        if (*segment_index >= 62) return; // Prevent overflow

        if (morse_pattern[i] == '.') {
            vibe_segments[(*segment_index)++] = MORSE_DOT_DURATION;
        } else {
            vibe_segments[(*segment_index)++] = MORSE_DASH_DURATION;
        }
        vibe_segments[(*segment_index)++] = MORSE_GAP_DURATION;
    }
}

static void trigger_morse_time_vibration() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    int hour = tick_time->tm_hour;
    int minute = tick_time->tm_min;

    if (!clock_is_24h_style()) {
        hour = hour % 12;
        if (hour == 0) {
            hour = 12;
        }
    }

    uint32_t segment_index = 0;

    // Hour - first digit
    if (hour >= 10) {
        queue_vibration_for_digit(hour / 10, &segment_index);
        if (segment_index > 0) {
            vibe_segments[segment_index - 1] = MORSE_DIGIT_GAP_DURATION;
        }
    }

    // Hour - second digit
    queue_vibration_for_digit(hour % 10, &segment_index);
    if (segment_index > 0) {
        vibe_segments[segment_index - 1] = MORSE_DIGIT_GAP_DURATION;
    }

    // Minute - first digit
    queue_vibration_for_digit(minute / 10, &segment_index);
    if (segment_index > 0) {
        vibe_segments[segment_index - 1] = MORSE_DIGIT_GAP_DURATION;
    }

    // Minute - second digit
    queue_vibration_for_digit(minute % 10, &segment_index);

    vibe_pattern.durations = vibe_segments;
    vibe_pattern.num_segments = segment_index;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Vibrating time in Morse code.");
    static char s_pattern_buffer[512];
    int offset = snprintf(s_pattern_buffer, sizeof(s_pattern_buffer), "Pattern: ");
    for(uint32_t i = 0; i < vibe_pattern.num_segments; i++) {
      offset += snprintf(s_pattern_buffer + offset, sizeof(s_pattern_buffer) - offset, "%lu ", (unsigned long)vibe_segments[i]);
      if (offset >= (int)sizeof(s_pattern_buffer)) {
        // Buffer is full, log what we have and stop.
        APP_LOG(APP_LOG_LEVEL_DEBUG, "%s...", s_pattern_buffer);
        break;
      }
    }
    if (offset < (int)sizeof(s_pattern_buffer)) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", s_pattern_buffer);
    }

    vibes_enqueue_custom_pattern(vibe_pattern);
}

static void long_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  // Called when the button is held down for the specified delay.
  trigger_morse_time_vibration();
}

static void prv_click_config_provider(void *context) {
  // long click config
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, long_click_down_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_UP, 0, long_click_down_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, long_click_down_handler, NULL);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, (bounds.size.h - 50) / 2, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set click handlers
  window_set_click_config_provider(s_main_window, prv_click_config_provider);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
