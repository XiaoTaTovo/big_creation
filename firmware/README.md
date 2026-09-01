# Firmware

嵌入式固件目录。工具链和芯片 SDK 尚未冻结，因此暂不导入大体积厂商工程模板。

建议后续结构：

```text
firmware/
├─ app/             应用入口与任务编排
├─ bsp/             板级支持、时钟、引脚和中断
├─ drivers/         显示、触摸、存储、通信和 PD 前端驱动
├─ capture/         采集、时间戳、缓冲区和统计
├─ pd_codec/        符号/帧恢复
├─ pd_protocol/     报文解析与 CRC
├─ pd_session/      协商事件归纳
├─ ui_model/        GUI 数据模型与消息通道
├─ ui/              页面与控件
├─ export/          对外数据导出
├─ platform/        RTOS/裸机和核间通信抽象
└─ vendor/          经审查的厂商 SDK 或子模块
```

在导入 SDK 前应先确认：许可证、官方版本、编译器版本、调试器、启动文件来源和是否使用 Git submodule。

协议解析代码应尽量可在 PC 上单独编译，以支持自动化测试。
