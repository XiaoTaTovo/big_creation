# 陶震轩的学习与开发路线

状态：Draft 0.1

你的申请书分工是“嵌入式系统界面实现”。因此学习顺序不应是先把 USB PD、模拟前端、RTOS、GUI 全部学完，而应围绕一个可演示闭环推进：**PC 模拟界面 → 固定报文数据模型 → 屏幕驱动 → LVGL 页面 → 与采集/解析模块联调 → 性能与稳定性测试**。

## 1. 第一阶段：开发基本功

需要掌握：

- C 语言的指针、数组、结构体、位运算、枚举、`const`、`volatile`、静态存储期；
- 头文件边界、模块化、错误码、内存所有权和固定容量容器；
- Git 的提交、分支、合并、回退、冲突解决和 Pull Request；
- 基本调试：断点、Watch、调用栈、内存查看、串口日志和最小复现。

阶段产出：

1. 在 PC 上编写一个小型 C 程序，读取固定的 `PdFrame` 数组并打印报文摘要；
2. 创建功能分支，完成一次规范提交和合并；
3. 能解释栈、堆、全局区，以及为什么中断里不能做耗时操作。

推荐材料：

- *Effective C, 2nd Edition*：重点读对象、类型、指针、动态内存、错误处理和未定义行为；
- *Pro Git*：先读第 1～3 章，再按需要读远程协作；
- 仓库中的 `CONTRIBUTING.md`。

## 2. 第二阶段：CH32H417 与嵌入式系统基础

需要掌握：

- CH32H417 的 V5F/V3F 双核、存储器映射、时钟、GPIO、中断和 DMA；
- LTDC/显示接口、外部存储器、缓存一致性和核间通信的基本概念；
- MounRiver Studio/官方工具链的工程结构、下载、调试与链接脚本；
- 定时器、环形缓冲区、生产者—消费者和有界队列。

学习顺序：

1. 先完成 GPIO、串口、定时器和 DMA 小实验；
2. 再跑官方 LCD/LTDC、外部 RAM 和双核 IPC 示例；
3. 最后把显示驱动、GUI 和业务数据模型接起来。

必须以官方资料为准：

- CH32H417 数据手册 `CH32H417DS0.PDF`；
- CH32H417 应用手册 `CH32H417RM.PDF`；
- QingKe V5/V3 处理器手册；
- `CH32H417EVT` 对应版本的开发板说明和示例工程。

阶段产出：

- 串口日志和系统节拍正常；
- DMA 搬运实验可复现；
- 屏幕能稳定显示纯色、色块和字体；
- 记录 Flash、RAM、帧缓冲和刷新带宽预算。

## 3. 第三阶段：USB Type-C / USB PD 必要知识

你不需要一上来实现完整协议栈，但必须理解界面上展示的数据从哪里来、哪些字段可信。

优先掌握：

- Type-C 的 CC1/CC2、Source/Sink、DFP/UFP、Rp/Rd 和连接方向；
- USB PD 报文的 SOP 类型、Message Header、Message ID、控制消息、数据消息、Data Object 和 CRC；
- GoodCRC、Source_Capabilities、Request、Accept、PS_RDY 的典型协商链路；
- PDO/RDO 的基本含义；
- PD 3.x、PPS、EPR 只在项目确认纳入范围后深入。

阅读方法：

1. 先看项目选定版本的 USB Type-C 规范中 CC 与角色部分；
2. 再看 USB PD 规范中的术语、报文格式和一条标准协商流程；
3. 用固定报文逐字段手算一次 Header 和 Data Object；
4. 再阅读开源实现，开源代码只能帮助理解，不能替代规范。

阶段产出：

- 能把一段已知报文转成 `PdFrame`；
- 能画出一次基础供电协商的时序图；
- 能定义界面需要的字段，而不是让 GUI 直接依赖原始字节。

## 4. 第四阶段：LVGL 与嵌入式 GUI

当前建议评估 LVGL 9.x，并固定一个明确的稳定版本，不跟随 `master` 漂移。

