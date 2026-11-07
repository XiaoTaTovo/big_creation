# 竖屏摘要与核间报文显示（2026-11-07）

## 本轮结果

实际工程：`D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST`。
沿用 CONTEXT 已确认的 CH32H417QEU-R0-1v1、170×320 ST7789、双局部缓冲、异步 DMA 和 16 槽共享队列。没有修改链接布局、SPI/DMA 或屏幕方向寄存器。

默认 V5 页面恢复为竖屏摘要：标题、数据源、协商阶段、PDO、REQ V、REQ I、RX/DROP 和最近 8 条记录。为容纳八行历史，压缩旧摘要布局；并非逐像素恢复旧页面。请求电压/电流不是实测值。
底部自上而下从旧到新，左列为完整十进制采集 sequence（不是协议 Message ID），右列为消息类型和校验标记。CAPS/REQ/GCRC 分别代表能力/请求/确认，+ / ! / ? 分别代表通过/失败/未知。底部使用12像素字体，80px序号列+70px消息列；不显示发送方、协议ID和原始字节，这些仍保留在记录中。

## 数据链路与显示节奏

V3 日志样本或队友适配器 → PD_Capture_Submit → 共享队列 → V5 按记录解析 → 最近八条缓存和协商摘要 → 每 200 ms 按变化字段更新 LVGL。

接收仍在主循环每轮最多取四条；200 ms 只限制 UI 提交，不限制接收。LVGL 原有等待 DMA 的边界仍存在，不承诺严格实时或任意流量下无丢失。
字段名只创建一次；数值固定位置/宽高，比较最终字符串后再更新。没有整页清空或重建。滚动列表内容发生变化时相应行仍需重绘。

当前离线样本按 t8 毫秒的相对时序运行，默认只发送一轮八条，然后停止回放计时并保留页面，页面标记 REPLAY 1022。PD_REPLAY_LOOP=0 为默认；仅压力测试设为1时才在轮末停留3秒后重复。V3 仍使用 Delay_Ms(1) 累计的演示时钟，执行开销和同毫秒记录会产生调度偏差，不能作为精确采集时钟验收。时间戳为回放时间轴；生产模式由采集端填写自启动起的微秒时间戳。

## 开关及队友入口

- `Common/Common/pd_source_config.h` 的 PD_CAPTURE_DEMO_MODE=1 为日志回放；队友接入后改 0，两核一起重编译。0 不会自动生成真实采集数据，必须接上采集适配器。
- `V5F/User/main.c`：UI_PD_MONITOR=1、UI_DEMO_IPC_COUNTER=0 默认摘要；后者改 1 切回诊断页。
- 原 pd_replay_model.h / pd_replay_ui_impl.h 保留。要使用旧的 V5 独立演示，关闭以上两项并打开 UI_DEMO_PD_REPLAY；此模式不验收 V3→V5。

队友继续只依赖 pd_capture.h：先零初始化 PdCaptureRecord，填写 sequence、timestamp_us、header、kind、flags、cc、sop、crc_status、length_status、payload_len、payload，再从 V3 主线程单生产者调用 PD_Capture_Submit(&record)。不得从中断直接调用；中断采集需要自己的交接缓存。

- payload 不含 Header 和 CRC，普通数据对象按小端字节顺序；最大保留 28 字节，裁剪时设置 TRUNCATED。
- 时间有效才置 HAS_TIME，头有效才置 HAS_HEADER；无法确认 CRC/长度时填 UNKNOWN。
- sequence 每捕获一条递增，即使提交失败也递增。OK 表示已复制进队列，可复用局部变量，不表示屏幕已刷新。
- FULL：新记录未入队，不能假装成功；INVALID：检查参数；NOT_READY：接收端未就绪或传输故障。不得循环忙等重试或清空共享区。

新增解析层支持这组样本的普通 SOP 固定供电协商，最多缓存 7 个 PDO；不宣称完整 PD、PPS 或 EPR。未知/扩展报文仍保留于列表。CRC/长度不可信、截断、缺帧、复位/拔出、未知协商消息会保守清除摘要，等待新的能力列表重新建立状态。

