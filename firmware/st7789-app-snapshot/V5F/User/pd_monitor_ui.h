#ifndef PD_MONITOR_UI_H
#define PD_MONITOR_UI_H
#include "pd_monitor_model.h"
#include "pd_source_config.h"
#include <stdio.h>
static PdMonitor monitor;
static lv_obj_t *mon_phase,*mon_pdo,*mon_v,*mon_i,*mon_rx,*mon_drop,*mon_rows[8],*mon_sequences[8];
static lv_obj_t *mon_label(int x,int y,int w,const char *text)
{
    lv_obj_t *o=lv_label_create(lv_screen_active());
    lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,18);
    lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(o,lv_color_hex(0xD7F9FF),0);
    lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_LEFT,0);
    lv_label_set_text(o,text); return o;
}
/* Comparing formatted values also avoids updates for invisible precision changes. */
static void mon_set(lv_obj_t *o,const char *text)
{
    if(strcmp(lv_label_get_text(o),text)) lv_label_set_text(o,text);
}
static void ui_pd_monitor_create(void)
{
    lv_obj_t *s=lv_screen_active();
    lv_obj_set_scrollable(s,false); lv_obj_set_style_pad_all(s,0,0);
    lv_obj_set_style_bg_color(s,lv_color_hex(0x101820),0);
    lv_obj_set_style_bg_opa(s,LV_OPA_COVER,0);
    mon_label(10,6,150,"PD XiaoT");
    mon_label(10,26,150,PD_CAPTURE_DEMO_MODE?"REPLAY 1022":"LIVE CAPTURE");
    mon_phase=mon_label(10,46,150,"WAITING");
    mon_label(10,66,48,"PDO"); mon_pdo=mon_label(64,66,96,"-- / --");
    mon_label(10,86,48,"REQ V"); mon_v=mon_label(64,86,96,"--.-- V");
    mon_label(10,106,48,"REQ I"); mon_i=mon_label(64,106,96,"--.-- A");
    mon_label(10,126,24,"RX"); mon_rx=mon_label(35,126,47,"0");
    mon_label(85,126,43,"DROP"); mon_drop=mon_label(130,126,30,"0");
    mon_label(10,146,150,"SEQ / MSG / CHECK");
    for(unsigned i=0;i<8;++i) {
        mon_sequences[i]=mon_label(10,166+18*i,80,"--");
        mon_rows[i]=mon_label(90,166+18*i,70,"--");
        lv_obj_set_style_text_font(mon_sequences[i],&lv_font_montserrat_12,0);
        lv_obj_set_style_text_font(mon_rows[i],&lv_font_montserrat_12,0);
    }
}
static void ui_pd_monitor_render(void)
{
    static const char *phase[]={"WAITING","CAPS RECEIVED","REQUESTED","ACCEPTED","POWER READY"};
    char b[40]; volatile PdSharedArea *s=PD_Shared_Area();
    const char *state=!PD_Shared_LayoutValid()?"LAYOUT ERROR":
        (s->tx_fault||s->rx_fault)?"QUEUE FAULT":!s->receiver_ready?"NOT READY":phase[monitor.phase];
    mon_set(mon_phase,state);
    if(monitor.valid) snprintf(b,sizeof b,"%u / %u",monitor.pdo,monitor.cap_count);
    else snprintf(b,sizeof b,"-- / %u",monitor.cap_count);
    mon_set(mon_pdo,b);
    if(monitor.valid) snprintf(b,sizeof b,"%u.%02u V",monitor.mv/1000,(monitor.mv%1000)/10);
    else strcpy(b,"--.-- V");
    mon_set(mon_v,b);
    if(monitor.valid) snprintf(b,sizeof b,"%u.%02u A",monitor.ma/1000,(monitor.ma%1000)/10);
    else strcpy(b,"--.-- A");
    mon_set(mon_i,b);
    if(monitor.frames>9999) strcpy(b,"9999+"); else snprintf(b,sizeof b,"%lu",(unsigned long)monitor.frames);
    mon_set(mon_rx,b);
    uint32_t drops=s->tx_full+s->rx_invalid+s->rx_integrity;
    if(drops>99) strcpy(b,"99+"); else snprintf(b,sizeof b,"%lu",(unsigned long)drops);
    mon_set(mon_drop,b);
    for(unsigned i=0;i<8;++i) {
        if(i>=monitor.count) { mon_set(mon_sequences[i],"--"); strcpy(b,"--"); }
        else {
            const PdCaptureRecord *r=&monitor.recent[(monitor.next+8-monitor.count+i)%8];
            const char *quality=(r->crc_status==PD_CHECK_FAIL || r->length_status==PD_CHECK_FAIL ||
                (r->flags&PD_CAPTURE_TRUNCATED) || ((r->flags&PD_CAPTURE_HAS_HEADER) && r->payload_len!=4U*((r->header>>12)&7U)))?"BAD":
                (r->crc_status==PD_CHECK_PASS && r->length_status==PD_CHECK_PASS)?"OK":"?";
            snprintf(b,sizeof b,"%lu",(unsigned long)r->sequence);
            mon_set(mon_sequences[i],b);
            /* Full capture sequence, not the 3-bit PD Message ID.
             * Compact quality: + pass, ! failed, ? unknown. */
            snprintf(b,sizeof b,"%s %c",pd_monitor_name(r),
                     !strcmp(quality,"BAD")?'!':!strcmp(quality,"OK")?'+':'?');
        }
        mon_set(mon_rows[i],b);
    }
}
#endif
