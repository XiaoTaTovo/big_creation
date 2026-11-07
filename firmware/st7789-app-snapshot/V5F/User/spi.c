/*********************************************************************
 * File Name          : spi.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/8/15
 * Description        : file for spi.
 *********************************************************************************
* Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "spi.h"

/*********************************************************************
 * @fn      SPI1_Init
 * 
 * @brief   Initialize SPI1 
 * 
 * @return  none
 */
void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef  SPI_InitStructure  = {0};

    
     RCC_HB2PeriphClockCmd( RCC_HB2Periph_GPIOA,
        ENABLE
    );

    /* Enable SPI1 clock */
    RCC_HB2PeriphClockCmd(RCC_HB2Periph_SPI1, ENABLE);

    /* PA5: SPI1_SCK (AF5) */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF5);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Very_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA7: SPI1_MOSI (AF5) */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF5);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Very_High;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* SPI1 configuration */
    // SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;

    /* SPI Mode 0 */
SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;

    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler =
        SPI_BaudRatePrescaler_Mode2;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}



/*********************************************************************
 * @fn      SPI1_Write
 * 
 * @brief   send one byte
 * 
 * @param   data -  the data to send
 * 
 * @return  none
 */
void SPI1_Write(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
        ;
    SPI_I2S_SendData(SPI1, data);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET)
        ;
}

static volatile uint8_t spi1_dma_state;

void SPI1_DMA_Init(void)
{
    RCC_HBPeriphClockCmd(RCC_HBPeriph_DMA1, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_DeInit(DMA1_Channel3);
    DMA_MuxChannelConfig(DMA_MuxChannel3, 63); /* SPI1_TX request */
    DMA_ClearFlag(DMA1, DMA1_FLAG_GL3);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC | DMA_IT_TE, ENABLE);
    NVIC_SetPriority(DMA1_Channel3_IRQn, 2);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    spi1_dma_state = 0;
}

uint8_t SPI1_DMA_Start(const uint8_t *data, uint16_t byte_count)
{
    DMA_InitTypeDef dma = {0};

    if(data == NULL || byte_count == 0 || (spi1_dma_state != SPI1_DMA_IDLE && spi1_dma_state != SPI1_DMA_DONE))
        return 0;

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_Cmd(DMA1_Channel3, DISABLE);
    DMA_ClearFlag(DMA1, DMA1_FLAG_GL3);
    DMA_ClearITPendingBit(DMA1, DMA1_IT_TC3 | DMA1_IT_TE3);

    dma.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DATAR;
    dma.DMA_Memory0BaseAddr = (uint32_t)data;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = byte_count;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel3, &dma);

    spi1_dma_state = SPI1_DMA_BUSY;
    __asm volatile("fence iorw, iorw" ::: "memory");
    DMA_Cmd(DMA1_Channel3, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    return 1;
}

uint8_t SPI1_DMA_GetState(void)
{
    return spi1_dma_state;
}

void SPI1_DMA_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1, DMA1_IT_TE3) != RESET)
    {
        DMA_ClearFlag(DMA1, DMA1_FLAG_GL3);
        SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
        DMA_Cmd(DMA1_Channel3, DISABLE);
        spi1_dma_state = SPI1_DMA_ERROR;
        return;
    }

    if(DMA_GetITStatus(DMA1, DMA1_IT_TC3) != RESET)
    {
        DMA_ClearITPendingBit(DMA1, DMA1_IT_TC3);
        SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
        DMA_Cmd(DMA1_Channel3, DISABLE);
        spi1_dma_state = SPI1_DMA_DONE;
    }
}

/* Terminal fault: stop all writes before releasing CS. Reinitialize to recover. */
void SPI1_DMA_Abort(void)
{
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_Cmd(DMA1_Channel3, DISABLE);
    DMA_ClearFlag(DMA1, DMA1_FLAG_GL3);
    SPI_Cmd(SPI1, DISABLE);
    spi1_dma_state = SPI1_DMA_ERROR;
}
