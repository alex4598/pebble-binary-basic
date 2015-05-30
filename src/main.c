#include <pebble.h>
   
enum KEYS {
    KEY_TEMPERATURE,
    KEY_CONDITIONS,
    KEY_SUNRISE = 4,
    KEY_SUNSET,
    KEY_WIND,
    KEY_PRESSURE,
    KEY_HUMIDITY,
    KEY_CONDITIONS_ID,
}

static const float hpa_hg = 1.3332239; //mmhq
//static const float hpa_hg = 33.86; //inhq

static const char *day_of_week_2ch[7] = {"su", "mo","tu","we", "th", "fr", "sa"};
static const char *month_of_year_2ch[12] = {"ja", "fe", "mr", "ap", "my", "jn", \
                                            "jl", "au", "se", "oc", "nv", "de"};

static const char *day_of_week_3ch[7] = {"SUN", "MON","TUE","WED", "THU", "FRI", "SAT"};
static const char *month_of_year_3ch[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", \
                                            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static const short int binary_map[4][10] = { \
    {0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, \
    {0, 0, 1, 1, 0, 0, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1} \
};

static char s_day_buffer[3], \
            s_weekday_buffer[4], \
            s_month_buffer[4], \
            s_seconds_buffer[3], \
            s_temperature_buffer[4], \
            s_conditions_buffer[4], \
            s_sunrise_buffer[6], \
            s_sunset_buffer[6], \
            s_wind_buffer[4],\
            s_pressure_buffer[5],\
            s_humidity_buffer[4],\
            s_refreshed_buffer[4];
            
static short int 
          screen_ox, \
          screen_oy, \
          canvas_count_ox, \
          canvas_count_oy, \
          canvas_corner_radius, \
          canvas_border_px, \
          canvas_fill_px, \
          canvas_layer_crop_px, \
          canvas_spacing_ox, \
          canvas_spacing_oy, \
          canvas_border_ox, \
          canvas_border_oy, \
          canvas_rectangle_ox, \
          canvas_rectangle_oy, \
          canvas_circle_radius, \
          canvas_circle_diameter, \
          hours_1stdigit_row, \
          hours_2nddigit_row , \
          minutes_1stdigit_row, \
          minutes_2nddigit_row, \
          seconds_1stdigit_row, \
          seconds_2nddigit_row, \
          default_digit_max_cols, \
          hours_1stdigit_max_cols, \
          minutes_1stdigit_max_cols, \
          seconds_1stdigit_max_cols, \
          canvas_font_text_height_px, \
          canvas_font_digits_height_px, \
          canvas_font_icons_height_px, \
          temperature_degrees, conditions_id, \
          wind, pressure, humidity, refreshed_minutes_ago;

static long int sunrise_epoch, sunset_epoch;

static bool canvas_is_circle, \
            canvas_justified, \
            screen_is_inverted, \
            conditions_updated;

static GCornerMask canvas_corner_mask;

static GFont canvas_font_text;
static GFont canvas_font_icons;
static GFont canvas_font_digits;

static Window *s_main_window;
static Layer *s_display_layer;
static InverterLayer *s_invert_layer;

static TextLayer *s_day_text_layer, \
                 *s_weekday_text_layer, \
                 *s_month_text_layer, \
                 *s_seconds_text_layer, \
                 *s_weather_text_layer, \
                 *s_temperature_text_layer, \
                 *s_sunrise_text_layer, \
                 *s_sunset_text_layer, \
                 *s_wind_text_layer, \
                 *s_pressure_text_layer, \
                 *s_humidity_text_layer, \
                 *s_refreshed_text_layer;

static short int get_canvas_size(short int axis_px, short int canvas_count, short int canvas_spacing) {
    return (axis_px - ((canvas_count + 1) * canvas_spacing)) / canvas_count;
}

static short int get_canvas_border(short int axis_px, short int canvas_count, short int canvas_spacing) {
    float get_canvas_size = (axis_px - ((canvas_count + 1) * canvas_spacing)) / canvas_count;
    return (axis_px - canvas_spacing - ((get_canvas_size + canvas_spacing) * canvas_count)) / 2;
}

static void init_settings() {
    
    screen_is_inverted = false;
    screen_ox = 144;
    screen_oy = 168;
    
    canvas_is_circle = false;
    canvas_justified = false;
    canvas_corner_radius = 0;
    canvas_corner_mask = GCornersAll;
    canvas_border_px = 1;
    canvas_fill_px = 0;
    canvas_layer_crop_px = canvas_fill_px;

    canvas_count_ox = 4;
    canvas_count_oy = 4;

    canvas_spacing_ox = 1;
    canvas_spacing_oy = 1;

    canvas_rectangle_ox = get_canvas_size(screen_ox,canvas_count_ox,canvas_spacing_ox);
    canvas_rectangle_oy = get_canvas_size(screen_oy,canvas_count_oy,canvas_spacing_oy);
    if (!canvas_justified) {
      if (canvas_rectangle_oy > canvas_rectangle_ox) {
        canvas_rectangle_oy = canvas_rectangle_ox;
        } else {
        canvas_rectangle_ox = canvas_rectangle_oy;
      } 
    }

    canvas_border_ox = get_canvas_border(screen_ox,canvas_count_ox,canvas_spacing_ox);
    canvas_border_oy = get_canvas_border(screen_oy,canvas_count_oy,canvas_spacing_oy);

    canvas_circle_diameter = (canvas_rectangle_ox < canvas_rectangle_oy) ? \
                              canvas_rectangle_ox : canvas_rectangle_oy;
    
    canvas_circle_radius = (canvas_circle_diameter - 1) / 2;
    
    hours_1stdigit_row = 0;
    hours_2nddigit_row = 1;
    minutes_1stdigit_row = 2;
    minutes_2nddigit_row = 3;
    seconds_1stdigit_row = 4;
    seconds_2nddigit_row = 5;

    default_digit_max_cols = 4;
    hours_1stdigit_max_cols = 4;
    minutes_1stdigit_max_cols = 4;
    seconds_1stdigit_max_cols = 4;

    refreshed_minutes_ago = 0;

    if (canvas_count_ox <= 4) {
        canvas_font_text = fonts_get_system_font(FONT_KEY_GOTHIC_14);
        canvas_font_digits = fonts_load_custom_font( \
                             resource_get_handle(RESOURCE_ID_FONT_DIGITALDREAM_NARROW_18));
        canvas_font_icons = fonts_load_custom_font( \
                             resource_get_handle(RESOURCE_ID_FONT_WEATHERICONS_REGULAR_18));

        canvas_font_text_height_px = 18;
        canvas_font_digits_height_px = 24;
        canvas_font_icons_height_px = 24;
      } else {
        canvas_font_text = fonts_get_system_font(FONT_KEY_GOTHIC_14);
        canvas_font_digits = fonts_get_system_font(FONT_KEY_GOTHIC_14);
        canvas_font_icons = fonts_get_system_font(FONT_KEY_GOTHIC_14);
        canvas_font_text_height_px = 18;
        canvas_font_digits_height_px = 18;
        canvas_font_icons_height_px = 18;
    }
}

static void set_s_month_buffer(int month) {
    if (canvas_count_ox <= 5) {
        snprintf(s_month_buffer, sizeof(s_month_buffer), "%s", month_of_year_3ch[month]);
      } else {
        snprintf(s_month_buffer, sizeof(s_month_buffer), "%s", month_of_year_2ch[month]);
    }
}

static void set_s_weekday_buffer(int weekday) {
    if (canvas_count_ox <= 5) {
        snprintf(s_weekday_buffer, sizeof(s_weekday_buffer), "%s", day_of_week_3ch[weekday]);
      } else {
        snprintf(s_weekday_buffer, sizeof(s_weekday_buffer), "%s", day_of_week_2ch[weekday]);
    }
}

static void set_s_temperature_buffer () {
  if (conditions_updated == true) {
    if (canvas_count_ox <= 5) {
      snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%02dc", temperature_degrees);
    } else {
      snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%02d", temperature_degrees);
    }
  } else {
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "  ");
  }
}

