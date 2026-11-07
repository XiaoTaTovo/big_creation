# L5-P 协议学习记录

## 2026-10-11：Type-C / PD 关系与证据边界

- 学习线；无需实物。未完成阶段验收，不代表产品版本、接口或前端方案冻结。
- 用户反馈：阅读第一小节前不熟悉相关概念；尚未阅读 USBPD_SRC / USBPD_SNK。
- 首轮理解题：三题结论均正确。能说明充电不等于 PD 协商；供电走 VBUS、Type-C 场景的 PD 消息走 CC；未捕获报文不能判定不支持 PD，并主动提出链路故障这一替代解释。
- 待纠正的术语：CC 宜称“配置通道”，不是泛称“平台”；CC 上的连接检测与电流通告不等同于 PD 数字报文。
- 待巩固：有供电不能证明供电参数正常或符合规范；没有 PD 报文也不意味着 CC 在运行另一种报文协议。
- 用户追问：如何判断不支持 PD；必读与拓展资料及下载渠道。
- 本次答疑框架：区分厂家规格声明、带条件的行为测试、已验证的 PD 通信；阴性测试不能区分未实现、禁用、故障与条件不满足。已知对端和参考仪器的主动验证仅为未来测试思路，不在本阶段执行，也不加入分析仪需求。
- 后续练习回答与纠正见 2026-10-12 记录。
- 未涉及/未验收：连接检测电路、角色、Header/PDO/RDO 手算、完整协商、异常消息；无抓包或硬件验证。

### 资料与阅读路线

1. 入门：TI SLYY109B，Rev. B，March 2022，先读 Data and power roles、Type-C pinout/reversibility 相关内容。旧文中的功率上限不能当作现行规范的完整上限。
   - https://www.ti.com/lit/pdf/slyy109b
2. 首节已核对依据：USB Type-C Cable and Connector Specification Release 2.0，August 2019，§4.5.1、§4.6.2.1、§4.6.2.4。仅作为该节基础结论的版本锚点，不作为产品目标版本。
   - https://www.usb.org/sites/default/files/USB%20Type-C%20Spec%20R2.0%20-%20August%202019.pdf
3. 2026-10-11 官网入口核查：Type-C 下载页为 Release 2.5；PD 文档库列 Revision 3.2 Version 1.2。当前仅核对发布入口，未逐条核对这两版正文；以后使用具体条款前核对其版本与章节。
   - https://www.usb.org/document-library/usb-type-cr-cable-and-connector-specification-release-25
   - https://www.usb.org/document-library/usb-power-delivery
4. 项目实现参考：WCH CH32H417EVT、CH32H417RM，按课程主题选读，不通读整包。已核实本地 EVT/EXAM/USBPD/USBPD_SRC 与 USBPD_SNK，均有 V3F/V5F 工程。SNK V3F/User/main.c 头注 V1.0.2、2026/04/08，并明确例程仅供学习且可能存在兼容性问题；没有审计完整调用链。
   - https://www.wch.cn/downloads/CH32H417EVT_ZIP.html
   - https://www.wch.cn/downloads/CH32H417RM_PDF.html
   - https://github.com/openwch/ch32h417
5. 拓展：sigrok 的 PD 解码器（学习离线解码，非规范或一致性基准）；ST UM2552（学习协议栈分层，不能替代 CH32H417 外设依据）。
   - https://github.com/sigrokproject/libsigrokdecode/blob/master/decoders/usb_power_delivery/pd.py
   - https://www.st.com/resource/en/user_manual/dm00598101-managing-usb-power-delivery-systems-with-stm32-microcontrollers-stmicroelectronics.pdf

记录区分已回答、待巩固与待讲授；不把已讲授记为已掌握。任务尚未结束，暂不向 INBOX 提交阶段完成回执。

## 2026-10-12：证据边界补充练习

