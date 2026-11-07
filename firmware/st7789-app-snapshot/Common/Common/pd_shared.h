#ifndef PD_SHARED_H
#define PD_SHARED_H
#include "pd_capture.h"
#include <stddef.h>
#include <string.h>
#define PD_SHARED_LAYOUT_VERSION 2U
#define PD_SHARED_SLOT_COUNT 16U
#define PD_SHARED_MAGIC 0x50445348UL
extern unsigned char __pd_shared_start[];
extern unsigned char __pd_shared_end[];
/* Fixed ABI, shared RAM only, not a PC wire format. */
typedef struct { PdCaptureRecord record; uint32_t checksum; } PdSharedSlot;
typedef struct {
    uint32_t magic, layout_version, total_bytes, record_bytes, capacity;
    uint32_t write_seq, read_seq, receiver_ready;
    uint32_t tx_ok, tx_full, tx_invalid, tx_not_ready, high_water;
    uint32_t rx_ok, rx_integrity, rx_invalid, tx_fault, rx_fault;
    uint32_t reserved[2];
    PdSharedSlot slots[PD_SHARED_SLOT_COUNT];
} PdSharedArea;
typedef char pd_record_size_check[sizeof(PdCaptureRecord)==48 ? 1:-1];
typedef char pd_payload_offset_check[offsetof(PdCaptureRecord,payload)==20 ? 1:-1];
typedef char pd_slot_size_check[sizeof(PdSharedSlot)==52 ? 1:-1];
typedef char pd_slots_offset_check[offsetof(PdSharedArea,slots)==80 ? 1:-1];
typedef char pd_area_size_check[sizeof(PdSharedArea)==912 ? 1:-1];
static inline volatile PdSharedArea *PD_Shared_Area(void)
{ return (volatile PdSharedArea *)(void *)__pd_shared_start; }
static inline void PD_Shared_Fence(void)
{
#ifdef PD_TRANSPORT_HOST_TEST
    __asm volatile("" ::: "memory"); /* Host tests do NOT prove hardware ordering. */
#else
    __asm volatile("fence iorw, iorw" ::: "memory");
#endif
}
#ifdef Core_V3F
/* Coordinated cold boot ONLY, before waking V5. No live/single-core restart. */
static inline void PD_Shared_BootInit(void)
{
    volatile PdSharedArea *s=PD_Shared_Area();
    volatile unsigned char *p=(volatile unsigned char *)s;
    s->magic=0; PD_Shared_Fence();
    for(unsigned i=0;i<sizeof(*s);++i) p[i]=0;
    s->layout_version=PD_SHARED_LAYOUT_VERSION;
    s->total_bytes=sizeof(*s); s->record_bytes=sizeof(PdCaptureRecord);
    s->capacity=PD_SHARED_SLOT_COUNT;
    PD_Shared_Fence(); s->magic=PD_SHARED_MAGIC; PD_Shared_Fence();
}
#endif
static inline int PD_Shared_LayoutValid(void)
{
    volatile PdSharedArea *s=PD_Shared_Area();
    if(s->magic!=PD_SHARED_MAGIC) return 0;
    PD_Shared_Fence();
    return s->layout_version==PD_SHARED_LAYOUT_VERSION &&
      s->total_bytes==sizeof(*s) && s->record_bytes==sizeof(PdCaptureRecord) &&
      s->capacity==PD_SHARED_SLOT_COUNT;
}
#endif
