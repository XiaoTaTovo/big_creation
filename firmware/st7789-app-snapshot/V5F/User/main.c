/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2025/03/01
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/



#include "debug.h"
#include "pd_capture.h"
#include "pd_shared.h"
#include "pd_transport.h"
#include "lcd.h"

#include "lvgl.h"
#include "lv_port_disp.h"
/* No dependency on the LVGL examples module. */
#define UI_DEMO_BASIC_DRAW 1
/* 1: replay; 0: original basic/clock selection above. */
#define UI_DEMO_PD_REPLAY 0

#define UI_DEMO_IPC_COUNTER 0
#define UI_PD_MONITOR 1
/* Test-only: set 2500 to pause receiving once after attach; default 0. */
#define PD_DEMO_HOLD_RX_MS 0U

#if UI_DEMO_PD_REPLAY && !UI_DEMO_IPC_COUNTER
#include "pd_replay_ui_impl.h"
#endif



static void LVGL_TIM2_Init(void)
{
    RCC_ClocksTypeDef clocks = {0};
    TIM_TimeBaseInitTypeDef tim = {0};

    RCC_GetClocksFreq(&clocks);

    /* H417 �� TIM2 ʹ�� HB1 ����ʱ�� */
    RCC_HB1PeriphClockCmd(RCC_HB1Periph_TIM2, ENABLE);

    TIM_Cmd(TIM2, DISABLE);

    /*
     * ������Ƶ�ʷ�Ƶ�� 1MHz��
     * ÿ����һ��Ϊ 1us������ 1000 ��Ϊ 1ms��
     * Ҫ��ʱ��Ƶ���ܱ� 1000000 ������
     */
    tim.TIM_Prescaler =
        (uint16_t)(clocks.HCLK_Frequency / 1000000UL - 1);
    tim.TIM_Period = 1000 - 1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM2, &tim);

    TIM_SetCounter(TIM2, 0);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    NVIC_SetPriority(TIM2_IRQn, 1);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

/* One revolution per minute, based on the LVGL millisecond tick. */
static lv_obj_t *clock_hand;
static lv_obj_t *clock_seconds;
static lv_point_precise_t hand_points[2];
static lv_point_precise_t tick_points[12][2];
static uint32_t clock_start;
static uint32_t clock_last_second;

static void clock_update(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    uint32_t second = ((uint32_t)(lv_tick_get() - clock_start) / 1000U) % 60U;
    if(second == clock_last_second) return;
    clock_last_second = second;

    int16_t angle = (int16_t)(second * 6U);
    lv_obj_invalidate(clock_hand);
    hand_points[1].x = 85 + lv_trigo_sin(angle) * 52 / LV_TRIGO_SIN_MAX;
    hand_points[1].y = 145 - lv_trigo_cos(angle) * 52 / LV_TRIGO_SIN_MAX;
    lv_line_set_points(clock_hand, hand_points, 2);

    /* Fixed-size text, without pulling in formatted printing. */
    char text[] = "00 s";
    text[0] = (char)('0' + second / 10U);
    text[1] = (char)('0' + second % 10U);
    lv_label_set_text(clock_seconds, text);
}