static void set_s_wind_buffer () {
    if (conditions_updated == true) {
      if (canvas_count_ox <= 5) {
        snprintf(s_wind_buffer, sizeof(s_wind_buffer), "%dkh", wind);
      } else {
        snprintf(s_wind_buffer, sizeof(s_wind_buffer), "%d", wind);
      }
    } else {
      snprintf(s_wind_buffer, sizeof(s_wind_buffer), "  ");
    } 
}

static void set_s_pressure_buffer () {
 if (conditions_updated == true) {
  if (canvas_count_ox <= 5) {
        snprintf(s_pressure_buffer, sizeof(s_pressure_buffer), "%d", pressure);
      } else {
        snprintf(s_pressure_buffer, sizeof(s_pressure_buffer), ".%dk", (pressure % 1000)/100);
      }
    } else {
      snprintf(s_pressure_buffer, sizeof(s_pressure_buffer), "  ");
    }
}

static void set_s_humidity_buffer () {
    if (conditions_updated == true) {
      if (canvas_count_ox <= 5) {
        snprintf(s_humidity_buffer, sizeof(s_humidity_buffer), "%d%c", humidity, 37);
        } else {
        snprintf(s_humidity_buffer, sizeof(s_humidity_buffer), "%d", humidity);
      }
    } else {
      snprintf(s_humidity_buffer, sizeof(s_humidity_buffer), "  ");
    } 
}

