#define Core_V3F 1
#define PD_TRANSPORT_HOST_TEST 1
#include "pd_transport.h"
#include <assert.h>
#include <stdio.h>
unsigned char __pd_shared_start[32768] __attribute__((aligned(16)));
unsigned char __pd_shared_end[1];
static void reset(void){PD_Shared_BootInit();assert(PD_Transport_Attach());}
int main(void){
    volatile PdSharedArea *s=PD_Shared_Area();
    PdCaptureRecord a,b,expected; unsigned cases=0;
    memset(&a,0,sizeof a);assert(PD_Record_Checksum(&a)==0xF288B395UL);++cases; /* zlib reference */
    PD_Shared_BootInit(); PD_Transport_DemoRecord(&a,1);
    assert(PD_Transport_Push(&a)==PD_SUBMIT_NOT_READY); assert(!s->write_seq); ++cases;
    assert(PD_Transport_Attach()); assert(PD_Transport_Pop(&b)==PD_RX_EMPTY); ++cases;
    assert(PD_Transport_Push(&a)==PD_SUBMIT_OK); a.payload[0]=99;
    assert(PD_Transport_Pop(&b)==PD_RX_OK); PD_Transport_DemoRecord(&expected,1);
    assert(!memcmp(&b,&expected,sizeof b));assert(s->read_seq==1);++cases;
    reset();for(unsigned i=0;i<16;i++){PD_Transport_DemoRecord(&a,i);assert(PD_Transport_Push(&a)==PD_SUBMIT_OK);}
    assert(PD_Transport_Push(&a)==PD_SUBMIT_FULL);assert(s->high_water==16 && s->tx_full==1);
    for(unsigned i=0;i<16;i++){assert(PD_Transport_Pop(&b)==PD_RX_OK);assert(b.sequence==i);}
    assert(PD_Transport_Push(&a)==PD_SUBMIT_OK);assert(PD_Transport_Pop(&b)==PD_RX_OK);++cases;
    reset(); s->write_seq=s->read_seq=0xFFFFFFF8U;
    for(unsigned i=0;i<1000;i++){PD_Transport_DemoRecord(&a,i);assert(PD_Transport_Push(&a)==0);assert(PD_Transport_Pop(&b)==PD_RX_OK);assert(!memcmp(&a,&b,sizeof a));}++cases;
    reset();PD_Transport_DemoRecord(&a,2);
    assert(PD_Transport_Push(NULL)==PD_SUBMIT_INVALID);
    for(unsigned f=0;f<11;f++){
        b=a;
        switch(f){case 0:b.payload_len=29;break;case 1:b.cc=3;break;case 2:b.sop=4;break;
        case 3:b.kind=0;break;case 4:b.flags|=128;break;case 5:b.crc_status=3;break;
        case 6:b.length_status=3;break;case 7:b.reserved[0]=1;break;case 8:b.timestamp_us=1;break;
        case 9:b.flags=0;break;case 10:b.payload_len=0;break;}
        assert(PD_Transport_Push(&b)==PD_SUBMIT_INVALID);
    }assert(s->write_seq==0);++cases;
    a.crc_status=PD_CHECK_FAIL;a.length_status=PD_CHECK_FAIL;a.flags|=PD_CAPTURE_TRUNCATED;
    assert(PD_Transport_Push(&a)==0);assert(PD_Transport_Pop(&b)==PD_RX_OK);assert(!memcmp(&a,&b,sizeof a));++cases;
    memset(&a,0,sizeof a);a.kind=PD_RECORD_HARD_RESET;
    assert(PD_Transport_Push(&a)==0);assert(PD_Transport_Pop(&b)==PD_RX_OK);
    a.payload_len=1;assert(PD_Transport_Push(&a)==PD_SUBMIT_INVALID);++cases;
    reset();PD_Transport_DemoRecord(&a,3);assert(PD_Transport_Push(&a)==0);
    s->slots[0].record.payload[5]^=1;b.sequence=12345;
    assert(PD_Transport_Pop(&b)==PD_RX_DROPPED);assert(b.sequence==12345 && s->rx_integrity==1 && s->read_seq==1);
    assert(PD_Transport_Push(&a)==0);assert(PD_Transport_Pop(&b)==PD_RX_OK);++cases;
    reset();assert(PD_Transport_Push(&a)==0);a.cc=9;s->slots[0].record=a;s->slots[0].checksum=PD_Record_Checksum(&a);
    assert(PD_Transport_Pop(&b)==PD_RX_DROPPED);assert(s->rx_invalid==1);++cases;
    reset();s->layout_version=1;assert(!PD_Transport_Attach());assert(PD_Transport_Pop(&b)==PD_RX_NOT_READY);++cases;
    reset();s->write_seq=17;assert(PD_Transport_Pop(&b)==PD_RX_FAULT);assert(s->rx_fault && s->read_seq==0);++cases;
    reset();s->read_seq=1;PD_Transport_DemoRecord(&a,9);assert(PD_Transport_Push(&a)==PD_SUBMIT_NOT_READY);assert(s->tx_fault);++cases;
    reset(); unsigned produced=0,consumed=0,seed=123;
    for(unsigned n=0;n<200000;n++){
        seed=seed*1664525U+1013904223U;
        if(seed&0x100U){PD_Transport_DemoRecord(&a,produced);PdSubmitResult x=PD_Transport_Push(&a);if(x==0)produced++;else assert(x==PD_SUBMIT_FULL);}
        else {PdReceiveResult x=PD_Transport_Pop(&b);if(x==PD_RX_OK){assert(b.sequence==consumed++);PD_Transport_DemoRecord(&expected,b.sequence);assert(!memcmp(&b,&expected,sizeof b));}else assert(x==PD_RX_EMPTY);}
    }
    while(PD_Transport_Pop(&b)==PD_RX_OK)assert(b.sequence==consumed++);
    assert(produced==consumed);++cases;
    printf("PASS %u groups; randomized FIFO delivered %u records\n",cases,consumed);
    return 0;
}
