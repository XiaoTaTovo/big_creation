#define Core_V3F 1
#define PD_TRANSPORT_HOST_TEST 1
#include "pd_transport.h"
#include "pd_monitor_model.h"
#include <assert.h>
#include <stdio.h>
unsigned char __pd_shared_start[32768] __attribute__((aligned(16)));
unsigned char __pd_shared_end[1];
static void feed(PdMonitor *m,unsigned begin,unsigned end) {
 for(unsigned i=begin;i<=end;++i) {PdCaptureRecord a,b; PD_Transport_1022Record(&a,i);
 assert(PD_Transport_Push(&a)==PD_SUBMIT_OK); assert(PD_Transport_Pop(&b)==PD_RX_OK); pd_monitor_apply(m,&b);}
}
int main(void) {
 PdMonitor m={0}; PdCaptureRecord r;
 PD_Shared_BootInit(); assert(PD_Transport_Attach());
 feed(&m,1,8); assert(m.phase==4 && m.valid && m.pdo==3 && m.mv==12000 && m.ma==1670);
 assert(m.frames==8 && m.bad==0 && m.count==8);
 for(unsigned i=0;i<8;i++) assert(m.recent[(m.next+i)%8].sequence==i+1);
 assert(!strcmp(pd_monitor_name(&m.recent[0]),"CAPS"));
 feed(&m,9,9); assert(m.phase==1 && !m.valid);
 assert(m.recent[m.next].sequence==2);
 PD_Transport_1022Record(&r,11); pd_monitor_apply(&m,&r); assert(!m.valid && !m.cap_count);
 memset(&m,0,sizeof m); feed(&m,1,2);
 PD_Transport_1022Record(&r,3); r.crc_status=PD_CHECK_FAIL; pd_monitor_apply(&m,&r);
 feed(&m,4,8); assert(!m.valid && m.phase!=4 && m.bad==1);
 memset(&m,0,sizeof m); feed(&m,1,2);
 PD_Transport_1022Record(&r,3); r.payload_len=0; pd_monitor_apply(&m,&r); assert(!m.valid && m.bad==1);
 memset(&m,0,sizeof m); feed(&m,1,8);
 memset(&r,0,sizeof r); r.sequence=9; r.kind=PD_RECORD_DETACH; pd_monitor_apply(&m,&r); assert(!m.valid && !m.phase);
 memset(&m,0,sizeof m); feed(&m,1,8);
 PD_Transport_1022Record(&r,9); r.header|=0x8000; pd_monitor_apply(&m,&r); assert(!m.valid);
 memset(&m,0,sizeof m); feed(&m,1,2);
 PD_Transport_1022Record(&r,3); r.payload[3]=0x73; pd_monitor_apply(&m,&r); assert(!m.valid);
 puts("PASS: queue -> parser, eight rows, wrap, loss, bad CRC/length, detach, extended, invalid PDO");
}
