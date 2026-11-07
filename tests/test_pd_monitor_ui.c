#define Core_V3F 1
#define PD_TRANSPORT_HOST_TEST 1
#include "pd_transport.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
typedef struct {int x,y,w,h; char text[80];} lv_obj_t;
static lv_obj_t objects[48], screen;
static unsigned count, updates;
#define LV_LABEL_LONG_CLIP 0
#define LV_TEXT_ALIGN_LEFT 0
#define LV_OPA_COVER 255
static lv_obj_t *lv_screen_active(void){return &screen;}
static lv_obj_t *lv_label_create(lv_obj_t *p){(void)p; assert(count<48);return &objects[count++];}
static void lv_obj_set_pos(lv_obj_t *o,int x,int y){o->x=x;o->y=y;}
static void lv_obj_set_size(lv_obj_t *o,int w,int h){o->w=w;o->h=h;}
static void lv_label_set_text(lv_obj_t *o,const char *s){assert(strlen(s)<80);strcpy(o->text,s);++updates;}
static const char *lv_label_get_text(lv_obj_t *o){return o->text;}
#define lv_obj_set_style_text_font(...) ((void)0)
#define lv_label_set_long_mode(...) ((void)0)
#define lv_obj_set_style_text_color(...) ((void)0)
#define lv_obj_set_style_text_align(...) ((void)0)
#define lv_obj_set_scrollable(o,...) ((void)(o))
#define lv_obj_set_style_pad_all(...) ((void)0)
#define lv_obj_set_style_bg_color(...) ((void)0)
#define lv_obj_set_style_bg_opa(...) ((void)0)
unsigned char __pd_shared_start[32768] __attribute__((aligned(16)));
unsigned char __pd_shared_end[1];
#include "pd_monitor_ui.h"
int main(void) {
 PD_Shared_BootInit();assert(PD_Transport_Attach());ui_pd_monitor_create();
 for(unsigned i=0;i<count;i++) {
  assert(objects[i].x>=0 && objects[i].x+objects[i].w<=170);
  assert(objects[i].y>=0 && objects[i].y+objects[i].h<=320);
 }
 for(unsigned i=1;i<=8;i++){PdCaptureRecord r;PD_Transport_1022Record(&r,i);pd_monitor_apply(&monitor,&r);}
 ui_pd_monitor_render();assert(!strcmp(mon_phase->text,"POWER READY"));
 assert(!strcmp(mon_v->text,"12.00 V"));assert(!strcmp(mon_i->text,"1.67 A"));
 unsigned old=updates;ui_pd_monitor_render();assert(updates==old);
 int x=mon_rx->x,w=mon_rx->w;monitor.frames=9;ui_pd_monitor_render();
 monitor.frames=10;ui_pd_monitor_render();assert(mon_rx->x==x && mon_rx->w==w);
 assert(updates==old+2);
 for(unsigned i=0;i<150;i++) ui_pd_monitor_render();
 assert(updates==old+2); /* 30 seconds at the production 200 ms render interval. */
 PdCaptureRecord r; PD_Transport_1022Record(&r,9); pd_monitor_apply(&monitor,&r);
 ui_pd_monitor_render();
 for(unsigned i=0;i<8;i++) {
  char expected[16]; snprintf(expected,sizeof expected,"%u",i+2);
  assert(!strcmp(mon_sequences[i]->text,expected));
 }
 assert(!strcmp(mon_rows[7]->text,"CAPS +"));
 old=updates; for(unsigned i=0;i<150;i++) ui_pd_monitor_render(); assert(updates==old);
 r.sequence=UINT32_MAX; pd_monitor_apply(&monitor,&r);ui_pd_monitor_render();
 assert(!strcmp(mon_sequences[7]->text,"4294967295"));
 puts("PASS: portrait bounds, decoded text, identical render zero updates, fixed geometry 9 -> 10");
}
