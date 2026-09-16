#include <pebble.h>
#include <ctype.h>
#include "pixel.h"
#include "clock_font.h"

static Window *s_window;
static Layer *s_canvas;
static GBitmap *s_art;
static GFont s_metric_font, s_battery_font;
static void label(GContext *ctx,const char *s,int x,int y,int width,GFont font) {
 graphics_context_set_text_color(ctx,GColorBlack);
 graphics_draw_text(ctx,s,font,GRect(x,y,width,22),GTextOverflowModeTrailingEllipsis,GTextAlignmentLeft,NULL);
}
static int s_battery, s_temp, s_code;
static time_t s_weather_time;
static int32_t s_steps=-1, s_heart=-1;
static bool s_charging;

// The display has no solid ivory: mix native white, light gray and pale yellow.
// Apply once at load so the soft paper tone costs nothing on minute redraws.
static void warm_paper(GBitmap *bitmap) {
 if(!bitmap || gbitmap_get_format(bitmap)!=GBitmapFormat8Bit) return;
 uint8_t *data=gbitmap_get_data(bitmap);
 const int stride=gbitmap_get_bytes_per_row(bitmap);
 const GRect bounds=gbitmap_get_bounds(bitmap);
 static const uint8_t bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
 for(int y=0;y<bounds.size.h;y++) for(int x=0;x<bounds.size.w;x++) {
  uint8_t *pixel=&data[y*stride+x];
  if((*pixel & 0x3c)==0x3c && (*pixel & 3)>=2) {
   const int level=bayer[(y%4)*4+x%4];
   *pixel=level<8?GColorLightGray.argb:(level<12?GColorPastelYellow.argb:GColorWhite.argb);
  }
 }
}

// Display the original plant at 90%, anchored to the lower-right corner.
// Integer nearest-neighbor sampling preserves its pixel-art edges.
static GBitmap *create_background(void) {
 GBitmap *source=gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
 if(!source) return NULL;
 GBitmap *result=gbitmap_create_blank(GSize(200,228),GBitmapFormat8Bit);
 if(!result) return source;
 uint8_t *dst=gbitmap_get_data(result);
 const uint8_t *src=gbitmap_get_data(source);
 const int ds=gbitmap_get_bytes_per_row(result), ss=gbitmap_get_bytes_per_row(source);
 const GBitmapFormat format=gbitmap_get_format(source);
 const GColor *palette=gbitmap_get_palette(source);
 const int bits=format==GBitmapFormat1BitPalette?1:(format==GBitmapFormat2BitPalette?2:(format==GBitmapFormat4BitPalette?4:8));
 for(int y=0;y<228;y++) for(int x=0;x<200;x++) {
  uint8_t color=GColorWhite.argb;
  if(x>=20 && y>=23) {
   const int sx=(x-20)*200/180, sy=(y-23)*228/205;
   const uint8_t value=(src[sy*ss+sx*bits/8] >> (8-bits-(sx*bits)%8)) & ((1<<bits)-1);
   color=palette?palette[value].argb:value;
  }
  dst[y*ds+x]=color;
 }
 gbitmap_destroy(source);
 return result;
}