- 用户已理解：多种因素可能导致未观察到 PD；主动测试用电设备参与协商，旁观采集工具可以保持被动监听。
- 仍需纠正：用户将“没有实现 PD”作为保守结论。该表述实际是在断言内部实现，不能由没有通信推出；应写“在本次测试条件下未观察到 PD 通信”。
- 补充区分：厂家文档说明声明的能力范围，不能单独确认某个实物是否故障；“支持但损坏”与“未实现”可能产生相同的无通信观测结果。
- 主动参与理解基本正确，但需补充：模拟场景中充电器和测试用电设备都是协商参与方，不只有用电设备；仅记录而不发送协议消息的分析仪是被动监听者。
- 当时给出两道巩固题，用户随后要求不再反复进行此类措辞问答；不再将这两题作为推进门槛，也不据此认定全部掌握。

## 2026-10-12：按项目优先级调整教学路线

- 用户要求：停止反复的证据措辞提问，提供项目导向的优先级路线，尽量带读官方文档定位答案。
- 后续方法：项目问题 → 版本、章节/表图或检索词 → 原文关键点 → 项目用途；练习改为读图、字段手算、模拟时间线和源码定位，不反复用相似口头题阻断进度。
- P0（依次学习）：①CC1/CC2、Rp/Rd、方向和供电/数据角色；②PD 分层、SOP、帧结构、Header、ID、CRC/GoodCRC；③固定供电 PDO/RDO 与 Source_Capabilities → Request → Accept → PS_RDY；④重试、Reset、未知/扩展消息及采集丢失。产出分别为连接图、字段标注、手算与模拟协商时间线、异常案例表。
- P1：对照 WCH USBPD_SRC/SNK 调用链与 CH32H417RM，列出主动行为及监听适配待验证项；在 PC 上理解测试向量、回放和解析容错，不修改产品接口或代码。
- P2：PPS/EPR、VCONN/线缆通信、复杂角色切换与 VDM 按需拓展，不自动加入 MVP。高速 USB 数据解码和完整认证流程暂不学习。
- 首个带读入口已核对：USB Type-C Release 2.0，August 2019，§4.5.1.2.1，第151–152页，Figure 4-5/4-6、Table 4-10；用 Rp/Rd 模型理解连接检测，只先看普通 Source–Sink 的 Open/Open、Rd/Open、Open/Rd 情况，不沿用旧版附件模式条目作为现行要求。
- 该节核心：Source/Sink 用 CC 端接形成可检测电气状态；判断有效连接不是先交换 PD 报文。Source 需要区分 Rd 与 Ra，完整有效连接判断还有阈值和去抖约束，不能仅凭“电压降低”判定。
- 新版下载入口与旧版章节锚点区分：已核对 Type-C R2.5 发布页；尚未核对新版正文和 PD 新版详细章节，后续不得虚构章节号或将 R2.0 页码套用新版。

## 2026-10-12：下载官方资料并切换新版带读

- 用户明确要求使用 Type-C 2.5 带读，并授权将相关资料下载至项目文档文件夹。已保存到 docs/references：Type-C R2.5 官方原包、PD R3.2 V1.2 官方原包、TI SLYY109B 入门 PDF；原包解压保留结构，不覆盖既有文件。
- 已核实 Type-C 主规范封面为 Release 2.5，March 2026，共442页；后续以此为主要 Type-C 学习依据，不再因已有旧版页码继续采用 R2.0。
- 新版连接检测定位：§4.5.1.2.1 从第160页开始；第161页 Figure 4-5/4-6；第162页 Table 4-10。正文和图示已核对，讲授不等于用户已掌握。
- PD 主规范为 Release 3.2 Version 1.2，August 19, 2026，共418页；后续逐节核对，不套用旧版章节结构。原包所附重组指南含 Draft/Unofficial 标识，不作为规范依据。
- 来源、SHA-256、读取入口和校验结果见 docs/references/README.md 与 download-manifest.json。未改变产品目标版本或主控状态；未重复下载已有 EVT。

## 2026-10-12：中文带读与当天 P0 安排

