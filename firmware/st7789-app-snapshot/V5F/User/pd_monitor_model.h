#ifndef PD_MONITOR_MODEL_H
#define PD_MONITOR_MODEL_H
#include "pd_capture.h"
#include <string.h>
typedef struct {
    PdCaptureRecord recent[8];
    uint32_t caps[7], frames, bad, last_seq;
    uint16_t mv, ma;
    uint8_t count, next, phase, cap_count, pdo, valid, have_seq, cc;
} PdMonitor;
static void pd_monitor_invalidate(PdMonitor *m)
{
    m->phase=0; m->cap_count=0; m->pdo=0; m->valid=0; m->mv=0; m->ma=0;
}
static uint32_t pd_u32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static const char *pd_monitor_name(const PdCaptureRecord *r)
{
    if(r->kind==PD_RECORD_HARD_RESET) return "HRESET";
    if(r->kind==PD_RECORD_CABLE_RESET) return "CRESET";
    if(r->kind==PD_RECORD_DETACH) return "DETACH";
    if(!(r->flags&PD_CAPTURE_HAS_HEADER)) return "NO HDR";
    if(r->header&0x8000U) return "EXT";
    unsigned n=(r->header>>12)&7U, t=r->header&31U;
    if(n) return t==1?"CAPS":t==2?"REQ":"DATA";
    return t==1?"GCRC":t==3?"ACCEPT":t==6?"PS_RDY":"CTRL";
}
static void pd_monitor_apply(PdMonitor *m, const PdCaptureRecord *r)
{
    if(m->have_seq && r->sequence != m->last_seq+1U) pd_monitor_invalidate(m);
    m->have_seq=1; m->last_seq=r->sequence;
    m->recent[m->next]=*r; m->next=(m->next+1U)%8U;
    if(m->count<8) ++m->count;
    ++m->frames;
    if(r->kind!=PD_RECORD_FRAME) { pd_monitor_invalidate(m); return; }
    if(m->cc && r->cc!=m->cc) pd_monitor_invalidate(m);
    m->cc=r->cc;
    unsigned n=(r->header>>12)&7U, t=r->header&31U, src=(r->header>>8)&1U;
    if(r->crc_status!=PD_CHECK_PASS || r->length_status!=PD_CHECK_PASS ||
       !(r->flags&PD_CAPTURE_HAS_HEADER) || (r->flags&PD_CAPTURE_TRUNCATED) || r->payload_len!=4U*n) {
        ++m->bad; pd_monitor_invalidate(m); return;
    }
    if(r->sop!=PD_SOP_NORMAL || !r->cc) return;
    if(r->header&0x8000U) {pd_monitor_invalidate(m); return;}
    if(n && t==1 && src) {
        pd_monitor_invalidate(m); m->cap_count=n; m->phase=1;
        for(unsigned i=0;i<n;++i) m->caps[i]=pd_u32(r->payload+4*i);
    } else if(n==1 && t==2 && !src) {
        m->valid=0; m->pdo=0; m->phase=m->cap_count?1:0;
        uint32_t rdo=pd_u32(r->payload); unsigned index=(rdo>>28)&7U;
        if(!index || index>m->cap_count || (rdo&(1UL<<27))) return;
        uint32_t pdo=m->caps[index-1];
        if(pdo>>30) return; /* Only fixed supply requests interpreted in v1. */
        m->mv=((pdo>>10)&1023U)*50U; m->ma=((rdo>>10)&1023U)*10U;
        if(m->ma>(pdo&1023U)*10U) return;
        m->pdo=index; m->valid=1; m->phase=2;
    } else if(!n && t==3 && src && m->phase==2) m->phase=3;
    else if(!n && t==6 && src && m->phase==3) m->phase=4;
    else if(!n && t==1) { /* GoodCRC never changes negotiation state. */ }
    else pd_monitor_invalidate(m); /* Unsupported control/data cannot preserve READY. */
}
#endif
