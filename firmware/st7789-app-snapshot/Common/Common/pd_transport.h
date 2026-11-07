#ifndef PD_TRANSPORT_H
#define PD_TRANSPORT_H
#include "pd_shared.h"
typedef enum { PD_RX_EMPTY, PD_RX_OK, PD_RX_DROPPED, PD_RX_NOT_READY, PD_RX_FAULT } PdReceiveResult;
/* Shape validation only. Malformed PD contents remain evidence, not API errors. */
static inline int PD_Record_Valid(const PdCaptureRecord *r)
{
    if(!r || r->kind<1 || r->kind>4 || r->cc>2 || r->sop>3 ||
       r->crc_status>2 || r->length_status>2 || r->payload_len>28 ||
       (r->flags & ~7U)) return 0;
    if(!(r->flags&PD_CAPTURE_HAS_HEADER) && r->header) return 0;
    if(!(r->flags&PD_CAPTURE_HAS_TIME) && r->timestamp_us) return 0;
    for(unsigned i=0;i<3;++i) if(r->reserved[i]) return 0;
    for(unsigned i=r->payload_len;i<28;++i) if(r->payload[i]) return 0;
    if(r->kind!=PD_RECORD_FRAME && (r->header || r->payload_len ||
       (r->flags&(PD_CAPTURE_HAS_HEADER|PD_CAPTURE_TRUNCATED)) ||
       r->crc_status || r->length_status)) return 0;
    return 1;
}
/* CRC-32/ISO-HDLC over fixed 48-byte ABI; independent of captured PD CRC. */
static inline uint32_t PD_Record_Checksum(const PdCaptureRecord *r)
{
    const unsigned char *p=(const unsigned char *)r;
    uint32_t crc=0xFFFFFFFFUL;
    for(unsigned i=0;i<sizeof(*r);++i) {
        crc^=p[i];
        for(unsigned j=0;j<8;++j) crc=(crc>>1)^((0U-(crc&1U))&0xEDB88320UL);
    }
    return ~crc;
}
/* V5 attaches once after display init; no queue clearing or reset. */
static inline int PD_Transport_Attach(void)
{
    volatile PdSharedArea *s=PD_Shared_Area();
    if(!PD_Shared_LayoutValid() || s->tx_fault || s->rx_fault) return 0;
    PD_Shared_Fence(); s->receiver_ready=1; PD_Shared_Fence(); return 1;
}
/* ONLY V3 writes write_seq / tx statistics. No waiting, no overwrite. */
static inline PdSubmitResult PD_Transport_Push(const PdCaptureRecord *r)
{
    volatile PdSharedArea *s=PD_Shared_Area();
    if(!PD_Shared_LayoutValid()) return PD_SUBMIT_NOT_READY;
    if(!PD_Record_Valid(r)) { ++s->tx_invalid; return PD_SUBMIT_INVALID; }
    if(s->tx_fault || s->rx_fault || s->receiver_ready!=1) {
        ++s->tx_not_ready; return PD_SUBMIT_NOT_READY;
    }
    uint32_t w=s->write_seq, used=w-s->read_seq;
    PD_Shared_Fence();
    if(used>PD_SHARED_SLOT_COUNT) {s->tx_fault=1;return PD_SUBMIT_NOT_READY;}
    if(used==PD_SHARED_SLOT_COUNT) {++s->tx_full;return PD_SUBMIT_FULL;}
    volatile PdSharedSlot *slot=&s->slots[w&(PD_SHARED_SLOT_COUNT-1U)];
    uint32_t crc=PD_Record_Checksum(r);
    const unsigned char *src=(const unsigned char *)r;
    volatile unsigned char *dst=(volatile unsigned char *)&slot->record;
    for(unsigned i=0;i<sizeof(*r);++i) dst[i]=src[i];
    slot->checksum=crc;
    PD_Shared_Fence(); s->write_seq=w+1U; PD_Shared_Fence();
    ++s->tx_ok;
    if(used+1U>s->high_water) s->high_water=used+1U;
    return PD_SUBMIT_OK;
}
/* ONLY V5 writes read_seq / rx statistics. Copy before releasing slot.
 * Corrupt records are consumed and counted, never delivered to caller.
 */
