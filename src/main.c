#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_suffix_layer;
static TextLayer *s_date_layer;

static bool bt_status;
static GBitmap *bt_icon_bitmap;
static BitmapLayer *s_bt_icon_layer;

static GBitmap *battery_container_bitmap;
static BitmapLayer *s_battery_container;
static GBitmap *battery_pellet_bitmap;
static BitmapLayer *s_battery_pellet[20];

static void bluetooth_callback(bool connected) {
  bt_status = connected;
  
  // update image
  layer_set_hidden((Layer*)s_bt_icon_layer, !bt_status);
}

static void update_battery_status(BatteryChargeState charge_state) {
  for (int i=0; i<20; i++) {
    // check each pellet (5%)
    bool pelletShouldBeHidden = charge_state.charge_percent < i*5;
    bool isHidden = layer_get_hidden((Layer*)s_battery_pellet[i]);
    if (pelletShouldBeHidden != isHidden) {
      layer_set_hidden((Layer*)s_battery_pellet[i], pelletShouldBeHidden);
    }
  }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";
  static char suffixBuffer[] = "am";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 24h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    layer_set_hidden((Layer*)s_suffix_layer, true);
    
    // make sure it is the right width
    GRect bounds = layer_get_bounds((Layer*)s_time_layer);
    if (bounds.size.w != 134) {
      bounds.size.w = 134;
      layer_set_bounds((Layer*)s_time_layer, bounds);
    }
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    if (buffer[0] == '0') {
      memmove(buffer, &buffer[1], sizeof(buffer) - 1);
    }
    strftime(suffixBuffer, sizeof("am"), "%p", tick_time);
    layer_set_hidden((Layer*)s_suffix_layer, false);
    
    // make sure it is the right width
    GRect bounds = layer_get_bounds((Layer*)s_time_layer);
    if (bounds.size.w != 110) {
      bounds.size.w = 110;
      layer_set_bounds((Layer*)s_time_layer, bounds);
    }
  }
  
  // date
  static char dateBuffer[] = "Mon Jan 01";
  strftime(dateBuffer, sizeof("Mon Jan 01"), "%a %b %d", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_suffix_layer, suffixBuffer);
  text_layer_set_text(s_date_layer, dateBuffer);
}

static void main_window_load(Window *window) {
  int textStartY = 80;
  int timeWidth = 110;
  
  if(clock_is_24h_style() == true) {
    timeWidth = 134;
  }
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, textStartY, timeWidth, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
  
  s_suffix_layer = text_layer_create(GRect(114, textStartY+24, 40, 50));
  text_layer_set_background_color(s_suffix_layer, GColorClear);
  text_layer_set_text_color(s_suffix_layer, GColorWhite);
  text_layer_set_text(s_suffix_layer, "am");
  text_layer_set_font(s_suffix_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_suffix_layer, GTextAlignmentLeft);
  
  s_date_layer = text_layer_create(GRect(10, textStartY+41, 124, 50));
  text_layer_set_background_color(s_date_layer, GColorClear);
  #ifdef PBL_COLOR
    text_layer_set_text_color(s_date_layer, GColorPictonBlue);
  #else
    text_layer_set_text_color(s_date_layer, GColorWhite);
  #endif
  text_layer_set_text(s_date_layer, "Mon Jan 01");
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_suffix_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // Battery
  s_battery_container = bitmap_layer_create(GRect(7, 7, 14, 60));
  bitmap_layer_set_background_color(s_battery_container, GColorClear);
  bitmap_layer_set_bitmap(s_battery_container, battery_container_bitmap);
  bitmap_layer_set_compositing_mode(s_battery_container, GCompOpAssign);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer (s_battery_container));
  
  // Battery Pellets
  for(int i=0; i<20; i++) {
    s_battery_pellet[i] = bitmap_layer_create(GRect(11, 9 + (20 - i) * 2, 6, 1));
    bitmap_layer_set_background_color(s_battery_pellet[i], GColorClear);
    bitmap_layer_set_bitmap(s_battery_pellet[i], battery_pellet_bitmap);
    bitmap_layer_set_compositing_mode(s_battery_pellet[i], GCompOpAssign);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer (s_battery_pellet[i]));
  }
  update_battery_status(battery_state_service_peek());
  battery_state_service_subscribe(update_battery_status);
  
  // BT 
  s_bt_icon_layer = bitmap_layer_create(GRect(10, 53, 8, 11));
  bitmap_layer_set_background_color(s_bt_icon_layer, GColorClear);
  bitmap_layer_set_bitmap(s_bt_icon_layer, bt_icon_bitmap);
  #ifdef PBL_COLOR
    bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);
  #else
    bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpAssign);
  #endif
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer (s_bt_icon_layer));
  
	bt_status = bluetooth_connection_service_peek();
	bluetooth_connection_service_subscribe( bluetooth_callback );
  
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_suffix_layer);
  text_layer_destroy(s_date_layer);
  
  // Destroy BT Layer
  bluetooth_connection_service_unsubscribe();
  bitmap_layer_destroy(s_bt_icon_layer);
  
  // Destroy Battery Layers
  battery_state_service_unsubscribe();
  bitmap_layer_destroy(s_battery_container);
  for(int i=0; i<20; i++) {
    bitmap_layer_destroy(s_battery_pellet[i]);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  #ifdef PBL_COLOR
    window_set_background_color(s_main_window, GColorOxfordBlue);
  #else 
    window_set_background_color(s_main_window, GColorBlack);
  #endif
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register Bluetooth status
	bt_status = bluetooth_connection_service_peek();
	bluetooth_connection_service_subscribe( bluetooth_callback );
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);
  battery_container_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIFE_CONTAINER);
  battery_pellet_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIFE_TICK);
  
  init();
  app_event_loop();
  deinit();
}
