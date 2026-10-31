#include <assert.h>
#include <stdio.h>
#include "pd_replay_model.h"

int main(void)
{
    PdDisplayState s;
    const PdPhase expected[] = {PD_HAVE_CAPS, PD_HAVE_CAPS, PD_REQUESTED,
        PD_REQUESTED, PD_ACCEPTED, PD_ACCEPTED, PD_READY, PD_READY};
    pd_display_reset(&s);
    assert(!s.request_valid && !s.has_frame && s.phase == PD_WAITING);
    for(unsigned i = 0; i < PD_REPLAY_COUNT; ++i) {
        pd_display_apply(&s, &pd_replay_events[i]);
        assert(s.phase == expected[i] && s.frames == i + 1 && s.bad_frames == 0);
        assert(s.request_valid == (i >= 2));
        if(i >= 2) {
            assert(s.pdo_index == 3 && s.voltage_mv == 12000);
            assert(s.operating_ma == 1670 && s.maximum_ma == 1670);
        }
    }
    assert(s.last_message == PD_GOODCRC && s.last_t8 == 167);
    pd_display_reset(&s);
    for(unsigned i = 0; i < PD_REPLAY_COUNT; ++i) {
        PdReplayEvent e = pd_replay_events[i];
        if(i == 2) e.crc_ok = 0;
        pd_display_apply(&s, &e);
    }
    assert(s.bad_frames == 1 && !s.request_valid && s.phase == PD_HAVE_CAPS);
    pd_display_reset(&s);
    pd_display_apply(&s, &pd_replay_events[2]); /* Request with no capabilities. */
    pd_display_apply(&s, &pd_replay_events[6]); /* PS_RDY with no accepted request. */
    assert(s.phase == PD_WAITING && !s.request_valid);
    pd_display_apply(&s, &pd_replay_events[0]);
    PdReplayEvent e = pd_replay_events[2];
    e.data[0] = 0x63029CA7UL; /* Index beyond the five captured PDOs. */
    pd_display_apply(&s, &e);
    assert(!s.request_valid);
    e.data[0] = 0x43029CA7UL; /* Unsupported APDO must not be decoded as Fixed. */
    pd_display_apply(&s, &e);
    assert(!s.request_valid);
    pd_display_apply(&s, &pd_replay_events[2]);
    e = pd_replay_events[4];
    e.len_ok = 0;
    pd_display_apply(&s, &e);
    assert(s.phase == PD_REQUESTED && s.bad_frames == 1);
    e = pd_replay_events[4];
    e.sender = PD_SNK;
    pd_display_apply(&s, &e);
    assert(s.phase == PD_REQUESTED);
    pd_display_apply(&s, &pd_replay_events[0]);
    assert(s.phase == PD_HAVE_CAPS && !s.request_valid && s.voltage_mv == 0);
    pd_display_reset(&s);
    assert(s.frames == 0 && s.bad_frames == 0 && s.caps_count == 0);
    puts("PASS: eight frames, request values, GoodCRC, bad CRC/length, order, PDO and reset");
    return 0;
}
