# INBOX

用于学习线和开发线向主控提交短回执。每条回执建议包含：

## D4-基础绘图页回执（2026-10-26）

- 任务：在当前 ST7789/LVGL 基线中先完成文字和基础图形绘制，降低进入协议首页的学习门槛。
- 依据：`CONTEXT.md` 确认当前基线为 CH32H417QEU-R0-1v1 + SPI ST7789 170×320；实际工程位于 `D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST`，不在本仓库 Git 根目录。
- 改动文件：外部工程 `V5F/User/main.c`。新增 `UI_DEMO_BASIC_DRAW` 开关和 `ui_basic_draw_create()`；页面包含 ASCII 文字、圆角矩形卡片、圆形状态点、分隔线、三条矩形条和箭头折线；原钟表页保留，开关改为 `0` 可恢复。演示电压/电流已标记为 `DEMO`，不代表实测报文。
- 验证：使用 MounRiver RISC-V GCC12 对当前 `main.c` 做 `-fsyntax-only` 检查，退出码 0；尝试 `mingw32-make -j24 all` 时，命令行环境缺少默认工具链名/生成 `.sources_list` 的 shell 引号兼容性，未完成完整链接。旧 ELF/BIN/HEX 时间早于本次修改，不能作为新页面构建或上板证据。
- 遗留：尚未在 MRS 中 Clean/Rebuild、烧录或上板观察；横竖屏、正式 PD 字段、真实数据接口仍未冻结。下一步由用户在 MRS 中重新构建并上板，只核对文字、边界、矩形、圆点、线和箭头是否显示。
- 追补：根据首次显示反馈，将状态文字缩短为 `WAITING`、底部说明改为 `TEXT + SHAPES`，并把箭头移到空白区域、增加为可见的 5 点折线；当前源码语法检查仍通过。
- 追补：状态圆点、`WAITING` 和 `CC1 LISTEN` 已改为 `card` 的子对象，位置改为相对卡片坐标，便于后续整体移动和理解父子层级。


## L1 学习线回执（2026-09-05）

- 任务：识别 CH32H417QEU-R0-1v1 板卡接口，定位 MounRiver Studio 帮助并给出首个程序下载/调试学习步骤。
- 依据：桌面 `CH32H417EVT/EVT/PUB/CH32H417评估板说明书.pdf`（V1.4，第 1–12 页）、`CH32H417SCH.pdf`、`CH32H417DS0.PDF`，官方 `GPIO_Toggle` V3F/V5F 工程，本机 MRS2 `.../mrs-team.mrs-vscode/resources/help/help.html` 与 `.../WCH-Link使用说明.pdf`。
- 结果：芯片 CH32H417QEU6，板卡 QEU6-R0；主要接口为 USER/RESET、USBFS、USBHS/USBSS、RJ45、SWD、LED/J3、Type-C/PD、电源 LDO/DCDC、扩展排针。SWD 为 PB9=SWDIO、PB8=SWCLK，另接 3V3 与 GND；WCH-Link 手册确认支持单线/两线调试。
- 例程事实：`GPIO_Toggle/Common/hardware.c` 实际翻转 PB1，不能直接假定板载 LED 会亮；V3F/V5F 目录不可互换。`SRC/Debug/debug.h` 给出 V3F 地址 0x00000000、V5F 地址 0x00010000，默认双核；V3F 默认 USART1/PA9，V5F 默认 USART8/PB4。
- 帮助定位：当前安装目录未找到截图中的旧版 `MounRiver_Help.pdf`/`MounRiver_ToolbarHelp.pdf`；现用 `D:\software\Code\MounRiver_Studio2\resources\app\extensions\mrs-team.mrs-vscode\resources\help\help.html`，5.2/5.4/5.5/5.6 分别对应打开工程、编译、调试、下载。
- 教学（未执行）：打开对应 `.wvproj`；编译并检查 `.elf/.hex/.bin`；WCH-Link 接 PB9/PB8/3V3/GND 并选 RISC-V；V3 下载地址 0x08000000，V5 地址 0x08010000，第二次勿 Erase All；MRS2 从 Flash→下载配置进入，F8/下载按钮执行，Debug/Start Debug 进入调试。说明书指出 V3F 不支持单步，V5F 断点要在 V5 唤醒后设置。
- 改动与验证：仅追加本文件；只读检查官方资料、工程文本和帮助 HTML，未连接 WCH-Link、未烧录、未复位、未运行真实硬件。
- 遗留：用户确认说明书图片与实物同款，但未提供实物近照；跳帽、PB1 与 LED/J3 的实际连接仍需现场核对。建议 L2 先做 MRS2 导入与静态编译并保留日志，再安排受控硬件下载。
- 对话/任务编号与日期；
- 完成了什么；
- 依据的文件或实验；
- 改动文件；
- 验证结果；
- 遗留问题和建议的下一步。