#if !UI_DEMO_BASIC_DRAW
static void ui_test_create(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "60 SECOND TEST");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

    lv_obj_t *dial = lv_obj_create(screen);
    lv_obj_remove_style_all(dial);
    lv_obj_set_pos(dial, 10, 70);
    lv_obj_set_size(dial, 150, 150);
    lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dial, 2, 0);
    lv_obj_set_style_border_color(dial, lv_color_hex(0x8090A0), 0);
    lv_obj_set_style_border_opa(dial, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(dial, false);
    lv_obj_set_clickable(dial, false);

    /* Twelve marks, five seconds per mark. All points must remain alive. */
    for(int32_t i = 0; i < 12; i++) {
        int32_t sx = lv_trigo_sin((int16_t)(i * 30));
        int32_t cy = lv_trigo_cos((int16_t)(i * 30));
        int32_t inner = (i % 3 == 0) ? 57 : 62;
        tick_points[i][0].x = 85 + sx * inner / LV_TRIGO_SIN_MAX;
        tick_points[i][0].y = 145 - cy * inner / LV_TRIGO_SIN_MAX;
        tick_points[i][1].x = 85 + sx * 68 / LV_TRIGO_SIN_MAX;
        tick_points[i][1].y = 145 - cy * 68 / LV_TRIGO_SIN_MAX;
        lv_obj_t *mark = lv_line_create(screen);
        lv_obj_set_pos(mark, 0, 0);
        lv_line_set_points(mark, tick_points[i], 2);
        lv_obj_set_style_line_color(mark, lv_color_hex(0xA0B0C0), 0);
        lv_obj_set_style_line_width(mark, 2, 0);
    }

    hand_points[0].x = 85;
    hand_points[0].y = 145;
    hand_points[1].x = 85;
    hand_points[1].y = 93;
    clock_hand = lv_line_create(screen);
    lv_obj_set_pos(clock_hand, 0, 0);
    lv_line_set_points(clock_hand, hand_points, 2);
    lv_obj_set_style_line_color(clock_hand, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_line_width(clock_hand, 3, 0);
    lv_obj_set_style_line_rounded(clock_hand, true, 0);

    lv_obj_t *hub = lv_obj_create(screen);
    lv_obj_remove_style_all(hub);
    lv_obj_set_pos(hub, 81, 141);
    lv_obj_set_size(hub, 8, 8);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(hub, false);

    clock_seconds = lv_label_create(screen);
    lv_label_set_text(clock_seconds, "00 s");
    lv_obj_set_style_text_color(clock_seconds, lv_color_hex(0x22D3EE), 0);
    lv_obj_align(clock_seconds, LV_ALIGN_TOP_MID, 0, 245);

    clock_start = lv_tick_get();
    clock_last_second = 0;
    lv_timer_create(clock_update, 50, NULL);
}

#endif

/*
 * A deliberately small LVGL drawing exercise.
 * It contains only text, rectangles, a circle, and lines.
 * Keep this page simple first; protocol data will be connected later.
 */
static void ui_basic_draw_create(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    /* Text: title and subtitle. */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "PD XiaoT");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "XiaoT DRAW TEST");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x8090A0), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 35);

    /* Rectangle: a status card. */
    lv_obj_t *card = lv_obj_create(screen);
    lv_obj_remove_style_all(card);
    lv_obj_set_pos(card, 10, 65);
    lv_obj_set_size(card, 150, 62);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E3040), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x426070), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(card, false);

    /* Circle: a simple status indicator. */
    lv_obj_t *status_dot = lv_obj_create(card);
    lv_obj_remove_style_all(status_dot);
    lv_obj_set_pos(status_dot, 12, 20);
    lv_obj_set_size(status_dot, 18, 18);
    lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(status_dot, lv_color_hex(0x22C55E), 0);
    lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(status_dot, false);

    lv_obj_t *state = lv_label_create(card);
    lv_label_set_text(state, "WAITING");
    lv_obj_set_style_text_color(state, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(state, 40, 10);

    lv_obj_t *cc = lv_label_create(card);
    lv_label_set_text(cc, "CC1   LISTEN");
    lv_obj_set_style_text_color(cc, lv_color_hex(0xA0B0C0), 0);
    lv_obj_set_pos(cc, 40, 34);

    /* Line: separator between the status card and values. */
    static lv_point_precise_t divider_points[2] = {
        {10, 145},
        {160, 145}
    };
    lv_obj_t *divider = lv_line_create(screen);
    lv_obj_set_pos(divider, 0, 0);
    lv_line_set_points(divider, divider_points, 2);
    lv_obj_set_style_line_color(divider, lv_color_hex(0x426070), 0);
    lv_obj_set_style_line_width(divider, 2, 0);

    lv_obj_t *values_title = lv_label_create(screen);
    lv_label_set_text(values_title, "PD DATA");
    lv_obj_set_style_text_color(values_title, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_pos(values_title, 10, 155);

    lv_obj_t *voltage = lv_label_create(screen);
    lv_label_set_text(voltage, "DEMO V  5.00 V");
    lv_obj_set_style_text_color(voltage, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(voltage, 10, 178);

    lv_obj_t *current = lv_label_create(screen);
    lv_label_set_text(current, "DEMO I  0.90 A");
    lv_obj_set_style_text_color(current, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(current, 10, 202);

    /* Rectangles: three simple value bars. */
    lv_obj_t *bar1 = lv_obj_create(screen);
    lv_obj_remove_style_all(bar1);
    lv_obj_set_pos(bar1, 10, 230);
    lv_obj_set_size(bar1, 120, 12);
    lv_obj_set_style_radius(bar1, 4, 0);
    lv_obj_set_style_bg_color(bar1, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_bg_opa(bar1, LV_OPA_COVER, 0);

    lv_obj_t *bar2 = lv_obj_create(screen);
    lv_obj_remove_style_all(bar2);
    lv_obj_set_pos(bar2, 10, 248);
    lv_obj_set_size(bar2, 90, 12);
    lv_obj_set_style_radius(bar2, 4, 0);
    lv_obj_set_style_bg_color(bar2, lv_color_hex(0x60A5FA), 0);
    lv_obj_set_style_bg_opa(bar2, LV_OPA_COVER, 0);

    lv_obj_t *bar3 = lv_obj_create(screen);
    lv_obj_remove_style_all(bar3);
    lv_obj_set_pos(bar3, 10, 266);
    lv_obj_set_size(bar3, 60, 12);
    lv_obj_set_style_radius(bar3, 4, 0);
    lv_obj_set_style_bg_color(bar3, lv_color_hex(0xA78BFA), 0);
    lv_obj_set_style_bg_opa(bar3, LV_OPA_COVER, 0);

    /* Line: a small arrow-like polyline. */
    static lv_point_precise_t arrow_points[5] = {
        {100, 215},
        {145, 215},
        {135, 205},
        {145, 215},
        {135, 225}
    };
    lv_obj_t *arrow = lv_line_create(screen);
    lv_obj_set_pos(arrow, 0, 0);
    lv_line_set_points(arrow, arrow_points, 5);
    lv_obj_set_style_line_color(arrow, lv_color_hex(0xFBBF24), 0);
    lv_obj_set_style_line_width(arrow, 3, 0);
    lv_obj_set_style_line_rounded(arrow, true, 0);

    lv_obj_t *footer = lv_label_create(screen);
    lv_label_set_text(footer, "TEXT + SHAPES");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x8090A0), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -12);
}


#include "pd_monitor_ui.h"

static lv_obj_t *ipc_status_label;
static lv_obj_t *ipc_frame_label;
static lv_obj_t *ipc_check_label;
static lv_obj_t *ipc_stats_label;
static uint32_t pd_attached_ms;

static lv_obj_t *ui_ipc_text(lv_obj_t *screen, int y, uint32_t color)
{
    lv_obj_t *label = lv_label_create(screen);
    lv_obj_set_width(label, LCD_W - 16);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
    return label;
}

static void ui_ipc_counter_create(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    ipc_status_label = ui_ipc_text(screen, 8, 0xFFFFFF);
    ipc_frame_label = ui_ipc_text(screen, 42, 0xD7F9FF);
    ipc_check_label = ui_ipc_text(screen, 76, 0xB7C9D6);
    ipc_stats_label = ui_ipc_text(screen, 116, 0xFFFFFF);
    lv_label_set_text(ipc_status_label, "PD MONITOR\nSHM ATTACHING");
    lv_label_set_text(ipc_frame_label, "NO FRAME");
    lv_label_set_text(ipc_check_label, "CC --  SOP --  CRC --  LEN --");
    lv_label_set_text(ipc_stats_label, "TX 0  RX 0  Q 0  DROP 0");
}

static void PD_Transport_Poll(void)
{
    static uint32_t last_ui, last_read, last_progress, rate_ms, rate_count, rx_rate;
    static uint32_t max_poll_gap, prev_poll, invalid_count;
    static uint8_t polled, have_frame;
    static uint32_t observed_full;
    static PdCaptureRecord last_record;
    volatile PdSharedArea *s = PD_Shared_Area();
    uint32_t entered = lv_tick_get();
    if(s->tx_full != observed_full || s->tx_fault || s->rx_fault) {
        pd_monitor_invalidate(&monitor); observed_full=s->tx_full;
    }
    if(polled && (uint32_t)(entered - prev_poll) > max_poll_gap) max_poll_gap = entered - prev_poll;
    polled = 1; prev_poll = entered;
    IPC_ClearFlagStatus(IPC_CH0, IPC_CH_Sta_Bit0);
    for(unsigned i = 0; i < 4 && (uint32_t)(entered - pd_attached_ms) >= PD_DEMO_HOLD_RX_MS; ++i)
    {
        PdCaptureRecord record;
        PdReceiveResult result = PD_Transport_Pop(&record);
        if(result == PD_RX_DROPPED) { invalid_count = s->rx_invalid; pd_monitor_invalidate(&monitor); continue; }
        if(result != PD_RX_OK) break;
        last_record = record; have_frame = 1;
        pd_monitor_apply(&monitor, &record);
    }
    uint32_t now = lv_tick_get();
    if(s->read_seq != last_read) { last_read = s->read_seq; last_progress = now; }
    if((uint32_t)(now - rate_ms) >= 1000U) {
        uint32_t count = s->rx_ok;
        rx_rate = (uint32_t)(((uint64_t)(count - rate_count) * 1000U) / (uint32_t)(now - rate_ms));
        rate_count = count; rate_ms = now;
    }
    if((uint32_t)(now - last_ui) < 200U) return;
    last_ui = now;
#if !UI_DEMO_IPC_COUNTER && UI_PD_MONITOR
    ui_pd_monitor_render();
    return;
#endif
    const char *status = !PD_Shared_LayoutValid() ? "LAYOUT ERROR" :
        (s->tx_fault || s->rx_fault) ? "QUEUE FAULT" :
        !s->receiver_ready ? "RECEIVER NOT READY" :
        (s->write_seq != s->read_seq && (uint32_t)(now - last_progress) > 2000U) ? "RECEIVER STALLED" : "SHM OK";
    lv_label_set_text_fmt(ipc_status_label, "PD MONITOR\n%s", status);
    if(have_frame) {
        const char *crc = last_record.crc_status == PD_CHECK_PASS ? "PASS" : last_record.crc_status == PD_CHECK_FAIL ? "FAIL" : "UNKNOWN";
        const char *len = last_record.length_status == PD_CHECK_PASS ? "PASS" : last_record.length_status == PD_CHECK_FAIL ? "FAIL" : "UNKNOWN";
        lv_label_set_text_fmt(ipc_frame_label, "SEQ %lu  HDR 0x%04lX  DATA %uB", (unsigned long)last_record.sequence, (unsigned long)last_record.header, (unsigned)last_record.payload_len);
        lv_label_set_text_fmt(ipc_check_label, "CC%u  SOP%u  CRC %s  LEN %s", (unsigned)last_record.cc, (unsigned)last_record.sop, crc, len);
    }
    uint32_t used = s->write_seq - s->read_seq;
    lv_label_set_text_fmt(ipc_stats_label, "TX %lu RX %lu Q %lu HI %lu\nFULL %lu INV %lu CRC %lu\nRX/s %lu MAXP %lu", (unsigned long)s->tx_ok, (unsigned long)s->rx_ok, (unsigned long)used, (unsigned long)s->high_water, (unsigned long)s->tx_full, (unsigned long)(s->tx_invalid + invalid_count), (unsigned long)s->rx_integrity, (unsigned long)rx_rate, (unsigned long)max_poll_gap);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
	SystemAndCoreClockUpdate();
	Delay_Init();
	USART_Printf_Init(115200);
	printf("V5F SystemCoreClk:%d\r\n", SystemCoreClock);

	Delay_Ms(500);

#if (Run_Core == Run_Core_V3FandV5F)
	HSEM_FastTake(HSEM_ID0);
	HSEM_ReleaseOneSem(HSEM_ID0, 0);

#elif (Run_Core == Run_Core_V3F)

#elif (Run_Core == Run_Core_V5F)

#endif

LCD_Init();
LCD_LED_SET;
Delay_Ms(3000);
lv_init();
lv_port_disp_init();
LVGL_TIM2_Init();

#if UI_DEMO_IPC_COUNTER
    ui_ipc_counter_create();
    (void)PD_Transport_Attach();
    pd_attached_ms=lv_tick_get();
#elif UI_PD_MONITOR
    ui_pd_monitor_create();
    (void)PD_Transport_Attach();
    pd_attached_ms=lv_tick_get();
#elif UI_DEMO_PD_REPLAY
    ui_pd_replay_create();
#elif UI_DEMO_BASIC_DRAW
    ui_basic_draw_create();
#else
    ui_test_create();
#endif

	while(1)
	{

        #if UI_DEMO_IPC_COUNTER || UI_PD_MONITOR
            PD_Transport_Poll();
        #endif
            lv_port_disp_poll();
            lv_timer_handler();
            Delay_Ms(5);
	}
}