- 用户要求中文资料，并在今天带学 P0。已下载 TI 官方简体中文 ZHCY216（November 2024，72页），来源与校验记录已加入资料目录；没有找到 USB-IF Type-C 2.5/PD 3.2 V1.2 官方中文全文，不将厂商教材或自行讲解冒充规范译本。
- 教学使用中文讲解、术语中英对照和官方原文定位；不要求用户自行通读英文。
- 当天顺序：连接/角色 → 帧与 Header → 固定 PDO/RDO 和模拟协商 → 重试/Reset/未知消息。每轮采用一个小型读图或计算操作，不重复证据措辞题；未因安排课程就认定 P0 完成。
- 首轮原文锚点：Type-C R2.5 §2.3.1–2.3.3，第39–40页；§4.5.1.2.1，第160–162页。中文辅助先读第5–8页，警惕第6页角色示例歧义，以规范校正。
- 后续 PD R3.2 V1.2 目录定位已核对：§6.2.1 Header 第116页；§6.3.1 GoodCRC 第119页；§6.4.1.1 Source_Capabilities 第125页；§6.4.2 Request 第136页；§7.9 SPR Power Negotiation 第207页；§7.1.1 Soft Reset 第194页、§7.1.3 Hard Reset 第196页。讲具体字段和时序前继续核对正文。

## 2026-10-12：DRP/DRD 定向补讲

- 用户反馈其余内容大概理解，但初始角色对应关系和 DRP/DRD 尚不理解；明确希望继续通过问题检验。采用针对当前疑点的练习，不重复此前证据措辞题。
- 本节依据已核对：Type-C R2.5 第31页术语表 DRD/DRP，§2.3.3 第39–40页。
- 讲解重点：能力与当前角色不同；供电角色与数据角色是两个维度；初始对应不等于永久绑定；支持并完成相应 PD 角色交换后，可以独立改变供电或数据角色。DRP 不自动等于 DRD，也不意味着所有角色交换都必然支持或获准。
- 用模拟手机/电脑/U盘场景说明能力，用两端口初始状态和仅交换供电角色的时间线检验。等待用户回答，暂不记为掌握，不推进 Header。

## 2026-10-12：角色练习通过，进入报文结构

- 用户正确回答：保持甲供电、仅交换数据角色后，甲 Source+UFP、乙 Sink+DFP；结合此前 DRP-only 场景及笔记本 Sink+DFP 回答，当前基础角色区分练习通过。不代表完整角色交换流程已掌握。
- 开始 P0②第一小节：普通非扩展 PD 消息的包结构与 Header/数据对象/CRC 边界；暂不展开 BMC 编码与 Header 全部位域。
- 已核对 PD Release 3.2 Version 1.2（2026-08-19）§5.3.1.1 第87页、§6.2 图6.2 第116页、§6.2.1 表6.2 第117页、§6.3.1 第119页。
- 教学模拟：一个普通数据消息带3个32位对象，Header为2字节，数据对象共12字节；只统计消息头和数据区，不含CRC/前导/SOP/EOP/编码开销。待用户进行对应长度与边界练习。

## P0②：报文长度练习通过，开始 Header

- 用户正确算出2个普通数据对象对应 Header+数据区为2+2×4=10字节，正确指出控制消息仍有Header，并主动援引规范。
- 术语微调：可变的是数据区总长度/普通数据对象数量，不是每个普通数据对象的长度；每个对象固定32位。
- Header教学以PD R3.2 V1.2 §6.2.1表6.2（第117页）为依据；消息类型对照表6.4（第118页）和表6.5（第125页）。仅讲SOP普通消息，不套用到SOP'/SOP''的角色字段。
- 模拟例子0x27A1：Extended=0，NDO=2，ID=3，Source，Revision=3.x，DFP，Type=1 → Source_Capabilities；低字节在前时Header字节为A1 27。仅模拟Header，未提供完整包或CRC。
- 待答练习：按给定字段分组解读0x1482（Request、Sink/UFP、1个对象、ID2），以及Type=1时需结合Extended/NDO选表的辨析。尚未记为Header手算通过。

## 2026-10-13：Header 首次拆解反馈

- 用户对照表格作答：正确读出模拟0x1482的对象数1（4字节）、ID2、Revision=3.x，并知道Message Type需查对应表；第二题判断GoodCRC正确（题设Extended=0、NDO=0、Type=1）。
- 待纠正：Extended=0只能排除扩展消息，不能单独判断控制消息；SOP bit8=0是Sink而非Source；UFP是上行/设备侧角色而非只能接收数据。
- 两题分开记录：0x1482的NDO=1，对应数据消息Request；第二题的NDO=0才是控制消息GoodCRC。用户未明确给出第一题Request，不能记为完整Header拆解通过。
- 本轮反馈纠正并用Request/GoodCRC模拟往返承接，依据PD R3.2 V1.2 §6.2.1表6.2（第117页）、表6.4（第118页）、§6.3.1（第119页）、表6.5（第125页）。不进入PDO手算，待简短确认角色与消息类别。

