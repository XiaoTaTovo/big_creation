/*********************************************************************
 * File Name          : spi.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for spi.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __SPI_H__
#define __SPI_H__

#include "debug.h"
#include "config.h"

#define SPI1_DMA_RX_CH   DMA1_Channel2
#define SPI1_DMA_TX_CH   DMA1_Channel3

#define SPI1_DMA_TC_FLAG DMA1_FLAG_TC3
#define SPI1_DMA_RC_FLAG DMA1_FLAG_TC2



void SPI1_Init(void);

void SPI1_DMA_Init(void);
void SPI1_Write(uint8_t data);
void SPI1_Write_DMA(uint8_t *data, uint16_t len);
#define SPI1_DMA_IDLE  0U
#define SPI1_DMA_BUSY  1U
#define SPI1_DMA_DONE  2U
#define SPI1_DMA_ERROR 3U
void SPI1_DMA_Abort(void);
uint8_t SPI1_DMA_Start(const uint8_t *data, uint16_t byte_count);
uint8_t SPI1_DMA_GetState(void);
void SPI1_DMA_IRQHandler(void);

#endif