static void set_s_refreshed_buffer() {
    if (conditions_updated == true) {
      snprintf(s_refreshed_buffer, sizeof(s_refreshed_buffer), "%d", refreshed_minutes_ago);
    } else {
      snprintf(s_refreshed_buffer, sizeof(s_refreshed_buffer), "  ");
    }
}

static void set_s_conditions_buffer() {
  if (canvas_count_ox <=4) {
    short int conditions_prefix = conditions_id / 10;
    switch (conditions_prefix) {
      case 20:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf01d");
        break;
      case 30:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf01c");
        break;
      case 50:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf01a");
        break;
      case 60:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf01b");
        break;
      case 70:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf014");
        break;
      case 80:
        if (conditions_id == 800) {
          snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf00d");
        } else {
          snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf002");
        }
        break;
      case 90:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf073");
        break;
      default:
        snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "\uf03e");
    }
  }
}

static void set_sunset_sunrise_time () {

  struct tm *t;
  t = localtime(&sunrise_epoch);
  short int sunrise_hour = t->tm_hour;
  short int sunrise_minutes = t->tm_min;
  t = localtime(&sunset_epoch);
  short int sunset_hour = t->tm_hour;
  short int sunset_minutes = t->tm_min;
  
  if (canvas_count_ox <= 4) {
    snprintf(s_sunrise_buffer, sizeof(s_sunrise_buffer), "%02d:%02d", sunrise_hour, sunrise_minutes);
    snprintf(s_sunset_buffer, sizeof(s_sunset_buffer), "%02d:%02d", sunset_hour, sunset_minutes);
      } else {
    snprintf(s_sunrise_buffer, sizeof(s_sunrise_buffer), "%02d%d", sunrise_hour, sunrise_minutes / 10);
    snprintf(s_sunset_buffer, sizeof(s_sunset_buffer), "%02d%d", sunset_hour, sunset_minutes / 10);
  }
}

static void set_text_layer_color (short int x, short int y, TextLayer *text_layer) {
  if (binary_map[x][y] == 0) {
      text_layer_set_text_color(text_layer, GColorBlack);
   } else {
      text_layer_set_text_color(text_layer, GColorWhite);
  }
}

static void set_text_layer_parameters (TextLayer *text_layer, GFont font){
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(text_layer));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, font);
}