主控吸收后更新 `CONTEXT.md` 和 `TASKS.md`，不要把这里当完整日志。

## L1 代码讲解补充（2026-09-05）

- 用户已成功编译 `GPIO_Toggle_V3F`，输出窗口显示生成 ELF/BIN/HEX；这只证明编译链成功，不代表已下载、运行或串口接收成功。
- `hardware.c`：`GPIO_Init1()` 开 GPIOB 时钟，配置 PB1 为高速推挽输出；`Hardware()` 每 250 ms 用 `i` 在 `Bit_SET/Bit_RESET` 间切换 PB1，并永久循环。
- `main.c`：启动时钟、更新时钟变量、初始化延时、调用 `USART_Printf_Init(115200)`，然后 `printf` 打印系统时钟；默认 `Run_Core` 是双核模式，V3F 分支会唤醒 V5F 并进入等待事件，只有之后才进入 `Hardware()`。工程定义 `Core_V3F`，但没有覆盖 `Run_Core` 默认值。
- 串口链：`main.c` 包含 `debug.h`；工程通过 `.project/.wvproj` 链接 `SRC/Debug`。V3F 下 `debug.h` 选 `DEBUG_UART1`，`debug.c` 的 `USART_Printf_Init` 开 USART1/GPIOA 时钟，将 PA9 复用为 AF7、设置 115200/8N1/仅发送；`_write()` 将 `printf` 每个字符送入 USART1。`hardware.c` 不需要重复配置串口。
- 用户截图中的 `Output` 是构建输出，不是 UART 终端；真正串口还需要下载并运行程序、物理连通 PA9 到串口接收端、打开对应 COM 口的串口终端。示例只配置 TX，没有 RX。

## L1 单核配置教学补充（2026-09-05）

- 已根据本机 MRS2 帮助 5.4.2 与用户截图确认属性界面入口：右键活动工程 → 工程属性 → 左侧 C/C++ Build → Build Settings → 中央 GNU RISC-V Cross C Compiler → Preprocessor。
- 当前工程 `V3F\\.cproject:117-118` 和 `GPIO_Toggle_V3F.wvproj` 的 `ccompiler.preprocessor.defined_symbols` 只有 `Core_V3F`，没有 `Run_Core`；公共 `SRC\\Debug\\debug.h` 因此按 `#ifndef Run_Core` 默认双核。
- 单核做法：在上述 Preprocessor 的 Defined symbols (-D) 中保留 `Core_V3F`，新增 `Run_Core=Run_Core_V3F`，Clean/Rebuild，并检查编译命令有 `-DRun_Core=Run_Core_V3F`。不建议改公共 `debug.h`。
- 只下载 V3F 且未加该宏时，`main.c` 会先执行两次 `printf`，随后进入双核分支的 `PWR_EnterSTOPMode(...WFE)` 等待 V5F，通常不会进入 `Hardware()`；加单核宏后直接调用 `Hardware()`。

## L1 串口助手定位补充（2026-09-05）

- 本机随 MounRiver 安装的官方串口工具为 `D:\\software\\Code\\MounRiver_Studio2\\resources\\app\\resources\\win32\\others\\COMTransmit\\COMTransmit.exe`；说明书为同目录 `Doc\\COMTransmitDoc_CH.pdf`，V1.6。
- COMTransmit 手册第 1、3、8、13 页覆盖串口选择、基础收发、默认串口参数和接收区；使用时需先识别 WCH-Link 的 COM 号，再设置 115200、8 数据位、无校验、1 停止位、无流控，打开串口后在接收区观察。
- WCH-Link 手册 `...\\SWDTool\\default\\Doc\\WCH-Link使用说明.pdf` 第 1–3 页确认 WCH-Link 有独立 RX/TX 串口桥和波特率；该桥仍需目标 UART TX→Link RX、共地，不能把 SWD 当 UART。V3F 例程输出脚为 PA9。
- WCH-Link 手册 5.2.11 的 SDI Printf 支持列表不含 CH32H417；本项目不要把 SDI Printf 当作 CH32H417 的普通 UART 输出方案。
- MRS2 的 `Output` 仍是构建输出，不是串口接收区；程序启动打印只发生一次，串口终端应在上电/复位前打开，否则需重新启动目标才能重现两行文本。

