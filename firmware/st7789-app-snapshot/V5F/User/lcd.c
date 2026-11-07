/*********************************************************************
 * File Name          : lcd.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for lcd screen.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#include "lcd.h"
#include "debug.h"



static uint16_t width;
static uint16_t height;

#define LCD_SOFT_RESET          (0x01)
#define LCD_READ_ID             (0x04)
#define LCD_READ_DISPLAY_STATUS (0x09)
#define LCD_ON                  (0x28)
#define LCD_OFF                 (0x29)

#define LCD_SET_X               (0x2a)
#define LCD_SET_Y               (0x2B)
#define LCD_MEM_WRITE           (0x2C)
#define LCD_MEM_READ            (0x2e)



/*********************************************************************
 * @fn      LCD_WR_REG
 *
 * @brief   Write an 8-bit command to the LCD screen
 * 
 * @param   data - Command value to be written
 *
 * @return  none
 */
static void LCD_WR_REG(uint8_t data)
{
    LCD_CS_CLR;
    LCD_DC_CLR;
    SPI1_Write(data);
    LCD_CS_SET;
}

/*********************************************************************
 * @fn      LCD_WR_DATA
 *
 * @brief   Write an 8-bit data to the LCD screen
 * 
 * @param   data - the data to write
 * 
 */
static void LCD_WR_DATA(uint8_t data)
{
    LCD_CS_CLR;
    LCD_DC_SET;
    SPI1_Write(data);
    LCD_CS_SET;
}

/*********************************************************************
 * @fn      LCD_WriteReg
 * 
 * @brief   write a data to the register
 * 
 * @param   LCD_Reg - register address
 *          LCD_RegValue - the data to write
 * 
 * @return  none
 */
 void LCD_WriteReg(uint8_t LCD_Reg, uint8_t LCD_RegValue)
{
    LCD_WR_REG(LCD_Reg);
    LCD_WR_DATA(LCD_RegValue);
}

/*********************************************************************
 * @fn      Lcd_WriteData_16Bit
 * 
 * @brief   write two bytes to the lcd screen
 * 
 * @param   Data - the data to write
 * 
 * @return  none
 */
void Lcd_WriteData_16Bit(uint16_t Data)
{
    LCD_CS_CLR;
    LCD_DC_SET;
    SPI1_Write(Data >> 8);
    SPI1_Write(Data);
    LCD_CS_SET;
}



/*********************************************************************
 * @fn      LCD_GPIOInit
 * 
 * @brief   Configuring the control gpio for the lcd screen
 * 
 * @return  none
 */
void LCD_WritePixels_Continuous(const uint16_t *pixels, uint32_t count)
{
    uint32_t i;

    /* Keep CS low and DC high for the complete pixel block. */
    LCD_CS_CLR;
    LCD_DC_SET;

    for(i = 0; i < count; i++)
    {
        /* RGB565: ST7789 receives the high byte first. */
        SPI1_Write((uint8_t)(pixels[i] >> 8));
        SPI1_Write((uint8_t)(pixels[i] & 0xFF));
    }

    LCD_CS_SET;
}

void LCD_GPIOInit(void)
{
    RCC_HB2PeriphClockCmd(LCD_LED_RCC|LCD_DC_RCC|LCD_RST_RCC|LCD_CS_RCC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;


    GPIO_InitStructure.GPIO_Pin   = LCD_LED_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_LED_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_DC_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_DC_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_RST_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_RST_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_CS_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LCD_CS_PORT, &GPIO_InitStructure);
}


/*********************************************************************
 * @fn      LCD_RESET
 * 
 * @brief   reset the lcd screen by the rst pin
 * 
 * @return  none
 */
void LCD_RESET(void)
{
    LCD_RST_CLR;
    Delay_Ms(100);
    LCD_RST_SET;
    Delay_Ms(50);
}

/*********************************************************************
 * @fn      LCD_Init
 * 
 * @brief   Initialization LCD screen
 * 
 * @return  none
 */