static inline PdReceiveResult PD_Transport_Pop(PdCaptureRecord *out)
{
    volatile PdSharedArea *s=PD_Shared_Area();
    if(!out || !PD_Shared_LayoutValid() || s->receiver_ready!=1) return PD_RX_NOT_READY;
    if(s->tx_fault || s->rx_fault) return PD_RX_FAULT;
    uint32_t r=s->read_seq, available=s->write_seq-r;
    PD_Shared_Fence();
    if(available>PD_SHARED_SLOT_COUNT) {s->rx_fault=1;return PD_RX_FAULT;}
    if(!available) return PD_RX_EMPTY;
    volatile PdSharedSlot *slot=&s->slots[r&(PD_SHARED_SLOT_COUNT-1U)];
    PdCaptureRecord copy;
    unsigned char *dst=(unsigned char *)&copy;
    volatile const unsigned char *src=(volatile const unsigned char *)&slot->record;
    for(unsigned i=0;i<sizeof(copy);++i) dst[i]=src[i];
    uint32_t crc=slot->checksum;
    PD_Shared_Fence(); s->read_seq=r+1U; PD_Shared_Fence(); /* ACK */
    if(PD_Record_Checksum(&copy)!=crc) {++s->rx_integrity;return PD_RX_DROPPED;}
    if(!PD_Record_Valid(&copy)) {++s->rx_invalid;return PD_RX_DROPPED;}
    *out=copy; ++s->rx_ok; return PD_RX_OK;
}
/* Offline replay transcribed from 1022LISTEN.txt.
 * The source log is not a live sensor. t8 is intentionally not placed in
 * timestamp_us uses t8 milliseconds converted to microseconds. Header is little-endian
 * from the first two raw bytes; payload excludes the four-byte captured CRC. */
static inline uint32_t PD_Transport_1022T8Ms(unsigned index)
{
    static const uint8_t t8_ms[8] = {5, 6, 9, 9, 11, 11, 166, 167};
    return t8_ms[index & 7U];
}

static inline void PD_Transport_1022Record(PdCaptureRecord *r, uint32_t sequence)
{
    static const uint8_t t8_ms[8] = {5, 6, 9, 9, 11, 11, 166, 167};
    static const uint16_t headers[8] = {
        0x51A1, 0x0041, 0x1042, 0x0161,
        0x0363, 0x0241, 0x0566, 0x0441
    };
    static const uint8_t lengths[8] = {20, 0, 4, 0, 0, 0, 0, 0};
    static const uint8_t payloads[8][20] = {
        {0x2C,0x91,0x01,0x08, 0xDE,0xD0,0x02,0x00,
         0xA7,0xC0,0x03,0x00, 0x3C,0x21,0x76,0xC0,
         0x28,0x21,0xDC,0xC0},
        {0},
        {0xA7,0x9C,0x02,0x33},
        {0}, {0}, {0}, {0}, {0}
    };
    unsigned index = (unsigned)((sequence - 1U) & 7U);
    memset(r, 0, sizeof(*r));
    r->sequence = sequence;
    r->kind = PD_RECORD_FRAME;
    r->flags = PD_CAPTURE_HAS_HEADER | PD_CAPTURE_HAS_TIME;
    r->timestamp_us = (uint32_t)t8_ms[index] * 1000U;
    r->header = headers[index];
    r->cc = 1;
    r->sop = PD_SOP_NORMAL;
    r->crc_status = PD_CHECK_PASS;
    r->length_status = PD_CHECK_PASS;
    r->payload_len = lengths[index];
    for(unsigned i = 0; i < r->payload_len; ++i)
        r->payload[i] = payloads[index][i];
}

/* Synthetic fixture, NOT a captured PD message. No claimed timestamp. */
static inline void PD_Transport_DemoRecord(PdCaptureRecord *r,uint32_t seq)
{
    memset(r,0,sizeof(*r)); r->sequence=seq; r->kind=PD_RECORD_FRAME;
    r->flags=PD_CAPTURE_HAS_HEADER; r->header=0x7123;
    r->cc=1; r->sop=PD_SOP_NORMAL; r->payload_len=28;
    for(unsigned i=0;i<28;++i) r->payload[i]=(uint8_t)(seq+i);
}
#endif
