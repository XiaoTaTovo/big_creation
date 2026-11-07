#ifndef PD_REPLAY_MODEL_H
#define PD_REPLAY_MODEL_H
#include <stdint.h>
#include <string.h>

/* Local replay model, NOT a frozen V3/V5 wire format. No hardware or LVGL. */
typedef enum { PD_CAPS, PD_GOODCRC, PD_REQUEST, PD_ACCEPT, PD_PS_RDY } PdMessage;
typedef enum { PD_WAITING, PD_HAVE_CAPS, PD_REQUESTED, PD_ACCEPTED, PD_READY } PdPhase;
typedef enum { PD_SRC, PD_SNK } PdSender;
typedef struct {
    PdMessage message;
    PdSender sender;
    uint8_t id, t8, crc_ok, len_ok, ndo;
    uint32_t data[5];
} PdReplayEvent;
typedef struct {
    PdPhase phase;
    PdMessage last_message;
    PdSender last_sender;
    uint8_t last_id, last_t8, has_frame, last_ok;
    uint8_t caps_count, pdo_index, request_valid;
    uint16_t voltage_mv, operating_ma, maximum_ma;
    uint16_t frames, bad_frames;
    uint32_t caps[5];
} PdDisplayState;

/* Transcribed from 1022LISTEN.txt. All frames CC1/SOP, listener sends nothing.
 * t8 is retained verbatim in milliseconds. CRC/length are logger results.
 * The printed revision field is intentionally omitted pending verification. */
static const PdReplayEvent pd_replay_events[] = {
    {PD_CAPS,    PD_SRC, 0,   5, 1, 1, 5, {0x0801912CUL, 0x0002D0DEUL, 0x0003C0A7UL, 0xC076213CUL, 0xC0DC2128UL}},
    {PD_GOODCRC, PD_SNK, 0,   6, 1, 1, 0, {0}},
    {PD_REQUEST, PD_SNK, 0,   9, 1, 1, 1, {0x33029CA7UL}},
    {PD_GOODCRC, PD_SRC, 0,   9, 1, 1, 0, {0}},
    {PD_ACCEPT,  PD_SRC, 1,  11, 1, 1, 0, {0}},
    {PD_GOODCRC, PD_SNK, 1,  11, 1, 1, 0, {0}},
    {PD_PS_RDY,  PD_SRC, 2, 166, 1, 1, 0, {0}},
    {PD_GOODCRC, PD_SNK, 2, 167, 1, 1, 0, {0}}
};
#define PD_REPLAY_COUNT (sizeof(pd_replay_events) / sizeof(pd_replay_events[0]))

static void pd_display_reset(PdDisplayState *s)
{
    memset(s, 0, sizeof(*s));
    s->phase = PD_WAITING;
}

/* A bounded reducer for this one fixed-PDO negotiation; not a complete PD parser. */
static void pd_display_apply(PdDisplayState *s, const PdReplayEvent *e)
{
    s->frames++;
    s->has_frame = 1;
    s->last_message = e->message;
    s->last_sender = e->sender;
    s->last_id = e->id;
    s->last_t8 = e->t8;
    s->last_ok = e->crc_ok && e->len_ok;
    if(!s->last_ok) { s->bad_frames++; return; }
    switch(e->message) {
    case PD_CAPS:
        if(e->sender != PD_SRC || e->ndo == 0 || e->ndo > 5) break;
        memcpy(s->caps, e->data, sizeof(s->caps));
        s->caps_count = e->ndo;
        s->request_valid = 0;
        s->pdo_index = 0;
        s->voltage_mv = s->operating_ma = s->maximum_ma = 0;
        s->phase = PD_HAVE_CAPS;
        break;
    case PD_REQUEST: {
        uint32_t rdo = e->data[0];
        uint8_t index = (uint8_t)((rdo >> 28) & 7U);
        if(e->sender != PD_SNK || e->ndo != 1 || s->phase < PD_HAVE_CAPS) break;
        s->request_valid = 0;
        s->pdo_index = 0;
        s->voltage_mv = s->operating_ma = s->maximum_ma = 0;
        s->phase = PD_HAVE_CAPS;
        if(index == 0 || index > s->caps_count) break;
        uint32_t pdo = s->caps[index - 1];
        if((pdo >> 30) != 0 || (rdo & (1UL << 27))) break; /* Fixed, no GiveBack only. */
        s->pdo_index = index;
        s->voltage_mv = (uint16_t)(((pdo >> 10) & 0x3ffU) * 50U);
        s->operating_ma = (uint16_t)(((rdo >> 10) & 0x3ffU) * 10U);
        s->maximum_ma = (uint16_t)((rdo & 0x3ffU) * 10U);
        s->request_valid = 1;
        s->phase = PD_REQUESTED;
        break;
    }
    case PD_ACCEPT:
        if(e->sender == PD_SRC && e->ndo == 0 && s->phase == PD_REQUESTED)
            s->phase = PD_ACCEPTED;
        break;
    case PD_PS_RDY:
        if(e->sender == PD_SRC && e->ndo == 0 && s->phase == PD_ACCEPTED)
            s->phase = PD_READY;
        break;
    case PD_GOODCRC:
        break; /* Acknowledgement must never undo negotiation progress. */
    }
}
#endif
