# 协议学习官方资料

下载日期：2026-10-12。用户授权在项目文档目录保存资料；不表示产品协议范围冻结。原包、许可证和附带说明保留，不自动提交或上传这些文件。

## 首选正文

- 中文辅助：`TI_ZHCY216_USB_Type-C_Guide_CN.pdf`，《工程师指南之 USB Type-C》，ZHCY216，November 2024，72页。TI 官方中文资料，不是 USB-IF Type-C 2.5 或 PD 3.2 规范的中文译本。先读第5–8页；中文个别角色示例存在歧义/疑似误译（第6页供电设备、受电主机示例），以 USB-IF 原文为准；CC 承载的是 PD 信令而非负载功率，高速数据也不是 PD 报文本身。

- Type-C：`USB Type-C 2.5 Release 202603/USB Type-C 2.5 Release 202603/USB Type-C Spec R2.5 - March 2026.pdf`（Release 2.5，March 2026，442页）。
- PD：`USB_PD_R3.2_V1.2(3)/USB_PD_R3.2_V1.2/USB_PD_R3.2_V1.2_2026_08_19.pdf`（Release 3.2 Version 1.2，August 19, 2026，418页）。
- 入门辅助：`TI_SLYY109B_Type-C_PD_Primer.pdf`（Rev. B，March 2022，14页）；旧资料，不用其功率上限代替现行规范。

目录重复一层源于官方 ZIP 自带顶层文件夹，保留原有结构。优先读不带 Compared/redline 的主规范。`__MACOSX` 是原包附带元数据，不是可阅读 PDF；文件名虽带 PDF 后缀也不要打开。PD 包内 reorganization guide 带 Draft/Unofficial 标识，仅保留原件，不作为规范依据。

## 当前带读入口（已核实正文）

Type-C R2.5 §4.5.1.2.1 Detecting a Valid Source-to-Sink Connection：

- 第160页末：通过端接检测建立有效连接，介绍 Rp/Rd 模型。
- 第161页：Figure 4-5 上拉/下拉模型；Figure 4-6 电流源/下拉模型；端接区分与去抖说明。
- 第162页：Table 4-10 Source Perspective，按 CC1/CC2 检测结果区分状态及方向。

上述正文页码与 PDF 页序一致。学习者尚未完成新版读图，不记为已掌握。

## 来源与完整性

- Type-C 发布入口：https://www.usb.org/document-library/usb-type-cr-cable-and-connector-specification-release-25
- PD 发布入口：https://www.usb.org/document-library/usb-power-delivery
- TI：https://www.ti.com/lit/pdf/slyy109b
- 三个原始下载文件的直链、字节数和 SHA-256 见 `download-manifest.json`。
- 两个 ZIP 均通过 ZIP CRC 检查；解压后40份真实 PDF 均可打开，排除 `__MACOSX` 元数据。Type-C 第161页另做渲染核对。
- 尚未下载 WCH 或 ST 的新副本；WCH EVT 已有本地资料，当前无需重复下载代码包。