## L1 视觉免试题附件（2026-09-08）

- 任务：仿照往年视觉免试题，设计“旋转箭头识别卡”完整题目，区分基础/进阶，并制作可直接发放的附件和详细解决思路。
- 依据：前一轮已核对的 2025 简易数字识别、答题卡识别、三子棋识别题型；本轮按传统 OpenCV 视觉流程设计，避免深度学习和额外专用硬件。
- 结果：完成题目说明、基础/进阶验收条款、评分表、打印说明、三张测试卡、标准答案 JSON、参考实现和题目/解题 DOCX。
- 改动文件：新增 `docs/视觉免试题_旋转箭头识别卡/`；新增参赛者题目包、完整题目包和组织者答案包 ZIP。未改动固件、硬件和测试目录。
- 验证结果：参考程序已对基础卡、旋转透视卡、含空位卡分别运行，识别结果与标准答案一致；DOCX 文本可正常解包读取；PNG 附件已检查尺寸和可视效果。
- 遗留：实际打印尺寸、摄像头距离和现场光照仍需由科协在发题前做一次现场验收；建议先打印 A4 样张，确认黑色边框在实际镜头下不会断裂。

## D4-3 回放开发回执（2026-10-29，主控已吸收）

- 依据：CONTEXT 的 SPI ST7789 170×320 基线及真实 1022LISTEN 八帧，未改变最终接口契约。
- 外部工程改动：main.c 加回放入口并保留原始备份；新增 pd_replay_model.h、pd_replay_ui_impl.h。
- 文档项目产出：tests/test_pd_replay.c、tools/verify_pd_replay.py、docs/learning/day2-pd-replay.md；已同步 CONTEXT/TASKS。
- 证据：主机测试通过；341 个活动源文件全新 WCH 编译链接通过，FLASH 129740 B；构建过程和产物位于外部工程 build/day2-verify。
- 边界：回放值不是测量值，t8 单位未知，坏 CRC 是单独测试模式；驱动仍同步阻塞。未操作硬件，上板验收待用户完成；后续扩展先处理代码空间余量。

## 存储与整块刷屏教学验证（2026-10-29）

- 用户要求手把手完成两个实验；本轮仅在外部工程 build/memory-block-lesson 制作候选文件并验证，活动源码保持未修改。
- 新鲜基线：异常回放宏当前为 1，341 文件全量编译通过。候选一把 rodata 从 ITCM 移入现有 DTCM data 复制区，并将 FLASH 配额改成 256 KiB；ITCM 使用从 129108 B 降到 111904 B，RAM 从 71972 B 增至 89180 B，FLASH 负载基本不变。
- 候选二：disp_flush 每区域设置一次窗口，再沿用 Lcd_WriteData_16Bit 发送全部像素；编译链接通过。主机替身检查窗口次数、像素顺序/数量和完成通知通过；未验证真实 SPI。
- 教学主来源：docs/learning/memory-and-block-flush-lab.md。等待用户按顺序修改、构建及手动上板反馈；不标记已部署或硬件通过。

## 2026-10-30 存储调整验收

用户完成 V5 链接脚本修改并提供 MRS 构建前后截图：text/data/bss 从 129420/324/71648 变为 112216/17532/71648 字节，与候选验证一致。已核对活动 Link_v5f.ld：FLASH=256K、ITCM RAM_CODE=128K，rodata 从 highcode 移至 data。用户报告手动烧录后运行无异常，任务一按“编译与基本上板回归”验收通过；不据此宣称长期稳定性或任务二通过。下一步为每刷新区域只设置一次 LCD 窗口。

## 2026-10-30 整块刷新基本验收

用户报告任务二完成且整个画面无异常。已核对活动 lv_port_disp.c：LCD_SetWindows 在像素循环外，每区域一次；循环调用 Lcd_WriteData_16Bit，最后调用 lv_display_flush_ready。按源码检查与用户上板反馈验收基本显示功能通过；没有新增帧率/总线耗时测量，SPI 仍为同步轮询且每像素切换片选。后续先理解存储与裁剪，不自动改动 lv_conf.h 或删除库文件。

## 2026-10-30 双缓冲配置验收

