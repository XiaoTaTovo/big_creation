# PC 离线回放：第一版

当前实现为 Windows 终端工具，没有窗口 GUI，不连接串口或板卡。
默认使用板端模型中从 1022LISTEN 整理的八条事件；也能导入本工具定义的结构化事件 CSV。
不支持直接解析原始 1022LISTEN.txt，不计算原始报文 CRC，不实现完整 PD 协议。

## 为什么先做它

同一份事件进入同一个 `pd_display_apply()`，PC 打印结果，板端 LVGL 显示结果。
这样可以先在电脑验证状态、异常和字段单位，再定位屏幕问题。PC 正确并不证明 SPI/DMA 正确。
当前使用 CONTEXT.md 已确认的八帧回放模型、固定 PDO 请求值和未知 t8 单位约定。

## 启动

双击本目录的 `start.cmd`。它先用本机 MinGW GCC 编译，再启动程序；退出后窗口保留输出。
需要本机已安装的 Python 和 MinGW，当前环境已验证可用。没有安装额外 Python 包。

也可在 PowerShell 执行：

```powershell
Set-Location 'D:\01_Workspace\12_Dev_Projects\GitHub_project\webcodex-大创项目\pc_replay'
python .\run.py
```

- 回车或 `n`：下一帧。
- `a`：立即播放剩余帧；没有按真实时间等待。
- `r`：清空状态，回到第零帧。
- `q`：退出。

这些命令均需回车提交。运行时间和报文 t8 没有对应关系。

## 先验收正常情况

启动应显示 WAITING、Request --、Frames 0。
按八次回车，阶段依次为：

```text
HAVE_CAPS → HAVE_CAPS → REQUESTED → REQUESTED
→ ACCEPTED → ACCEPTED → READY → READY
```

第三帧起：PDO 3，12000 mV，operating 1670 mA，maximum 1670 mA。
第八帧：Frames 8、CRC/length bad 0；最新消息 GoodCRC，阶段仍 READY。
第九次回车：提示结束，不增加帧数。
输入 r：所有累计状态清空，请求值重新显示 --。输入 a：结果应与第一轮相同。

这些都是请求/协商推导值。READY 表示此模型观察到了相应的协商消息，不证明实测电压。

## 再验收错误处理

```powershell
python .\run.py --bad-request --auto
```

只修改 PC 本次运行中的第一条 Request 的 crc_ok，不改源文件或默认事件。
标题明确显示 SIMULATED FAULT。最终应为 HAVE_CAPS、Request --、Frames 8、坏帧数 1。
不能因后面的 Accept、PS_RDY 就进入 READY。

然后重新运行正常回放：

```powershell
python .\run.py --auto
```

应恢复正常八帧结果。模拟错误不会写回板端模型。

## 文件导入与结果导出

在本目录执行（示例输出名需尚不存在）：

```powershell
python .\run.py --auto --save-events .\my-events.csv
python .\run.py --input .\my-events.csv
python .\run.py --input .\my-events.csv --export .\my-result.csv
```

`my-events.csv` 是输入事件；`my-result.csv` 是每帧处理后的状态表，两种文件格式不同，不能互相替代。
`--export` 自动跑完，不等待按键。写入已有文件会报错，使用新名字保留前次结果。

用文本编辑器打开事件 CSV，可以看到原始数据对象十六进制数。
例如将 Request 行的 crc_ok 从 1 改成 0，重新导入应得到坏帧结果。
不要修改默认模型做这个实验；编辑自己的导出副本。

CSV 字段及限制见 `fields-v1.md`。错误行、未知消息类型和超过容量的数据会拒绝导入，显示行号。

## 一键软件验证

```powershell
python .\run.py --test
```

预期所有测试 `ok`，结尾 `OK`，PowerShell 的 `$LASTEXITCODE` 为 0。
覆盖正常阶段/单位、交互重置、坏 CRC、文件往返、输入确实改变状态、已有文件保护、中文文件名、格式错误，以及原板端模型的异常测试。
测试用临时目录，结束自动清理；不会修改板端源码或操作硬件。

## 字段统一如何验证

`run.py` 直接引用板端 `V5F/User/pd_replay_model.h`，本目录不保留第二份模型。
每次编译生成 `build/model-source.json`（绝对路径与 SHA-256）和 `build/dependencies.txt`（编译器报告的依赖）。
查看两者确认使用的是你的实际板端源码。PC 的序号是 1～8，id 则来自报文，不可混淆。

板端正常回放最终应与 PC 对应：

| PC | LCD |
|---|---|
| READY | POWER READY |
| GoodCRC | GoodCRC |
| PDO 3 | PDO 3 / 5 |
| 12000 mV | REQ V 12.00 V |
| operating 1670 mA | REQ I 1.67 A |
| Frames 8、bad 0 | RX 8/8、BAD 0 |

PC 编译的是当前磁盘文件；板卡上的固件可能较旧。共用源码不能证明两边当前运行的构建相同，上板对照由用户手动完成。

## 文件职责与后续

- `replay_cli.c`：读取事件、交互、显示、导出；调用共享模型，不重复实现状态机。
- `run.py`：构建、记录模型来源、启动或测试。
- `test_replay.py`：自动验证真正编译出的程序。
- `fields-v1.md`：字段含义、单位、有效性和支持边界。
- `build/`：本机可执行文件、验证产物，不提交。

默认模型目录：`D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST\V5F\User`。
换电脑或移动工程，可使用 `python run.py --model-dir '新的User目录' --gcc '新的gcc.exe路径'`。

下一步如需原始 TXT 导入，先提供实际日志路径并核对格式，再做输入转换层。
CSV v1 不是 V3/V5 或 USB/串口最终传输格式；其冻结仍需与采集方确认。
