/********************************** (C) COPYRIGHT *******************************
 * File Name          : main_v3f.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2025/03/01
 * Description        : Main program body for V3F.
 *********************************************************************************
 * Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 polling transceiver mode, master/slave transceiver routine:
 Master:USART2_Tx(PD5)\USART2_Rx(PD6).
 Slave:USART3_Tx(PB10)\USART3_Rx(PB11).
 This example demonstrates sending from USART2 and receiving from USART3.

   Hardware connection:
			   PD5 -- PB11
               PD6 -- PB10

*/

#include "debug.h"
#include "pd_capture.h"
#include "pd_shared.h"
#include "pd_transport.h"
/* 1 validates transport with synthetic records; set 0 for teammate capture. */
#include "pd_source_config.h"
/* Test-only: 1 injects invalid payload_len on every tenth capture. */
#define PD_DEMO_INVALID_EVERY_10 0
#include "hardware.h"

static void PD_Transport_IPCInit(void)
{
    IPC_InitTypeDef cfg = {0};

    /* 本实验由 V3 在启动 V5 之前初始化 IPC。 */
    IPC_DeInit();

    cfg.IPC_CH = IPC_CH0;
    cfg.TxCID = IPC_TxCID0;   /* 发送方：V3 */
    cfg.RxCID = IPC_RxCID1;   /* 接收方：V5 */

    /* 第一版使用轮询，不使用 IPC 中断。 */
    cfg.TxIER = DISABLE;
    cfg.RxIER = DISABLE;
    cfg.AutoEN = DISABLE;//它控制的是：写入或置位 IPC 状态时，相关的中断使能位是否自动更新。

    IPC_Init(&cfg);

    __asm volatile("fence iorw, iorw" ::: "memory");
    
    //现在先把它读成：对前后的内存和外设访问，建立明确的执行顺序约束。放在初始化末尾，是让初始化阶段的访问，与后续唤醒 V5 等操作保持正确顺序。
}

PdSubmitResult PD_Capture_Submit(const PdCaptureRecord *record)
{
    PdSubmitResult result = PD_Transport_Push(record);
    if(result == PD_SUBMIT_OK)
        IPC_SetFlagStatus(IPC_CH0, IPC_CH_Sta_Bit0);
    return result;
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
	SystemInit();
	SystemAndCoreClockUpdate();
	Delay_Init();
	USART_Printf_Init(115200);
	Delay_Ms(1000);

	printf("SystemClk:%d\r\n", SystemClock);
	printf("V3F SystemCoreClk:%d\r\n", SystemCoreClock);
	Delay_Ms(500);

#if (Run_Core == Run_Core_V3FandV5F)

    PD_Shared_BootInit();
    PD_Transport_IPCInit();
	NVIC_WakeUp_V5F(Core_V5F_StartAddr);//wake up V5
	HSEM_ITConfig(HSEM_ID0, ENABLE);
	NVIC->SCTLR |= 1<<4;
	RCC_HB1PeriphClockCmd(RCC_HB1Periph_PWR, ENABLE);
	PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFE);
	HSEM_ClearFlag(HSEM_ID0);
	printf("V3F wake up\r\n");

	//Hardware();

#elif (Run_Core == Run_Core_V3F)
	Hardware();

#elif (Run_Core == Run_Core_V5F)
	NVIC_WakeUp_V5F(Core_V5F_StartAddr);//wake up V5
	PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFE);
	printf("V3F wake up\r\n");
#endif

#if PD_CAPTURE_DEMO_MODE
    /* Validation source only. Replays the eight 1022 frames at t8 milliseconds.
     * Production capture code calls PD_Capture_Submit() instead. */
    uint32_t sequence = 0;
    /* V3 has no LVGL tick. The 1 ms Delay_Ms() below is the demo clock. */
    uint32_t replay_elapsed_ms = 0;
    unsigned replay_index = 0;
    uint32_t replay_base_ms = 0;
#endif
    while(1)
    {
#if PD_CAPTURE_DEMO_MODE
        if(PD_REPLAY_LOOP && replay_index == 8U && replay_elapsed_ms >= 3167U) {
            replay_index=0; replay_elapsed_ms=0; replay_base_ms+=3167U;
        }
        if(PD_Shared_LayoutValid() && PD_Shared_Area()->receiver_ready == 1 &&
           replay_index < 8U && replay_elapsed_ms >= PD_Transport_1022T8Ms(replay_index))
        {
            PdCaptureRecord record;
            PD_Transport_1022Record(&record, ++sequence);
            record.timestamp_us += replay_base_ms * 1000U;
#if PD_DEMO_INVALID_EVERY_10
            if(sequence % 10U == 0) record.payload_len=29;
#endif
            (void)PD_Capture_Submit(&record);
            ++replay_index;

        }
        Delay_Ms(1);
        if((replay_index < 8U || PD_REPLAY_LOOP) && PD_Shared_LayoutValid() &&
           PD_Shared_Area()->receiver_ready == 1) ++replay_elapsed_ms;
#else
        /* Teammate capture path: fill PdCaptureRecord and call
         * PD_Capture_Submit(&record) from the V3 main-context adapter. */
        Delay_Ms(1);
#endif
    }
}
