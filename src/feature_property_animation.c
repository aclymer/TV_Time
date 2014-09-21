#include <pebble.h>
#include <stdlib.h>

// Declare window and layers
static Window *window;
static GBitmap *s_res_static_1_image;
static GBitmap *s_res_tv_image;
static TextLayer *time_layer;
static TextLayer *ch_layer;
static TextLayer *screen_on_layer;
static BitmapLayer *static_1_bitmap;
static BitmapLayer *tv_bitmap;
static InverterLayer *static_inverter_layer;
static GFont s_res_bitham_42_bold;
static GFont s_res_gothic_14;

// Declare app timers
AppTimer *inverter_timer;
AppTimer *time_timer;
AppTimer *show_time;

char buffer[6];

#define INVERTER_TIMEOUT 	71			// Delay between inverter bar pos. updates (71 ~= 1 full scroll / 5 sec)
#define TIME_TIMEOUT			60000		// Time update interval (ms)
#define TIMEON						3000		// Timer duration to show the time (ms)
#define BARWIDTH					30			// Thickness of the bar in pixels

GRect to_rect;

// Updates the time text
static void update_time(void *data)
{
	clock_copy_time_string(buffer, 12);
	text_layer_set_text(time_layer, buffer);
	time_timer = app_timer_register(TIME_TIMEOUT, update_time, 0);
}

// Updates the position of the inverter bar
static void inverter_timer_callback(void *data)
{
	to_rect = layer_get_frame(inverter_layer_get_layer(static_inverter_layer));
	if (to_rect.size.h < BARWIDTH && to_rect.origin.y == 12 + BARWIDTH) to_rect.size.h++;
	else to_rect.origin.y++;
	if (to_rect.origin.y > 69 + BARWIDTH) to_rect.size.h--;
	if (to_rect.origin.y > 98 + BARWIDTH) to_rect.origin.y = 12 + BARWIDTH;
  layer_set_frame(inverter_layer_get_layer(static_inverter_layer), to_rect);
	layer_mark_dirty((Layer *) tv_bitmap); 
	inverter_timer = app_timer_register(INVERTER_TIMEOUT, inverter_timer_callback, 0);
}

// Hide time callback
void hide_time(void *data)
{
	layer_set_hidden(text_layer_get_layer(time_layer), true);
	layer_set_hidden(text_layer_get_layer(screen_on_layer), true);
	layer_set_hidden(inverter_layer_get_layer(static_inverter_layer), false);
}

// Callback for tap event (ACCELL_AXIS_Y == wrist flick)
void tap_handler(AccelAxisType axis, int32_t dir)
{
	if (axis == (ACCEL_AXIS_Y || ACCEL_AXIS_Z))	// Flick wrist or top screen to show time
	{
		show_time = app_timer_register(TIMEON, hide_time, 0); // Register timer to hide time after TIMEON)
		layer_set_hidden(text_layer_get_layer(screen_on_layer), false);
		layer_set_hidden(inverter_layer_get_layer(static_inverter_layer), true);
		layer_set_hidden(text_layer_get_layer(time_layer), false);
	}
}

// Initialise UI
static void init(void)
{
	srand(time(NULL));
	inverter_timer 	= app_timer_register(INVERTER_TIMEOUT, inverter_timer_callback, 0);
	time_timer 			= app_timer_register(TIME_TIMEOUT, update_time, 0);
	
  window 					= window_create();
	
	window_set_fullscreen(window, true);
  window_stack_push(window, false);
	
	// Assign resources
  s_res_tv_image 				= gbitmap_create_with_resource(RESOURCE_ID_TV_IMAGE);
  s_res_static_1_image 	= gbitmap_create_with_resource(RESOURCE_ID_STATIC_2_IMAGE);
	s_res_bitham_42_bold 	= fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
	s_res_gothic_14 			= fonts_get_system_font(FONT_KEY_GOTHIC_14);
		
	// tv_bitmap
	tv_bitmap = bitmap_layer_create(GRect(0,0,144,168));
	bitmap_layer_set_bitmap(tv_bitmap, s_res_tv_image);
	bitmap_layer_set_background_color(tv_bitmap, GColorClear);
	bitmap_layer_set_compositing_mode(tv_bitmap, GCompOpAssign);
	layer_add_child(window_get_root_layer(window), (Layer *) tv_bitmap);
	
  // static_1_bitmap
  static_1_bitmap = bitmap_layer_create(GRect(14, 42, 115, 87));
  bitmap_layer_set_bitmap(static_1_bitmap, s_res_static_1_image);
	bitmap_layer_set_compositing_mode(static_1_bitmap, GCompOpOr);
  layer_add_child(window_get_root_layer(window), (Layer *)static_1_bitmap);
  
  // inverter_layer
  static_inverter_layer = inverter_layer_create(GRect(14, 42, 115, 0));
  layer_add_child(window_get_root_layer(window), (Layer *)static_inverter_layer);
	layer_set_hidden(inverter_layer_get_layer(static_inverter_layer), false);
	
	// Screen On layer
  screen_on_layer = text_layer_create(GRect(14, 42, 115, 87));
  text_layer_set_background_color(screen_on_layer, GColorWhite);
  layer_set_hidden(text_layer_get_layer(screen_on_layer), true);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(screen_on_layer));
	
	// ch_layer
	ch_layer = text_layer_create(GRect(102, 46, 24, 14));
	text_layer_set_background_color(ch_layer, GColorClear);
	text_layer_set_text(ch_layer, "Ch 3");
	text_layer_set_font(ch_layer, s_res_gothic_14);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(ch_layer));
	
  // time_layer
	clock_copy_time_string(buffer, 12);
  time_layer = text_layer_create(GRect(0, 56, 144, 42));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text(time_layer, buffer);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer, s_res_bitham_42_bold);
	layer_set_hidden(text_layer_get_layer(time_layer), true);
  layer_add_child(window_get_root_layer(window), (Layer *)time_layer);
	
	accel_tap_service_subscribe(tap_handler); // Subcribe to the tap event service
}

// De-Initialise UI
static void deinit(void) {
	accel_data_service_unsubscribe();
	app_timer_cancel(show_time);
	app_timer_cancel(inverter_timer);
	app_timer_cancel(time_timer);
  bitmap_layer_destroy(static_1_bitmap);
  text_layer_destroy(time_layer);
  inverter_layer_destroy(static_inverter_layer);
  gbitmap_destroy(s_res_static_1_image);
  window_stack_remove(window, false);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