重点掌握：

- `lv_init`、tick、timer handler、display/input device 注册；
- draw buffer、`flush_cb`、单缓冲/双缓冲和 DMA 刷屏；
- RGB565、分辨率、色深、字体和中文资源的 Flash/RAM 开销；
- Widget、Style、Flex/Grid、事件回调和页面生命周期；
- LVGL 的线程上下文约束：业务线程通过消息或快照更新 UI，采集中断不得直接调用 GUI；
- 性能分析：刷新区域、帧率、CPU 占用、缓存命中、内存碎片和最坏刷新延迟。

建议先在 PC 模拟器完成界面，再移植到板端。首版页面建议只有：

1. 状态栏：连接、CC 通道、采集/丢帧状态；
2. 报文时间线：时间、方向、消息类型、Message ID、CRC；
3. 单帧详情：Header、Data Objects、警告；
4. 协商摘要：电压、电流、关键状态；
5. 操作：开始/暂停、清空、筛选、导出。

阶段产出：

- PC 模拟器可加载固定 JSON/C 测试数据；
- 板端能显示相同页面；
- 数据持续涌入时界面不阻塞采集；
- 对不同缓冲方案给出 RAM、帧率和延迟对比。

## 5. 第五阶段：RTOS、双核与联调

只有在需求和官方示例确认后再决定使用 FreeRTOS、厂商调度方案或裸机。

需要掌握：

- 任务、优先级、队列、信号量、互斥量和临界区；
- 中断到任务通知；
- 优先级反转、死锁、栈溢出和队列溢出；
- 双核共享内存、缓存一致性、所有权和消息传递；
- “采集不可被 UI 阻塞”的系统不变量。

阶段产出：

- 固定数据源持续输入时，GUI 可正常交互；
- 队列满时有明确统计和降级行为；
- 运行 2 小时无崩溃、无无限内存增长；
- 能测量采集到屏幕显示的端到端延迟。

推荐材料：

- 官方免费书 *Mastering the FreeRTOS Real Time Kernel*；
- *Making Embedded Systems, 2nd Edition*：重点读系统架构、调试、I/O、并发和资源约束；
- *Test-Driven Development for Embedded C*：重点学习把协议解析从硬件中拆出来测试。

## 6. 你暂时不需要先啃完的内容

- USB-IF 一致性认证全套测试；
- USB 2.0/3.x 高速数据链路协议；
- 完整 EPR/PPS/VDM/Cable Message；
- 自研 GUI 引擎；
- 复杂 3D、模糊、阴影等高成本视觉效果；
- 在需求未冻结前同时适配多种屏幕、RTOS 和主控。

这些内容会让项目看起来很“硬核”，也很容易把一年期项目做成一锅技术火锅。先让一条报文可靠地从采集端走到屏幕上，价值更高。

## 7. 官方入口与书目

- USB-IF USB Power Delivery 文档库：https://www.usb.org/document-library/usb-power-delivery
- USB-IF USB Type-C 文档库：https://www.usb.org/document-library/usb-type-cr-cable-and-connector-specification-release-24
- WCH CH32H417 产品页：https://www.wch.cn/products/CH32H417.html
- WCH CH32H417 EVT 与相关资料：https://www.wch.cn/downloads/CH32H417EVT_ZIP.html
- WCH 官方 CH32H417 示例仓库：https://github.com/openwch/ch32h417
- LVGL 稳定版本发布页：https://github.com/lvgl/lvgl/releases
- LVGL 文档：https://docs.lvgl.io/
- FreeRTOS 文档与免费教材：https://www.freertos.org/Documentation/02-Kernel/07-Books-and-manual/01-RTOS_book
- Pro Git 简体中文：https://git-scm.com/book/zh/v2
- Robert C. Seacord, *Effective C, 2nd Edition*, No Starch Press, 2024
- Elecia White, *Making Embedded Systems, 2nd Edition*, O'Reilly, 2024
- James W. Grenning, *Test-Driven Development for Embedded C*, Pragmatic Bookshelf