void LCD_Init(void)
{
    SPI1_Init();

    LCD_GPIOInit();
    LCD_LED_CLR;//关闭背光
    LCD_RESET();


    LCD_WR_REG(0x36);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA(0x05);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x33);

    LCD_WR_REG(0xB7);
    LCD_WR_DATA(0x56);

    LCD_WR_REG(0xBB);
    LCD_WR_DATA(0x1D);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA(0x2C);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA(0x01);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA(0x0F);

    LCD_WR_REG(0xC6);
    LCD_WR_DATA(0x0F);

    LCD_WR_REG(0xD0);
    LCD_WR_DATA(0xA7);

    LCD_WR_REG(0xD0);
    LCD_WR_DATA(0xA4);
    LCD_WR_DATA(0xA1);

    LCD_WR_REG(0xD6);
    LCD_WR_DATA(0xA1);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA(0xF0);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x06);
    LCD_WR_DATA(0x14);
    LCD_WR_DATA(0x2F);
    LCD_WR_DATA(0x54);
    LCD_WR_DATA(0x46);
    LCD_WR_DATA(0x38);
    LCD_WR_DATA(0x13);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x2E);
    LCD_WR_DATA(0x35);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA(0xF0);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x2F);
    LCD_WR_DATA(0x43);
    LCD_WR_DATA(0x46);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x12);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x32);

    LCD_WR_REG(0x21);



    /* 设置列地址范围：170列 */
LCD_WR_REG(0x2A);
LCD_WR_DATA(0x00);
LCD_WR_DATA(0x23);  // X起始地址：35
LCD_WR_DATA(0x00);
LCD_WR_DATA(0xCC);  // X结束地址：204

/* 设置行地址范围：320行 */
LCD_WR_REG(0x2B);
LCD_WR_DATA(0x00);
LCD_WR_DATA(0x00);  // Y起始地址：0
LCD_WR_DATA(0x01);
LCD_WR_DATA(0x3F);  // Y结束地址：319

    LCD_WR_REG(0x11);  //Exit Sleep
    Delay_Ms(120);
    LCD_WR_REG(0x29);  //display on


    LCD_direction(2);
LCD_Clear(LCD_GREEN);
}




/*********************************************************************
 * @fn      LCD_SetWindows
 * 
 * @brief   Setting LCD display window
 * 
 * @param   xStar -  the bebinning x coordinate of the LCD display window
 *          yStar -  the bebinning y coordinate of the LCD display window
 *          xEnd -  the endning x coordinate of the LCD display window
 *          yEnd -  the endning y coordinate of the LCD display window
 * 
 * @return  none
 */
void LCD_SetWindows(uint16_t xStar, uint16_t yStar,
                    uint16_t xEnd,  uint16_t yEnd)
{
    /* 170x320屏幕的列地址偏移 */
    xStar += 35;
    xEnd  += 35;

    /* 设置列地址 */
    LCD_WR_REG(LCD_SET_X);
    LCD_WR_DATA(xStar >> 8);
    LCD_WR_DATA(xStar & 0xFF);
    LCD_WR_DATA(xEnd >> 8);
    LCD_WR_DATA(xEnd & 0xFF);

    /* 设置行地址 */
    LCD_WR_REG(LCD_SET_Y);
    LCD_WR_DATA(yStar >> 8);
    LCD_WR_DATA(yStar & 0xFF);
    LCD_WR_DATA(yEnd >> 8);
    LCD_WR_DATA(yEnd & 0xFF);

    /* 开始写入显存 */
    LCD_WR_REG(LCD_MEM_WRITE);
}

/*********************************************************************
 * @fn      LCD_direction
 * 
 * @brief   Setting the display direction of LCD screen
 * 
 * @param   旋转角度:0-0 degree  1-90 degree  2-180 degree  3-270 degree
 * 
 * @return  none
 */