static GRect get_canvas_text_layer_bounds(GPoint center, short int type) {
  GRect bounds;
  GPoint origin;
  GSize size;
  short int x = center.x;
  short int y = center.y;

    switch (type) {
      case 1:
        origin.x = x - canvas_circle_radius + canvas_layer_crop_px + 2;
        origin.y = y - (canvas_font_digits_height_px / 2);
        size.w = (canvas_circle_radius * 2) - (canvas_layer_crop_px * 2) - 1;
        size.h = canvas_font_digits_height_px;
        break;

      case 2:
        origin.x = x - canvas_circle_radius + canvas_layer_crop_px + 1;
        origin.y = y - (canvas_font_icons_height_px / 2);
        size.w = (canvas_circle_radius * 2) - (canvas_layer_crop_px * 2) - 1;
        size.h = canvas_font_icons_height_px;
        break;

      default:
        origin.x = x - canvas_circle_radius + canvas_layer_crop_px + 1;
        origin.y = y - (canvas_font_text_height_px / 2);
        size.w = (canvas_circle_radius * 2) - (canvas_layer_crop_px * 2) - 1;
        size.h = canvas_font_text_height_px;
        break;
    }

  bounds.origin = origin;
  bounds.size = size;
  return bounds;
}

static void draw_rectangle(GContext *ctx, GRect rectangle, bool filled) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, rectangle, canvas_corner_radius, canvas_corner_mask);
  
  if (canvas_fill_px > 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, grect_crop(rectangle,canvas_fill_px), \
                       canvas_corner_radius, canvas_corner_mask);
  }
  
  if (!filled) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, grect_crop(rectangle,canvas_border_px), \
                       canvas_corner_radius, canvas_corner_mask);
  }
}

static void draw_circle(GContext *ctx, GPoint circle_center, bool filled) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, circle_center, canvas_circle_radius);

  if (canvas_fill_px > 0) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, circle_center, canvas_circle_radius - canvas_fill_px);
  }

  if (!filled) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, circle_center, canvas_circle_radius - canvas_border_px);
  }
}

static GRect get_rectangle_parameters_from_cell_location(unsigned short x, unsigned short y) {
    short int canvas_start_ox = canvas_border_ox + canvas_spacing_ox \
                                + ((canvas_rectangle_ox + canvas_spacing_ox) * x);
    short int canvas_start_oy = screen_oy - canvas_rectangle_oy - canvas_border_oy \
                                - canvas_spacing_oy - ((canvas_rectangle_oy + canvas_spacing_oy) * y);
    return GRect (canvas_start_ox, canvas_start_oy, canvas_rectangle_ox, canvas_rectangle_oy);
}

static GPoint get_circle_center_from_cell_location(unsigned short x, unsigned short y) {
    short int circle_ox = canvas_border_ox + canvas_spacing_ox \
                                    + ((canvas_rectangle_ox + canvas_spacing_ox) * x) \
                                    + (canvas_rectangle_ox / 2) ;
    short int circle_oy = screen_oy - (canvas_rectangle_oy / 2) - canvas_border_oy \
                          - canvas_spacing_oy - ((canvas_rectangle_oy + canvas_spacing_oy) * y);
    return GPoint (circle_ox, circle_oy);
}

static void draw_cell_row_for_digit(GContext *ctx, unsigned short digit, unsigned short max_columns_to_display, unsigned short cell_row) {
  for (int i = 0; i < max_columns_to_display; i++) {
    if (canvas_is_circle == true) {
      draw_circle(ctx, get_circle_center_from_cell_location(cell_row, i), (digit >> i) & 0x1);
    } else { 
      draw_rectangle(ctx, get_rectangle_parameters_from_cell_location(cell_row, i), (digit >> i) & 0x1);
    }
  }
}

static unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12;
}

