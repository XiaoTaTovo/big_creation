# 回放字段约定 v1

适用范围：当前 CC1/SOP 八帧回放、PC 事件 CSV 和状态结果 CSV。不是完整 PD 数据模型，不是核间/串口二进制协议。
行为的唯一实现是板端 `pd_replay_model.h` 中的 PdReplayEvent、PdDisplayState 和 pd_display_apply。
此文档定义其 PC 文件表示；不复制结构体、不改板端源码。

## 事件文件：一行是一次观察

表头必须精确为：

```csv
message,sender,id,t8,crc_ok,len_ok,ndo,data0,data1,data2,data3,data4
```

编码为 ASCII 或 UTF-8 无 BOM；LF/CRLF 均可。12 列都要填写；不支持引号、空行、空格或空字段。
最多 4096 行事件（不包括表头）；限制在主机内存和模型 16 位帧计数范围内。

| 字段 | 文件表示 | 含义及限制 |
|---|---|---|
| message | Source_Capabilities / GoodCRC / Request / Accept / PS_RDY | 当前支持的五种类型；枚举整数不是外部格式 |
| sender | SRC / SNK | 报文发送方供电角色，不是本机收发方向 |
| id | 十进制 0～7 | 原报文 Message ID，与文件行号不同 |
| t8 | 十进制 0～255 | 原日志原值，单位和计时语义未知，不参与调度 |
| crc_ok | 0/1 | 记录的 CRC 检查结果；PC 不重新计算 CRC |
| len_ok | 0/1 | 记录的长度检查结果 |
| ndo | 十进制 0～5 | 当前模型容量，不声称这是 PD 协议最大容量 |
| data0～4 | 无符号 32 位十六进制数 | 数据对象数值；不是原始线上的字节数组。未使用槽必须填 00000000 |

示例 Request：

```csv
Request,SNK,0,9,1,1,1,33029CA7,00000000,00000000,00000000,00000000
```

格式校验只证明数据能装入当前模型，不代表语义正确。语义处理由共享模型负责。
无完整 Header、原始帧字节、实测值、可信时间单位和丢帧统计；不能从 CSV 补造这些字段。
Source_Capabilities 中可保留 APDO 数值，但当前模型只推导固定 PDO、非 GiveBack 请求；其他情况不生成有效请求。

## 状态文件：每行是处理该事件后的结果

| 字段 | 含义 |
|---|---|
| sequence | 本轮 1 起始的事件序号，不等于 Message ID |
| message / sender / id | 最近处理事件的相应字段 |
| t8_raw | 对应事件 t8，特意保留 raw 命名防止误当毫秒 |
| crc_ok / len_ok | 当前事件的检查结果 |
| phase | WAITING / HAVE_CAPS / REQUESTED / ACCEPTED / READY |
| request_valid | 0/1；0 时以下四个请求字段留空 |
| pdo_index | 请求对象位置，从 1 开始 |
| voltage_mv | 选中固定 PDO 的电压，整数 mV，不是 ADC 实测 |
| operating_ma | RDO 工作电流请求，整数 mA |
| maximum_ma | 当前支持的非 GiveBack RDO 最大电流请求，整数 mA |
| frames | 本轮处理事件数，含 CRC/长度错误事件 |
| bad_frames | CRC 或长度标志失败的事件数，不是所有语义错误的总数 |

请求无效时数值留空，终端显示 --；不能把模型内部清零的数值解释为真实零电压。
当前模型遇到错误事件会保留先前有效协商状态并增加坏帧数，所以“最新帧坏”和“先前已协商”可以同时成立。
GoodCRC 不回退阶段。当前模型不完整处理重传、Reset、断连、角色交换、未知消息或数据过期。

## 固定元信息与来源

- schema_version=1：见 build/model-source.json；这里的版本是本工具 CSV 约定版本。
- CC1、SOP：本工具 v1 的固定场景约束，不得导入其他通道/包类型并宣称适用。
- 内置事件来自此前从 1022LISTEN.txt 整理的数据；本轮原始临时文件已不可访问，未重新审计原始日志。
- --bad-request 是单独标注的模拟注入；导出到 CSV 的 crc_ok=0 同样只是检查结果，不附带真实损坏原始帧。
- 速度是人工逐帧或立即遍历，不从 t8 推算真实间隔。

以后扩展真实数据接口时，需补齐未知/未检查状态、原始帧、时间基准、通道、包类型、错误分类与版本机制；不能把当前 C 结构体内存直接当作线上格式。