void LCD_direction(uint8_t direction)
{
    switch (direction)
    {
        case 0:
            width  = LCD_W;
            height = LCD_H;
            LCD_WriteReg(0x36, (0 << 3) | (0 << 6) | (0 << 7));  //BGR==1,MY==0,MX==0,MV==0
            break;
        case 1:
            width  = LCD_H;
            height = LCD_W;
            LCD_WriteReg(0x36, (0 << 3) | (0 << 7) | (1 << 6) | (1 << 5));  //BGR==1,MY==1,MX==0,MV==1
            break;
        case 2:
            width  = LCD_W;
            height = LCD_H;
            LCD_WriteReg(0x36, (0 << 3) | (1 << 6) | (1 << 7));  //BGR==1,MY==0,MX==0,MV==0
            break;
        case 3:
            width  = LCD_H;
            height = LCD_W;
            LCD_WriteReg(0x36, (0 << 3) | (1 << 7) | (1 << 5));  //BGR==1,MY==1,MX==0,MV==1
            break;
        default:
            break;
    }
}

/*********************************************************************
 * @fn      LCD_Clear
 * 
 * @brief   Full screen filled LCD screen
 * 
 * @param   Color -   Filled color
 * 
 * @return  none
 */
void LCD_Clear(uint16_t Color)
{
    unsigned int i, m;
    LCD_SetWindows(0, 0, width - 1, height - 1);
    LCD_CS_CLR;
    LCD_DC_SET;
    for (i = 0; i < height; i++)
    {
        for (m = 0; m < width; m++)
        {
            Lcd_WriteData_16Bit(Color);
        }
    }
    LCD_CS_SET;
}

/*********************************************************************
 * @fn      LCD_drawImageWithSize
 * 
 * @brief   read image from SPI Flash and display on the LCD screen
 * 
 * @param   x -  the x coordinate of the LCD screen
 *          y - the y coordinate of the LCD screen
 *          width -  the image width
 *          height - the image height
 *          addr - the image address in SPI flash
 * 
 * @return  none
 */

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
                                color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{
    u16 i,j;
    LCD_SetWindows(xsta,ysta,xend-1,yend-1);//设置显示范围
    for(i=ysta;i<yend;i++)
    {
        for(j=xsta;j<xend;j++)
        {
            Lcd_WriteData_16Bit(color);
        }
    }
}


/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
    LCD_SetWindows(x,y,x,y);//设置光标位置
    Lcd_WriteData_16Bit(color);
}


void LCD_Test(void)
{
    LCD_direction(2);

    /* 1. 全屏黑色 */
    LCD_Clear(LCD_BLACK);

    /* 2. 左上角：红色方块 */
    LCD_SetWindows(0, 0, 19, 19);
    LCD_CS_CLR;
    LCD_DC_SET;
    for (uint32_t i = 0; i < 20 * 20; i++)
        Lcd_WriteData_16Bit(LCD_RED);
    LCD_CS_SET;

    /* 3. 右上角：绿色方块 */
    LCD_SetWindows(150, 0, 169, 19);
    LCD_CS_CLR;
    LCD_DC_SET;
    for (uint32_t i = 0; i < 20 * 20; i++)
        Lcd_WriteData_16Bit(LCD_GREEN);
    LCD_CS_SET;

    /* 4. 左下角：蓝色方块 */
    LCD_SetWindows(0, 300, 19, 319);
    LCD_CS_CLR;
    LCD_DC_SET;
    for (uint32_t i = 0; i < 20 * 20; i++)
        Lcd_WriteData_16Bit(LCD_BLUE);
    LCD_CS_SET;

    /* 5. 右下角：白色方块 */
    LCD_SetWindows(150, 300, 169, 319);
    LCD_CS_CLR;
    LCD_DC_SET;
    for (uint32_t i = 0; i < 20 * 20; i++)
        Lcd_WriteData_16Bit(LCD_WHITE);
    LCD_CS_SET;
}
