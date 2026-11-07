#ifndef PD_REPLAY_UI_IMPL_H
#define PD_REPLAY_UI_IMPL_H
#include "pd_replay_model.h"
#include "lvgl.h"

/* Included only by main.c so the existing MRS source list needs no edits. */
#ifndef PD_REPLAY_BAD_CRC_DEMO
#define PD_REPLAY_BAD_CRC_DEMO 1
#endif
#define PD_REPLAY_STEP_MS 2000U
static PdDisplayState pd_view_state;
static unsigned pd_next_event, pd_hold_ticks;
static lv_obj_t *pd_phase_label, *pd_message_label, *pd_meta_label;
static lv_obj_t *pd_pdo_label, *pd_voltage_label, *pd_current_label;
static lv_obj_t *pd_stats_label, *pd_quality_label, *pd_progress;

static lv_obj_t *pd_label(lv_obj_t *parent, int y, const char *text, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, 10, y);
    lv_obj_set_width(label, 150);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_label_set_text(label, text);
    return label;
}

static void pd_replay_render(void)
{
    static const char *phase_names[] = {"WAITING", "CAPS RECEIVED", "REQUESTED", "ACCEPTED", "POWER READY"};
    static const char *message_names[] = {"Source Caps", "GoodCRC", "Request", "Accept", "PS_RDY"};
    PdDisplayState *s = &pd_view_state;
    lv_label_set_text(pd_phase_label, phase_names[s->phase]);
    lv_obj_set_style_text_color(pd_phase_label,
        lv_color_hex(s->phase == PD_READY ? 0x22C55E : 0xFBBF24), 0);
    if(s->has_frame) {
        lv_label_set_text(pd_message_label, message_names[s->last_message]);
        lv_label_set_text_fmt(pd_meta_label, "%s ID%u t8:%ums",
            s->last_sender == PD_SRC ? "SRC" : "SNK", (unsigned)s->last_id, (unsigned)s->last_t8);
    } else {
        lv_label_set_text(pd_message_label, "No frame yet");
        lv_label_set_text(pd_meta_label, "-- ID- t8:--");
    }
    if(s->request_valid) {
        lv_label_set_text_fmt(pd_pdo_label, "PDO %u / %u", (unsigned)s->pdo_index, (unsigned)s->caps_count);
        lv_label_set_text_fmt(pd_voltage_label, "REQ V %u.%02u V",
            (unsigned)(s->voltage_mv / 1000U), (unsigned)((s->voltage_mv % 1000U) / 10U));
        lv_label_set_text_fmt(pd_current_label, "REQ I %u.%02u A",
            (unsigned)(s->operating_ma / 1000U), (unsigned)((s->operating_ma % 1000U) / 10U));
    } else {
        lv_label_set_text_fmt(pd_pdo_label, "PDO - / %u", (unsigned)s->caps_count);
        lv_label_set_text(pd_voltage_label, "REQ V --.-- V");
        lv_label_set_text(pd_current_label, "REQ I --.-- A");
    }
    lv_label_set_text_fmt(pd_stats_label, "RX %u/8  BAD %u", (unsigned)s->frames, (unsigned)s->bad_frames);
    lv_label_set_text(pd_quality_label, !s->has_frame ? "CRC --  DROP --" :
        s->last_ok ? "CRC OK DROP --" : "CRC BAD DROP --");
    lv_obj_set_style_text_color(pd_quality_label,
        lv_color_hex(s->has_frame && !s->last_ok ? 0xEF4444 : 0xA0B0C0), 0);
    lv_obj_set_width(pd_progress, s->frames ? (int32_t)(s->frames * 150U / PD_REPLAY_COUNT) : 1);
}

/* Called by lv_timer_handler(), not by the hardware timer ISR. */
static void pd_replay_tick(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if(pd_next_event < PD_REPLAY_COUNT) {
        PdReplayEvent event = pd_replay_events[pd_next_event++];
#if PD_REPLAY_BAD_CRC_DEMO
        if(event.message == PD_REQUEST) event.crc_ok = 0; /* Synthetic, not log data. */
#endif
        pd_display_apply(&pd_view_state, &event);
    } else {
        if(++pd_hold_ticks < 3U) return; /* Keep the final frame visible for 6 seconds. */
        pd_display_reset(&pd_view_state);
        pd_next_event = pd_hold_ticks = 0;
    }
    pd_replay_render();
}

static void ui_pd_replay_create(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    pd_label(screen, 8, "PD XiaoT", 0xFFFFFF);
    pd_label(screen, 29, PD_REPLAY_BAD_CRC_DEMO ? "TEST: BAD CRC" : "REPLAY: 1022", 0x22D3EE);
    pd_label(screen, 50, "CC1 / LISTEN", 0x8090A0);
    pd_phase_label = pd_label(screen, 77, "", 0xFBBF24);
    pd_message_label = pd_label(screen, 101, "", 0xFFFFFF);
    pd_meta_label = pd_label(screen, 123, "", 0xA0B0C0);
    pd_pdo_label = pd_label(screen, 152, "", 0x22D3EE);
    pd_voltage_label = pd_label(screen, 176, "", 0xFFFFFF);
    pd_current_label = pd_label(screen, 200, "", 0xFFFFFF);
    pd_stats_label = pd_label(screen, 231, "", 0xA0B0C0);
    pd_quality_label = pd_label(screen, 253, "", 0xA0B0C0);
    pd_progress = lv_obj_create(screen);
    lv_obj_remove_style_all(pd_progress);
    lv_obj_set_pos(pd_progress, 10, 279);
    lv_obj_set_size(pd_progress, 1, 6);
    lv_obj_set_style_bg_color(pd_progress, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_bg_opa(pd_progress, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(pd_progress, false);
    pd_label(screen, 295, "REQ = requested", 0x8090A0);
    pd_display_reset(&pd_view_state);
    pd_next_event = pd_hold_ticks = 0;
    pd_replay_render();
    lv_timer_create(pd_replay_tick, PD_REPLAY_STEP_MS, NULL);
}
#endif