用户提供 MRS 构建截图：text=112200、data=17532、bss=75048、dec=204780。相较单缓冲 bss=71648 增加 3400 B，符合新增第二块 170×10×2 RGB565 局部缓冲区；用户未报告烧录异常。双缓冲配置阶段可视为通过。DMA 与非阻塞刷屏仍未实现。

## 2026-10-30 连续 SPI 发送改造

用户授权完成连续 SPI 发送改造。外部工程 User/lcd.h 新增 LCD_WritePixels_Continuous() 声明，lcd.c 在整块发送期间保持 CS 低、DC 高，逐像素按 RGB565 高字节/低字节调用 SPI1_Write；lv_port_disp.c 计算区域像素总数后调用该函数。已为三个文件保存 before-continuous-spi-20261030.bak。改动文件单独交叉编译通过；依据当前 MRS 活动源文件全新编译链接 341 个文件通过，FLASH 129796 B / 256K，RAM_CODE 111952 B / 128K，RAM 92580 B，bss 75048 B。尚未烧录或用逻辑分析仪测量，仍为同步轮询 SPI，不是 DMA/非阻塞。

## 2026-10-30 DMA 阻塞验证版实现

用户明确授权直接修改。检查发现此前 spi.c 的 DMA 草稿含错误 `void SPI1_DMA_Init(void); {`，已修正并保留 `spi.c.before-dma-repair-20261030.bak`、`spi.h.before-dma-repair-20261030.bak`、`lv_port_disp.c.before-dma-repair-20261030.bak`。当前实现：SPI1 TX 使用 DMA1 Channel3、DMAMUX 请求 63、8-bit 内存到 SPI1->DATAR、Normal 模式；lv_port_disp 使用 3400 B staging buffer 将 RGB565 转为高字节在前，LCD 每区域一次窗口，CS/DC 包住整块 DMA 发送；DMA 完成后阻塞等待 SPI BSY 清零，再 flush_ready。全量 WCH 编译链接 341 源文件通过：FLASH 131152/262144 B，RAM_CODE 113260/131072 B，RAM 96028 B，bss 78448 B；仅有旧演示函数未使用警告。当前仍是阻塞验证版，不是 DMA 中断异步双缓冲；未烧录或上板，等待用户手动验证花屏、颜色、卡死与失败日志。

## 2026-10-30 DMA 阻塞版上板回归

用户先检查接线后反馈：DMA 版本屏幕恢复正常，未出现 `LCD DMA: transfer failed`。结合前述全量编译链接通过，DMA1 Channel3/SPI1_TX 阻塞发送、整块窗口和 RGB565 字节顺序完成基本上板验证。该结果不等于 DMA 中断非阻塞或性能实测通过；下一步可设计完成中断，再将 flush_ready 延后到 DMA 完成回调。


## 2026-10-30 回执：DMA 异步提交修改

用户授权的六个外部工程文件已修改并备份：V5F/User/spi.c、spi.h、lv_port_disp.c、lv_port_disp.h、ch32h417_it.c、main.c。全量 WCH 编译链接及主机显示状态桩测试通过；证据在 ST7789_TEST/build/async-dma-verify/compile.log、pd_replay.elf、test_async_display.c 与 test_async_display.exe。主控已将当前事实写入 CONTEXT.md、待上板任务写入 TASKS.md。遗留：真实中断/CS 时序、显示与长期运行未测；LVGL 缓冲区复用等待保留。没有烧录、复位或其他硬件动作。


## 2026-10-30 回执：PC 回放第一版已实现

用户明确要求直接完成。新增 pc_replay/replay_cli.c、run.py、start.cmd、test_replay.py、README.md、fields-v1.md、VALIDATION.md、.gitignore；未改固件模型。实现终端回放、CSV 输入输出和共享字段/状态函数。执行 python pc_replay/run.py --test，9 项测试全部通过；正常与模拟坏 Request 的结果已导出至 build/，详情见 VALIDATION.md。当前事实与待办已同步 CONTEXT.md、TASKS.md。遗留：原始临时 TXT 不可访问，未实现原始日志解析、GUI 或实时输入，上板对照待用户完成。


## 2026-10-30 双核学习资料核对

新增官方应用手册 docs/references/CH32H417RM.PDF（V1.8）、来源记录及 docs/learning/dual-core-reading.md。核对当前 V3 启动/HSEM0/日志，V5 仍本地回放；识别 V3 旧构建绝对路径和未划共享区两项实验前置问题。定位 IPC/HSEM 官方样例，未改固件或执行硬件实验。手册页码、HSEM 读返回值与本地库差异和学习顺序见阅读路线。
