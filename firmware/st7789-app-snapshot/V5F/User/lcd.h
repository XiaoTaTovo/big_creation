/*********************************************************************
 * File Name          : lcd.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for lcd screen.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#ifndef __LCD_H
#define __LCD_H
#include "spi.h"
#include "stdio.h"
#include "stdlib.h"

#include "config.h"

// [15:11] 5bit red
//  [10:5] 6bit green
//   [4:0] 5bit blue
#define LCD_WHITE    0xFFFF
#define LCD_BLACK    0x0000
#define LCD_BLUE     0x001F
#define LCD_BRED     0XF81F
#define LCD_GRED     0XFFE0
#define LCD_GBLUE    0X07FF
#define LCD_RED      0xF800
#define LCD_MAGENTA  0xF81F
#define LCD_GREEN    0x07E0
#define LCD_CYAN     0x7FFF
#define LCD_YELLOW   0xFFE0
#define LCD_BROWN    0XBC40
#define LCD_BRRED    0XFC07
#define LCD_GRAY     0X8430


#define LCD_LED_SET  (LCD_LED_PORT->BSHR = LCD_LED_PIN)
#define LCD_CS_SET   (LCD_CS_PORT->BSHR = LCD_CS_PIN)
#define LCD_DC_SET   (LCD_DC_PORT->BSHR = LCD_DC_PIN)
#define LCD_RST_SET  (LCD_RST_PORT->BSHR = LCD_RST_PIN)


#define LCD_LED_CLR  (LCD_LED_PORT->BCR = LCD_LED_PIN)
#define LCD_CS_CLR   (LCD_CS_PORT->BCR = LCD_CS_PIN)
#define LCD_DC_CLR   (LCD_DC_PORT->BCR = LCD_DC_PIN)
#define LCD_RST_CLR  (LCD_RST_PORT->BCR = LCD_RST_PIN)

void LCD_Init(void);
void LCD_Clear(uint16_t Color);
void LCD_GPIOInit(void);
void Lcd_WriteData_16Bit(uint16_t Data);
void LCD_WritePixels_Continuous(const uint16_t *pixels, uint32_t count);
void LCD_direction(uint8_t direction);
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd);
void LCD_Test(void);
void LCD_DrawPoint(u16 x,u16 y,u16 color);


void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);//指定区域填充颜色


#endif