static void health_refresh(void) {
 s_steps=-1; s_heart=-1;
#if defined(PBL_HEALTH)
 time_t now=time(NULL);
 if(health_service_metric_accessible(HealthMetricStepCount,time_start_of_today(),now)&HealthServiceAccessibilityMaskAvailable)
  s_steps=health_service_sum_today(HealthMetricStepCount);
 if(health_service_metric_accessible(HealthMetricHeartRateBPM,now-600,now)&HealthServiceAccessibilityMaskAvailable) {
  int32_t hr=health_service_peek_current_value(HealthMetricHeartRateBPM);
  if(hr>0) s_heart=hr;
 }
#endif
}
static void icon(GContext *ctx,int x,int y,const uint16_t *rows,int h,int w) {
 graphics_context_set_fill_color(ctx,GColorBlack);
 const int dw=(w*4+2)/5, dh=(h*4+2)/5;
 x+=(w-dw)/2; y+=(h-dh)/2;
 for(int r=0;r<dh;r++) for(int c=0;c<dw;c++)
  if(rows[r*h/dh]&(1<<(w-1-c*w/dw))) graphics_fill_rect(ctx,GRect(x+c,y+r,1,1),0,GCornerNone);
}
static void weather_icon(GContext *ctx,bool valid) {
 graphics_context_set_stroke_color(ctx,GColorBlack);
 graphics_context_set_fill_color(ctx,GColorBlack);
 if(!valid) {pixel_text(ctx,"-",15,68,1,1);return;}
 if(s_code<=1) {
  graphics_fill_circle(ctx,GPoint(17,70),3);
  graphics_draw_line(ctx,GPoint(17,64),GPoint(17,65));
  graphics_draw_line(ctx,GPoint(17,75),GPoint(17,76));
  graphics_draw_line(ctx,GPoint(11,70),GPoint(12,70));
  graphics_draw_line(ctx,GPoint(22,70),GPoint(23,70));
  graphics_draw_line(ctx,GPoint(12,65),GPoint(13,66));
  graphics_draw_line(ctx,GPoint(21,74),GPoint(22,75));
  graphics_draw_line(ctx,GPoint(12,75),GPoint(13,74));
  graphics_draw_line(ctx,GPoint(21,66),GPoint(22,65));
 } else {
  graphics_fill_circle(ctx,GPoint(15,70),4);
  graphics_fill_circle(ctx,GPoint(19,68),4);
  graphics_fill_circle(ctx,GPoint(22,71),3);
  graphics_fill_rect(ctx,GRect(13,70,10,4),0,GCornerNone);
  if(s_code>=51) for(int x=14;x<=22;x+=4) graphics_draw_line(ctx,GPoint(x,76),GPoint(x-1,78));
 }
}
static void draw(Layer *layer,GContext *ctx) {
 graphics_context_set_compositing_mode(ctx,GCompOpSet);
 if(s_art) graphics_draw_bitmap_in_rect(ctx,s_art,layer_get_bounds(layer));
 time_t now=time(NULL); struct tm *t=localtime(&now);
 char buf[24]; strftime(buf,sizeof(buf),"%a %b %d",t);
 for(char *p=buf;*p;p++) *p=toupper((unsigned char)*p);
 label(ctx,buf,10,2,125,s_metric_font);
 strftime(buf,sizeof(buf),clock_is_24h_style()?"%H:%M":"%I:%M",t);
 if(!clock_is_24h_style() && buf[0]=='0') memmove(buf,buf+1,strlen(buf));
 clock_text(ctx,buf,10,26);
 graphics_context_set_stroke_color(ctx,GColorBlack);
 graphics_context_set_fill_color(ctx,GColorBlack);
 graphics_draw_rect(ctx,GRect(173,7,19,10));
 graphics_fill_rect(ctx,GRect(192,10,2,4),0,GCornerNone);
 int bw=15*s_battery/100;
 if(bw) graphics_fill_rect(ctx,GRect(175,9,bw,6),0,GCornerNone);
 snprintf(buf,sizeof(buf),"%d%%",s_battery);label(ctx,buf,166,16,34,s_battery_font);
 if(s_charging) pixel_text(ctx,"+",165,9,1,1);
 bool valid=s_weather_time>0 && now>=s_weather_time && now-s_weather_time<7200;
 weather_icon(ctx,valid);
 if(valid) snprintf(buf,sizeof(buf),"%d°",s_temp); else strcpy(buf,"--°");
 label(ctx,buf,33,60,60,s_metric_font);
 static const uint16_t feet[]={0x018,0x03c,0x03c,0x03c,0x03c,0x218,0x700,0x780,0x780,0x780,0x780,0x300};
 icon(ctx,11,84,feet,12,12);
 if(s_steps<0) strcpy(buf,"--");
 else if(s_steps>=100000) snprintf(buf,sizeof(buf),"%ldK",(long)(s_steps/1000));
 else if(s_steps>=1000) snprintf(buf,sizeof(buf),"%ld,%03ld",(long)(s_steps/1000),(long)(s_steps%1000));
 else snprintf(buf,sizeof(buf),"%ld",(long)s_steps);
 label(ctx,buf,33,78,62,s_metric_font);
 static const uint16_t heart[]={0x318,0x7bc,0x7fc,0x7fc,0x3f8,0x1f0,0x0e0,0x040};
 icon(ctx,10,103,heart,8,12);
 if(s_heart>0) snprintf(buf,sizeof(buf),"%ld",(long)s_heart); else strcpy(buf,"--");
 label(ctx,buf,33,95,35,s_metric_font);
}
static void request_weather(void) {
 if(!connection_service_peek_pebble_app_connection()) return;
 DictionaryIterator *iter;
 if(app_message_outbox_begin(&iter)==APP_MSG_OK) {
  dict_write_uint8(iter,MESSAGE_KEY_FetchWeather,1);app_message_outbox_send();
 }
}
static void inbox(DictionaryIterator *iter,void *context) {
 Tuple *temp=dict_find(iter,MESSAGE_KEY_Temperature), *code=dict_find(iter,MESSAGE_KEY_WeatherCode), *stamp=dict_find(iter,MESSAGE_KEY_WeatherTime);
 if(temp&&code&&stamp) {
  s_temp=temp->value->int32;s_code=code->value->int32;s_weather_time=stamp->value->int32;
  persist_write_int(1,s_temp);persist_write_int(2,s_code);persist_write_int(3,s_weather_time);
  layer_mark_dirty(s_canvas);
 }
}
static void tick(struct tm *t,TimeUnits units) {
 health_refresh(); layer_mark_dirty(s_canvas);
 if(t->tm_min%30==0 || (!s_weather_time && t->tm_min%5==0)) request_weather();
}
static void battery(BatteryChargeState state) {
 s_battery=state.charge_percent;s_charging=state.is_charging;
 if(s_canvas) layer_mark_dirty(s_canvas);
}
static void connection(bool connected) {if(connected) request_weather();}
#if defined(PBL_HEALTH)
static void health(HealthEventType event,void *context) {health_refresh();if(s_canvas) layer_mark_dirty(s_canvas);}
#endif
static void init(void) {
 s_temp=persist_read_int(1);s_code=persist_read_int(2);s_weather_time=persist_read_int(3);
 s_battery_font=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BATTERY_12));
 s_metric_font=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_METRIC_16));
 s_window=window_create();window_set_background_color(s_window,GColorPastelYellow);
 s_art=create_background();
 warm_paper(s_art);
 s_canvas=layer_create(GRect(0,0,200,228));layer_set_update_proc(s_canvas,draw);
 layer_add_child(window_get_root_layer(s_window),s_canvas);window_stack_push(s_window,true);
 health_refresh();battery(battery_state_service_peek());
 app_message_register_inbox_received(inbox);app_message_open(128,64);
 tick_timer_service_subscribe(MINUTE_UNIT,tick);battery_state_service_subscribe(battery);
 connection_service_subscribe((ConnectionHandlers){.pebble_app_connection_handler=connection});
#if defined(PBL_HEALTH)
 health_service_events_subscribe(health,NULL);
#endif
}
static void deinit(void) {
 tick_timer_service_unsubscribe();battery_state_service_unsubscribe();connection_service_unsubscribe();
#if defined(PBL_HEALTH)
 health_service_events_unsubscribe();
#endif
 fonts_unload_custom_font(s_battery_font);fonts_unload_custom_font(s_metric_font);
 app_message_deregister_callbacks();layer_destroy(s_canvas);gbitmap_destroy(s_art);window_destroy(s_window);
}
int main(void) {init();app_event_loop();deinit();}