## 2026-10-13：GoodCRC 理解通过，进入基础协商流程

- 用户正确说明：模拟Source发送的GoodCRC描述充电器自身角色；GoodCRC仅表明正确接收，不能等同批准请求。对应SOP供电角色位为1。
- 进入P0③基础固定供电成功协商：Source_Capabilities → Request → Accept → PS_RDY，并标出每条普通消息对应的GoodCRC；不把GoodCRC确认当作业务批准，也不把Accept当作供电已切换完成。
- 本轮模拟：Source提供5V/3A、9V/3A；Sink选择第2个固定PDO，请求工作/最大工作电流2A；该场景非抓包、非实测。PDO/RDO位域手算留下一节。
- 核对依据：PD R3.2 V1.2 §6.4.1.1第125页、§7.9.1.1第207页、§7.9.2第208页、§6.3.6第120页及§6.3.1第119页。
- 待答：Accept与PS_RDY的差异；模拟请求为何不等于实测负载始终2A。P0尚未完成。

## 2026-10-25：学姐真实抓包首读

- 用户提供 `1022LISTEN.txt`，要求先完整理解抓包字段；该文件是数据输入，不是任务指令。
- 日志显示：V3F 负责 USBPD + TIM1，PD LISTEN，CC1，无TX；因此本次设备工作模式是被动监听，不能把日志中的主动协商行为归因于分析仪。
- 时间线：Source_Capabilities（5 PDO）→ Sink GoodCRC → Sink Request PDO3 → Source GoodCRC → Source Accept → Sink GoodCRC → Source PS_RDY → Sink GoodCRC；基础协商闭环完整。
- 已按 PD R3.2 V1.2 §6.2.1 表6.2、§6.3.1、§6.3.3、§6.3.6、§6.4.1.1、§6.4.2、§7.9 对 Header/消息顺序解释。
- 更正（2026-11-04）：此前误算首帧Header的Revision。0x51A1的bit7..6为10b，即PD 3.x，恰好与日志rev=2一致；后续rev=1帧为01b，即PD 2.0。不应据此怀疑首帧显示错误。
- RDO 0x33029CA7：对象位置3；Capability Mismatch=0；USB Communications Capable=1；No USB Suspend=1；Operating Current=1670mA；Maximum Operating Current=1670mA；未设置GiveBack。与PDO3 12V/1670mA相符，Power Data Object类型字段为固定供电。
- PDO1/2/3已核对为5V/3A、9V/2.22A、12V/1.67A；PDO4/5仅确认type=3（APDO），未在本轮展开其子类型与字段。
- 日志的 `len=OK`、`crc=OK` 是解析器对长度/CRC的状态判断；`raw`看起来含解码后的Header、数据对象和CRC，不包含完整线上前导、SOP和EOP/编码开销，需以项目解析代码最终确认。
- 后续先带读这一真实时间线的Header和PDO/RDO，不再另造无关模拟题；所有推断标明为推断。

## 2026-10-25：真实抓包与 raw 十六进制阅读

- 用户已基本理解真实抓包的协商时间线，但希望区分调测板监听记录与真实USB受电端、理解按位运算，并掌握raw的边界和解析方法。
- 关键教学结论：协议字段结构不因“调测板/真实受电端”改变；差异主要在角色、固件策略、PDO广告、RDO选择、时序/重试和是否真正承载负载。当前日志明确no TX，因此板端是被动监听，不是受电端协商参与者。
- raw解析模型（依据当前项目日志格式的合理解释，最终仍需代码确认）：去掉线上Preamble/SOP/EOP和BMC/4b5b编码后，raw包含解码后的Header、数据对象和CRC；Header低字节先传；普通数据消息总长度=2+4*NDO+4 CRC，控制消息=2+4 CRC。
- 已对真实样本定位：Source_Cap raw共26字节=Header A1 51 + 5×4字节PDO + 4字节CRC 71 75 15 70；请求raw共10字节=Header 42 10 + RDO A7 9C 02 33 + CRC FA FE E1 3A；GoodCRC共6字节=Header 41 00 + CRC BB 6C BB A8。
- 位运算教学：掩码取字段使用 `(header >> shift) & mask`；组合小端Header使用 `header = byte0 | (byte1 << 8)`；按位或用于拼接不重叠位，按位与用于保留目标位，右移用于把字段移到最低位。
- 本节以后先不继续扩展异常消息，下一节逐字节拆Source_Cap raw，再逐字段拆PDO1-3，随后拆PDO4-5 APDO；不要求用户脱离表格心算。