## 文件和验证

外部工程新增：pd_source_config.h、pd_monitor_model.h、pd_monitor_ui.h；修改 V3/V5 main.c，原 main.c 已保存 before-portrait-20261107.bak。原文件含历史非 UTF-8 注释，本轮保留原字节，新文件和文档使用 UTF-8。

软件验证：

- tests/test_pd_monitor.c：实际队列与解析串联，8 帧结果 READY/PDO3/12000mV/1670mA、历史覆盖、序号缺口、坏 CRC/长度、拔出、扩展报文、非法 PDO。
- tests/test_pd_monitor_ui.c：LVGL 接口桩验证控件边界、显示文本、重复渲染零标签更新、9→10 位置宽度不变。不是像素级渲染或真实字体裁切验收。
- 原回放状态测试及 15 组共享队列测试通过，随机 FIFO 交付 89066 条。
- WCH GCC12 全新编译链接：V3 45 源文件；V5 341 源文件。V5 FLASH 131384/262144 B，RAM_CODE 113636/131072 B；仅保留未选中旧演示函数/变量的未使用警告。
- 证据目录：外部工程 build/portrait-20261107；最终 V3 ELF 在 v3，V5 ELF 在 v5-final。sha256.json 记录源码及 ELF 哈希。

## 用户上板验收

1. 在 MRS 重编译当前 V3、V5 工程，确认都成功，生成本次配套 Merge.bin。不要直接拿独立验证 ELF 当合并镜像。
2. 用户手动烧录并冷启动；本轮没有执行下载、复位，也没有覆盖 MRS obj 或 Merge.bin。
3. 应看到竖屏摘要、REPLAY 1022，以及八行列表。八条结束后保持不变，显示 RX=8、序号1～8、POWER READY、PDO 3/5、12.00 V、1.67 A。
4. 确认底部八行全部可见，无越界、残影、明显整屏闪烁；静止保持阶段不持续闪。检查 DROP=0、无 LCD DMA 故障。
5. 默认单轮结束后连续观察至少30秒，不应周期性回跳；若仍闪，记录是数值区域还是整屏，再测量刷新区域/传输耗时决定是否提高局部缓冲高度。当前不宣称已消除硬件可见闪烁。

## 单次回放修复验证（2026-11-07）

本轮修改 V3 main.c、pd_source_config.h、pd_monitor_ui.h、lv_conf.h，修改前保存 before-single-replay-20261107.bak；环形缓存逻辑保持不变。循环回放默认关闭，接收始终继续。

扩充 tests/test_pd_monitor_ui.c：第九条后逐行序号2～9、连续150次render零标签更新、UINT32_MAX完整格式化。提取实际V3回放循环做30000ms主机仿真，确认只提交8条。原解析测试和15组队列测试通过。12px字体的未加字距字符advance检查覆盖十位序号和最长消息名；实际像素仍待上板。

两核全新构建通过（45/341源文件）。V5 FLASH=143012/262144 B、RAM_CODE=113768/131072 B、普通RAM=107856/261376 B；增加12px字体占用约11KB常量存储。证据及SHA256在外部工程 build/single-replay-20261107。未覆盖MRS obj/Merge.bin，未进行硬件操作。用户需重新构建V3和V5、生成配套Merge.bin并手动烧录，验收RX=8和八行静止30秒。

## 当前上板异常定位与待办

逐字节检查本地MRS Merge.bin：偏移0含2026-11-06 22:46:45旧V3 bin，偏移65536含2026-11-07 10:50:25新V5 bin。新V5显示递增序号而旧V3循环发送，解释了周期性CAPS/请求未知/恢复READY。需重新构建V3和V5并合并，不能据旧镜像判定单次回放已上板。

另有已确认UI缺陷：标题“SEQ / MSG / CHECK”是整条标签，数据实际为左右两列，标题与列不对齐。计划改为两个独立标题“SEQ”和“MSG / CHECK”，当前尚未实现；不要把MSG下方空隙误认为报文解析为空。

仓库应用快照位于firmware/st7789-app-snapshot，依赖和恢复边界见其README。
