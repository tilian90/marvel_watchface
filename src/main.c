#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static GFont s_time_font, s_date_font, s_battery_font;
static GBitmap *s_background_bitmap,*s_captain_bitmap, *s_empty_battery_bitmap;
static BitmapLayer *s_background_layer, *s_captain_layer, *s_empty_battery_layer;
static Layer *s_battery_level_layer;
static int8_t switcher;
static int s_battery_level;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  static char date_buffer[16];
  
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  strftime(date_buffer, sizeof(date_buffer), "%d %b %y", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void battery_callback(BatteryChargeState state){
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_level_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 24.0F);
  
  // Draw the background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
  
}

static void bluetooth_callback(bool connected){
  if(connected){
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
  }else{
    text_layer_set_background_color(s_time_layer, GColorBlack);
    text_layer_set_text_color(s_time_layer, GColorClear);
  }
  
  if(!connected){
    vibes_double_pulse();
  }
  
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // A tap event occured
  switch(switcher){
    case 0:
    layer_set_hidden(bitmap_layer_get_layer(s_background_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_captain_layer), false);
    switcher = 1;
    break;
    case 1:
    layer_set_hidden(bitmap_layer_get_layer(s_background_layer), false);
    layer_set_hidden(bitmap_layer_get_layer(s_captain_layer), true);
    switcher = 0;
    break;
    default:
    layer_set_hidden(bitmap_layer_get_layer(s_background_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(s_captain_layer), false);
    switcher = 1;
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RESOURCE_ID_ROBO_42));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RESOURCE_ID_ROBO_23));
  s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RESOURCE_ID_ROBO_11));
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 124), bounds.size.w, 50));
  
  s_date_layer = text_layer_create(GRect(0, 100, 144, 28));
  
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, "00:00");
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_IMAGE_BACKGROUND);
  s_captain_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_CAPTAIN);
  s_empty_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FULL_BATTERY);
  
  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);
  
  s_captain_layer = bitmap_layer_create(bounds);
  s_empty_battery_layer = bitmap_layer_create(GRect(110, 0, 34, 15));
  
  // Set the bitmap onto the layer and add to the window
  //Captain
  bitmap_layer_set_bitmap(s_captain_layer, s_captain_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_captain_layer));
  //Iron man
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  //Battery empty
  bitmap_layer_set_bitmap(s_empty_battery_layer, s_empty_battery_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_empty_battery_layer));
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  //Battery level layer
  s_battery_level_layer = layer_create(GRect(114, 4, 24, 7));
  layer_set_update_proc(s_battery_level_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_level_layer);
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  //Show correct battery level on starrt
  battery_callback(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_battery_font);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
  //Destroy Layer
  layer_destroy(s_battery_level_layer);
  
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  gbitmap_destroy(s_captain_bitmap);
  gbitmap_destroy(s_empty_battery_bitmap);
  
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  bitmap_layer_destroy(s_captain_layer);
  bitmap_layer_destroy(s_empty_battery_layer);
  
}


static void init() {
  //Setting switcher to 0
  switcher = 0;
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler); 
  
  // Subscribe to tap events
  accel_tap_service_subscribe(accel_tap_handler);
  
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  window_set_background_color(s_main_window, GColorBlack);
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