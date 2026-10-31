# 当前显示工程的双核协作阅读路线

2026-10-30。只做源码与官方资料核对，未修改固件、未编译 V3、未执行硬件实验。

## 当前代码事实

实际工程：D:/01_Workspace/12_Dev_Projects/AllCode/QinHen/ST77789_LVGL/ST7789_TEST。
Common/Debug/debug.h 默认 Run_Core_V3FandV5F；V3 编译定义 Core_V3F，V5 定义 Core_V5F。
V3 main.c 调用 SystemInit，当前 HSE 时钟方案目标 V3=100 MHz、V5=400 MHz（不是本轮实测）。
V3 唤醒 V5（入口 0x00010000），使能 HSEM0 通知后 WFE 等待，醒来清标志，随后每秒输出 V3F running。
V3 双核分支 Hardware() 被注释；main 顶部 USART2/3 注释是遗留说明，不能当作活动功能。
V5 获取并释放 HSEM0 后才初始化 LCD/LVGL，因此该信号不是 LCD_READY。V5 内部生成回放事件，尚无 V3→V5 数据通路。
V3 printf 默认 USART1/PA9 TX，V5 默认 USART8/PB4 TX；均 115200/8N1、仅发送，_write 是逐字节阻塞等待。
物理调试器接到哪个 TX 引脚、当前板上是否为磁盘这版固件，尚未重新确认。
V3 obj/User/subdir.mk 仍有旧 d:/STM32_projects 路径；后续实验前需确认活动工程与重新构建，不能拿旧构建产物证明可复现。

## 本地应用手册与导航

手册：docs/references/CH32H417RM.PDF，来自官方，V1.8；来源/哈希见同名 source.json。
桌面 CH32H417DS0.PDF 为数据手册，不替代应用手册的寄存器说明。
以下是 PDF 阅读器页序号（从 1 开始），不是纸面页码；此版本前部页码不连续，不统一按固定偏移换算。

| PDF 页 | 章节 | 本次阅读目标 |
|---|---|---|
| 3 | 图1-1-1 | 两个 CPU 与片上资源的关系 |
| 6–7 | 1.2 | ITCM/DTCM 与 512KB 共享代码数据区 |
| 43–44 | 4.1、4.1.1 | C0=V3，C1=V5；内存重排序与 fence |
| 67 | 4.4、4.4.1 | IPC 的通道、状态、接收/发送中断 |
| 71 | 4.4.2.8、4.5 | 四个32位信息寄存器；32个 HSEM 通道 |
| 72 | 4.5.1.1、4.5.1.2 | 单步获取、解锁及通知 |
| 79 | 4.7 | 中断归属；一次中断请求分配给一个核 |
| 108 | 4.7.5.52 | V5 唤醒指令地址 |
| 111–112 | 4.7.5.58 | SCTLR，SEVONPEND 与 SENDEVENT |

核对提醒：手册 PDF72 对 HSEM_RLRX 单步读返回值的叙述，与本地库 ch32h417_hsem.c:60 的比较条件不一致。应用代码应判断 HSEM_FastTake 返回 READY/NoREADY；不直接把返回值与手册中的寄存器读值混用。后续用官方样例验证，不宣称已经验证差异原因。

## 官方样例

根目录：C:/Users/taowz/Desktop/大创/CH32H417EVT/EVT/EXAM/CPU。

- HSEM/HSEM_CoreNotification：释放信号量通知另一核。
- HSEM/HSEM_CoreSync：两核阶段同步。
- HSEM/HSEM_DataSharing：shared.h、两核 User/shared.c、Common/Ld 中 .shared (NOLOAD) 与 RAM_SHARED。
- IPC/IPC：Common/hardware.c 的 IPC_Config、IPC_WriteMSG/ReadMSG 和 IPC_CH0_Handler。

DataSharing 样例将 0x20178000 开始的32KB保留为共享区，同时从 V3 普通 RAM 配额减去32KB。
当前显示工程没有这项预留；其 V3 RAM/栈分配可覆盖这块地址，不能只复制 shared 指针。
IPC 样例 Hardware() 含永久循环，ISR 中有 printf；用来理解机制，不直接整段搬进 LVGL 主循环。
共享结构体名称相同不代表地址相同；volatile 不提供完整互斥或跨核发布顺序。

## 后续实验顺序（尚未实施）

1. 确认当前两核启动和串口日志，修复 V3 活动构建路径。
2. 在当前显示工程增加一个递增整数的核间通信，先跑通发布、通知、接收、确认，限制为单条在途消息。
3. 保留独立共享区后改为结构化事件；先使用既有八帧模拟源，由 V3 产生，V5 调用原状态更新函数。
4. 再接 V3 UART 到 PC：明确物理桥接、记录格式、序号/长度/校验/队列，避免把 printf 当成最终协议。
5. 最后替换 V3 的模拟源为队友真实采集模块；测试接收慢、队列满与丢帧计数。

V3 采集/生成数据、V5 显示是项目既定方向；V3 直接导出还是经 V5 导出、UART 还是 USB CDC 尚未冻结。