static void display_layer_update_callback(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  unsigned short display_hour = get_display_hour(t->tm_hour);

  // weekday layer display
  set_text_layer_color (0, display_hour / 10, s_weekday_text_layer);
  set_s_weekday_buffer(t->tm_wday);
  text_layer_set_text(s_weekday_text_layer, s_weekday_buffer);

  // sunrise layer display
  set_text_layer_color (1, display_hour / 10, s_sunrise_text_layer);
  text_layer_set_text(s_sunrise_text_layer, s_sunrise_buffer);

  // wind layer display
  set_text_layer_color (2, display_hour / 10, s_wind_text_layer);
  set_s_wind_buffer();
  text_layer_set_text(s_wind_text_layer, s_wind_buffer);

  // month layer display
  set_text_layer_color (0, display_hour % 10, s_month_text_layer);
  set_s_month_buffer(t->tm_mon);
  text_layer_set_text(s_month_text_layer, s_month_buffer);

  // sunset layer display
  set_text_layer_color (1, display_hour % 10, s_sunset_text_layer);
  text_layer_set_text(s_sunset_text_layer, s_sunset_buffer);

  // pressure layer display
  set_text_layer_color (2, display_hour % 10, s_pressure_text_layer);
  set_s_pressure_buffer();
  text_layer_set_text(s_pressure_text_layer, s_pressure_buffer);

  // day layer display
  set_text_layer_color (0, t->tm_min / 10, s_day_text_layer);
  snprintf(s_day_buffer, sizeof(s_day_buffer), "%02d", t->tm_mday);
  text_layer_set_text(s_day_text_layer, s_day_buffer);

  // weather-today layer display
  set_text_layer_color (1, t->tm_min / 10, s_weather_text_layer);
  set_s_conditions_buffer();
  text_layer_set_text(s_weather_text_layer, s_conditions_buffer);

  // humidity layer display
  set_text_layer_color (2, t->tm_min / 10, s_humidity_text_layer);
  set_s_humidity_buffer();
  text_layer_set_text(s_humidity_text_layer, s_humidity_buffer);
  
  // seconds layer display
  set_text_layer_color (0, t->tm_min % 10, s_seconds_text_layer);
  snprintf(s_seconds_buffer, sizeof(s_seconds_buffer), "%02d", t->tm_sec);
  text_layer_set_text(s_seconds_text_layer, s_seconds_buffer);

  // weather-temperature layer display
  set_text_layer_color (1, t->tm_min % 10, s_temperature_text_layer);
  set_s_temperature_buffer();
  text_layer_set_text(s_temperature_text_layer, s_temperature_buffer);

  // refreshed layer display
  set_text_layer_color (2, t->tm_min % 10, s_refreshed_text_layer);
  set_s_refreshed_buffer();
  text_layer_set_text(s_refreshed_text_layer, s_refreshed_buffer);

  draw_cell_row_for_digit(ctx, display_hour / 10, hours_1stdigit_max_cols, hours_1stdigit_row);
  draw_cell_row_for_digit(ctx, display_hour % 10, default_digit_max_cols, hours_2nddigit_row);
  draw_cell_row_for_digit(ctx, t->tm_min / 10, minutes_1stdigit_max_cols, minutes_1stdigit_row);
  draw_cell_row_for_digit(ctx, t->tm_min % 10, default_digit_max_cols, minutes_2nddigit_row);
  if (canvas_count_ox >= 5) { draw_cell_row_for_digit(ctx, t->tm_sec / 10, seconds_1stdigit_max_cols, seconds_1stdigit_row); }
  if (canvas_count_ox >= 6) { draw_cell_row_for_digit(ctx, t->tm_sec % 10, default_digit_max_cols, seconds_2nddigit_row); }
}