## 2026-11-04：PDO1 位字段带读

- 用户自行将0x0801912C转为二进制1000000000011001000100101100，数值正确；需左补4个0至32位再按位号定位。
- 已核对PD R3.2 V1.2 §6.4.1.3.1表6.6第126页、§6.4.1.3.3表6.8第127–128页、§6.4.1.3.13第135–136页。
- PDO1字段：Type=00；DRP=0、Suspend=0、Unconstrained=1、USBComm=0、DRD=0、Unchunked=0、EPR=0、Reserved=0、Peak=00；电压编码100×50mV=5V；最大电流编码300×10mA=3A。
- Peak=00不通告该字段内的额外过载档位，不能单凭此位断言完全没有任何峰值能力；Unconstrained不是无限功率；USBComm指USB数据线而非CC上的PD通信。
- 顺便纠正此前首帧Header Revision计算错误：0x51A1对应10b=3.x，与日志rev=2一致。其余rev=1帧为2.0。raw显示不足以确认Sink是手机还是调测板固件，之前“真实Sink”不应当作消费产品身份结论。
- 本轮用纯二进制切片和权重相加解释，不要求先掌握按位运算；待用户完成PDO2电压180、电流222的单位换算。

## 2026-11-04：Source_Capabilities 日志字段复习

- 用户正确写出PDO2的单位换算式180×50mV、222×10mA，对应9V/2.22A；仍需澄清USB数据线通信能力与CC上的PD通信。
- 按用户要求复习真实首帧，重点区分日志元数据(t8/CC1)、包类型(SOP)、Header解析结果(SRC/type/id/ndo/rev及消息名)、检查结果(len/crc)、原始解码字节(raw)与正文对象(PDO)。这些打印标签不是一组逐项排列在raw中的协议字段。
- raw首2字节A1 51→0x51A1：Extended0、NDO5、ID0、Source、Revision10b=3.x、DFP、Type1；确认此前Revision更正有效。
- NDO是对象数量，不是后面对象的名称；Source_Capabilities内对象是PDO，Request内对象是RDO。CRC不计入NDO。普通非扩展消息才适用当前2+4*NDO+4的raw长度解释。
- t8计时单位/时间戳采样点及raw确切定义需解析固件确认；len=OK、crc=OK在此只作为日志报告，不等于本轮独立验证CRC，更不证明实际电压/电流或完整无漏采。
- 未确认被测Sink物理身份，不能仅凭no TX断言是消费产品而非另一块调测板；仅说明记录程序声明监听、不发送。
- 本轮等待按元数据/Header/PDO分类作答，不推进新协议主题。

## 2026-11-05：长度不匹配检测练习

- 用户正确区分id/ndo来自Header、PDO2电流来自正文，并判断声明5对象但仅采集4对象且无CRC时不完整；提出编写指出错误的逻辑。
- 本轮提供独立C教学函数，不修改产品代码：限定raw=2字节Header+普通非扩展对象区+4字节CRC；先检查指针/最短Header，再读取NDO，计算expected=2+4*NDO+4并比较长度；扩展消息明确转交其他解析分支。
- 模拟输入NDO5、raw_len18时：预期26、实际18、缺8字节，报告截断/长度不足，而非笼统CRC错误。仅凭长度差不能唯一判断哪段字节丢失；“只有4PDO且无CRC”由题设提供，不是长度检查独立证明。
- 等长仅通过长度检查，仍需CRC与字段合法性验证；尚未运行/编译教学函数，无硬件操作。
