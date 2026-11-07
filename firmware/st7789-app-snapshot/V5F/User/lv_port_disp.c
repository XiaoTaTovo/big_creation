/**
 * @file lv_port_disp_template.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "lcd.h"

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES LCD_W  /* 170 列 */
#define MY_DISP_VER_RES LCD_H  /* 320 行 */


#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
#define LCD_DMA_MAX_PIXELS (MY_DISP_HOR_RES * 10U)

LV_ATTRIBUTE_MEM_ALIGN
static uint8_t lcd_dma_bytes[LCD_DMA_MAX_PIXELS * 2U];
#define LCD_DMA_TIMEOUT_MS 100U
static lv_display_t *dma_pending_disp;
static uint32_t dma_started_ms;
static uint8_t dma_flush_active;
static uint8_t dma_fault;
static void disp_wait(lv_display_t *disp);

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);//disp_flush 后面没有括号，这里传递的是函数地址。
    //这种“先交出函数地址，之后由对方调用”的方式，就叫回调。 cb 是 callback，即“回调”的缩写。
    //LVGL，这批像素画好以后，请调用 disp_flush()，由它负责发送到屏幕。
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_wait_cb(disp, disp_wait);

    /* Example 1
     * One buffer for partial rendering*/
    /* 缓冲区 A：可容纳满宽十行 */
    LV_ATTRIBUTE_MEM_ALIGN static uint8_t draw_buf_a[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    /* 缓冲区 B：与 A 大小相同 */
    LV_ATTRIBUTE_MEM_ALIGN static uint8_t draw_buf_b[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    /* 将两块缓冲区交给 LVGL 管理 */
    lv_display_set_buffers(disp,draw_buf_a,draw_buf_b,sizeof(draw_buf_a),LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    //告诉lvgl 对于disp对象请在这个缓冲区画画 采用分块绘制模式

#if 0
    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);
    #endif

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    SPI1_DMA_Init();
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    if(!dma_fault) disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/

/* Called in main context only. IRQ only records DMA status. */
static void disp_finish(const char *error)
{
    lv_display_t *disp = dma_pending_disp;
    if(error) {
        SPI1_DMA_Abort();
        dma_fault = 1;
        disp_flush_enabled = false;
    }
    LCD_CS_SET;
    dma_flush_active = 0;
    dma_pending_disp = NULL;
    lv_display_flush_ready(disp);
    if(error) printf("LCD DMA: %s\r\n", error);
}

static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area,
                       uint8_t *px_map)
{
    uint32_t count;
    const uint16_t *pixels = (const uint16_t *)px_map;
    if(!disp_flush_enabled) {
        lv_display_flush_ready(disp_drv);
        return;
    }
    /* LVGL must finish the previous flush before invoking us again. */
    if(dma_flush_active) {
        disp_finish("unexpected overlapping flush");
        return;
    }
    if(!px_map || area->x1 < 0 || area->y1 < 0 ||
       area->x2 < area->x1 || area->y2 < area->y1 ||
       area->x2 >= MY_DISP_HOR_RES || area->y2 >= MY_DISP_VER_RES) {
        disp_flush_enabled = false;
        printf("LCD DMA: invalid area\r\n");
        lv_display_flush_ready(disp_drv);
        return;
    }
    count = (uint32_t)(area->x2 - area->x1 + 1) *
            (uint32_t)(area->y2 - area->y1 + 1);
    if(count > LCD_DMA_MAX_PIXELS) {
        disp_flush_enabled = false;
        printf("LCD DMA: area exceeds staging buffer\r\n");
        lv_display_flush_ready(disp_drv);
        return;
    }
    for(uint32_t i = 0; i < count; i++) {
        lcd_dma_bytes[2U * i] = (uint8_t)(pixels[i] >> 8);
        lcd_dma_bytes[2U * i + 1U] = (uint8_t)pixels[i];
    }
    LCD_SetWindows((u16)area->x1, (u16)area->y1,
                   (u16)area->x2, (u16)area->y2);
    LCD_CS_CLR;
    LCD_DC_SET;
    dma_pending_disp = disp_drv;
    dma_started_ms = lv_tick_get();
    dma_flush_active = 1;
    if(!SPI1_DMA_Start(lcd_dma_bytes, (uint16_t)(count * 2U)))
        disp_finish("start failed");
    /* Success: return without flush_ready; DMA still owns the staging buffer. */
}

void lv_port_disp_poll(void)
{
    uint8_t state;
    if(!dma_flush_active) return;
    state = SPI1_DMA_GetState();
    if(state == SPI1_DMA_ERROR) {
        disp_finish("transfer error (TE3)");
        return;
    }
    if(state == SPI1_DMA_DONE &&
       SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET &&
       SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == RESET) {
        disp_finish(NULL);
        return;
    }
    if(lv_tick_elaps(dma_started_ms) >= LCD_DMA_TIMEOUT_MS) {
        disp_finish(state == SPI1_DMA_DONE ? "SPI tail timeout" : "DMA timeout");
    }
}

/* LVGL can wait INSIDE lv_timer_handler; main-loop polling alone deadlocks.
 * Only this buffer-reuse point waits. DMA runs while LVGL renders the other buffer.
 * The 100 ms timeout depends on the existing TIM2 1 ms tick interrupt.
 */
static void disp_wait(lv_display_t *disp)
{
    (void)disp;
    while(dma_flush_active) lv_port_disp_poll();
}

#else
 typedef int keep_pedantic_happy;
#endif