static void handle_time_unit_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_display_layer);
  if (refreshed_minutes_ago > 120) {
    conditions_updated = false;
    conditions_id = 0;
  }
  if (tick_time->tm_sec == 0) { refreshed_minutes_ago += 1; } 
  if ((refreshed_minutes_ago == 45) || \
      (refreshed_minutes_ago > 45 && refreshed_minutes_ago % 15 == 0) || \
      (refreshed_minutes_ago > 180 && refreshed_minutes_ago % 30 == 0)) {
    conditions_updated = false;
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message
    app_message_outbox_send();
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  s_display_layer = layer_create(bounds);
  layer_set_update_proc(s_display_layer, display_layer_update_callback);
  layer_add_child(window_layer, s_display_layer);

  // seconds layer setup
  s_seconds_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(3, 0),1));
  set_text_layer_parameters (s_seconds_text_layer, canvas_font_digits);

  // day layer setup
  s_day_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(2, 0),1));
  set_text_layer_parameters (s_day_text_layer, canvas_font_digits);

  // temperature layer setup
  s_temperature_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(3, 1),0));
  set_text_layer_parameters (s_temperature_text_layer, canvas_font_text);

  // weekday layer setup
  s_weekday_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(0, 0),0));
  set_text_layer_parameters (s_weekday_text_layer, canvas_font_text);

  // sunrise layer setup
  s_sunrise_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(0, 1),0));
  set_text_layer_parameters (s_sunrise_text_layer, canvas_font_text);
 
  // month layer setup
  s_month_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(1, 0),0));
  set_text_layer_parameters (s_month_text_layer, canvas_font_text);

  // sunset layer setup
  s_sunset_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(1, 1),0));
  set_text_layer_parameters (s_sunset_text_layer, canvas_font_text);

  // weather-today layer setup
  s_weather_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(2, 1),2));
  set_text_layer_parameters (s_weather_text_layer, canvas_font_icons);

 // wind layer setup
  s_wind_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(0, 2),0));
  set_text_layer_parameters (s_wind_text_layer, canvas_font_text);

  // pressure layer setup
  s_pressure_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(1, 2),0));
  set_text_layer_parameters (s_pressure_text_layer, canvas_font_text);

  // humidity layer setup
  s_humidity_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(2, 2),0));
  set_text_layer_parameters (s_humidity_text_layer, canvas_font_text);

  // humidity layer setup
  s_refreshed_text_layer = text_layer_create(get_canvas_text_layer_bounds(get_circle_center_from_cell_location(3, 2),0));
  set_text_layer_parameters (s_refreshed_text_layer, canvas_font_text);


  if (screen_is_inverted == true) {
    s_invert_layer = inverter_layer_create(bounds);
    layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(s_invert_layer));
  }
  
}

static void main_window_unload(Window *window) {
  layer_destroy(s_display_layer);
  inverter_layer_destroy(s_invert_layer);
  text_layer_destroy(s_seconds_text_layer);
  text_layer_destroy(s_day_text_layer);
  text_layer_destroy(s_weekday_text_layer);
  text_layer_destroy(s_month_text_layer);
  text_layer_destroy(s_weather_text_layer);
  text_layer_destroy(s_temperature_text_layer);
  text_layer_destroy(s_sunrise_text_layer);
  text_layer_destroy(s_sunset_text_layer);
  text_layer_destroy(s_wind_text_layer);
  text_layer_destroy(s_pressure_text_layer);
  text_layer_destroy(s_humidity_text_layer);
  text_layer_destroy(s_refreshed_text_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  conditions_updated = true;
  refreshed_minutes_ago = 0;
  while(t != NULL) {
    switch(t->key) {
    case KEY_TEMPERATURE:
      temperature_degrees = (int)t->value->int32;
      break;
    case KEY_CONDITIONS:
      snprintf(s_conditions_buffer, sizeof(s_conditions_buffer), "%s", t->value->cstring);
      break;
    case KEY_CONDITIONS_ID:
      conditions_id = (int)t->value->int32;
      break;
    case KEY_SUNRISE:
      sunrise_epoch = (long int)t->value->int32;
      break;
    case KEY_SUNSET:
      sunset_epoch = (long int)t->value->int32;
      break;
    case KEY_WIND:
      wind = (int)t->value->int32;
      break;
    case KEY_PRESSURE:
      pressure = (int)t->value->int32/hpa_hg;
      break;
    case KEY_HUMIDITY:
      humidity = (int)t->value->int32;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    // Look for next item
    t = dict_read_next(iterator);
  }

  set_sunset_sunrise_time();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  init_settings();
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, handle_time_unit_tick);
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Debugging...");
  init();
  app_event_loop();
  deinit();
}
