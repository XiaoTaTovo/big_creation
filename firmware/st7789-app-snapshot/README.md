# ST7789 当前应用源码快照

2026-11-07 从活动 ST7789_TEST 工程复制，用于提交、审查和恢复本轮应用修改；本目录不是独立可构建工程，不作为第二个日常编辑入口。

保留 Common/Common、V3F/User、V5F/User、两核链接脚本及 LVGL 配置。WCH 外设库、启动文件、LVGL 库及 MRS 工程配置仍使用原工程依赖，未重复上传。恢复时先备份目标工程，并按相对路径对照合入；不要无条件覆盖其他版本。

默认 PD_CAPTURE_DEMO_MODE=1、PD_REPLAY_LOOP=0；V3 发送一次1022八帧，V5 默认竖屏摘要及最近8条。真实采集需关闭演示并接入 PD_Capture_Submit。详细接口、验证和已知限制见 docs/learning/pd-portrait-monitor.md。

两核源码分别构建后再生成配套 Merge.bin。2026-11-07 已发现本地 MRS Merge.bin 混入11月6日旧V3；这不是当前源码验收通过的镜像。没有发布可直接烧录的二进制。

source-sha256.json 是复制前原始字节的SHA256；Git按仓库规则将文本行尾规范化为LF，检出后的字节哈希可能不同。历史C文件注释编码保持原样。
