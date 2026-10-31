# 第二天：1022 报文回放页面

日期：2026-10-29。当前基线：CH32H417QEU-R0-1v1，V5F，SPI ST7789，170×320 竖屏。依据 CONTEXT 中已经跑通的显示工程继续开发，暂不改变屏幕方向或 V3 接口。

## 先运行，再读三处代码

实际工程：`D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST`。

1. 在 MRS 打开原来的 V5F 工程，刷新文件后构建。新页面默认启用。
2. 用户按已有流程手动烧录。此次不需要修改 V3 程序或接线。
3. 观察标题 `PD XiaoT`、`REPLAY: 1022`，等待约 16 秒看到第八帧。
4. 确认最后显示 `POWER READY`、`GoodCRC`、`PDO 3 / 5`、`REQ V 12.00 V`、`REQ I 1.67 A`、`RX 8/8 BAD 0`。
5. 最后一帧保持约 6 秒，随后回到 WAITING，重新播放。拍摄最终画面并确认右侧没有文字裁切。

这里的时间是为了方便阅读而设置的播放速度。它不是抓包中报文的真实间隔。耗时的原有 SPI 刷屏可能让实际播放略慢。

## 每两秒会发生什么

| 播放时间（约） | 最近报文 | 发送方 | 协商阶段 | 请求值 |
|---|---|---|---|---|
| 0 秒 | 尚无报文 | — | WAITING | 未知 |
| 2 秒 | Source Caps | SRC | CAPS RECEIVED | 未知 |
| 4 秒 | GoodCRC | SNK | CAPS RECEIVED | 未知 |
| 6 秒 | Request | SNK | REQUESTED | PDO3、12.00 V、1.67 A |
| 8 秒 | GoodCRC | SRC | REQUESTED | 保留 |
| 10 秒 | Accept | SRC | ACCEPTED | 保留 |
| 12 秒 | GoodCRC | SNK | ACCEPTED | 保留 |
| 14 秒 | PS_RDY | SRC | POWER READY | 保留 |
| 16 秒 | GoodCRC | SNK | POWER READY | 保留 |
| 22 秒 | 清空本轮 | — | WAITING | 未知 |

`POWER READY` 表示观察到源端声明供电准备就绪，不能当作电压实测证明。`GoodCRC` 是最近报文，不是协商阶段，二者可以同时出现。

## 页面字段

- `REPLAY: 1022`：来自既有抓包的离线回放，没有实时采集。
- `CC1 / LISTEN`：原抓包监听 CC1，分析仪没有主动发送报文。
- `SRC / SNK`：该帧来自供电端 / 用电端，不是 MCU 串口的收发方向。
- `ID`：原报文的消息 ID；不是本轮报文序号。
- `t8`：原日志计数值，单位尚未确认，不标成毫秒。
- `PDO 3 / 5`：请求了源端所公布的五个能力对象中的第三个。
- `REQ V`：根据 Request 选择的固定 PDO 查得的请求电压。
- `REQ I`：RDO 中的工作电流请求。此抓包最大电流请求也为 1.67 A，已保存在状态中。
- `RX`：本轮回放已经处理的帧数。
- `BAD`：本轮 CRC 或长度检查失败的帧数。正常八帧均为零。
- `CRC`：最近一帧的日志 CRC 结果。回放不重新计算原始字节 CRC。
- `DROP --`：原日志没有丢帧统计，不能填写零。
- 底部横条：八帧的播放进度，不是电压或电流大小。

## 代码阅读顺序

```text
main.c：选择 ui_pd_replay_create()
    ↓ 只创建一次文字和进度条
LVGL 每约两秒调用 pd_replay_tick()
    ↓ 取出一条结构化事件
pd_display_apply()：更新状态和请求值
    ↓
pd_replay_render()：把状态填写到已有 label
    ↓
原 LVGL 刷新流程 → 原 LCD/SPI 驱动 → 屏幕
```

先读 `V5F/User/pd_replay_model.h` 中的 `pd_replay_events[]`，它就是按时间排列的八张“报文卡片”。再读 `pd_display_apply()`，观察 Request 如何从 PDO 和 RDO 得出电压、电流。最后读 `V5F/User/pd_replay_ui_impl.h` 的 `pd_replay_render()`，看整数如何变成屏幕文字。

model 不依赖 LVGL，UI 不解析原始日志字符串。当前两个头文件只由 main.c 引入一次，函数为 static；这样不需要手动修改 MRS 自动生成的源文件列表。正式接口冻结后可拆成独立 `.c/.h` 模块。该状态结构不是最终核间传输协议。

原时钟、绘图页和用户修改的标题均保留。main.c 的原始字节备份为同目录 `main.c.before-day2-20261029.bak`。

| 想运行的页面 | main.c 宏设置 |
|---|---|
| 新回放页（默认） | `UI_DEMO_PD_REPLAY 1` |
| 原基础绘图页 | `UI_DEMO_PD_REPLAY 0`、`UI_DEMO_BASIC_DRAW 1` |
| 原钟表页 | 两个宏均为 `0` |

## 单独观察异常

在 `pd_replay_ui_impl.h` 中将 `PD_REPLAY_BAD_CRC_DEMO` 默认值改成 `1`，重编译后由用户手动烧录。标题变为 `TEST: BAD CRC`，只在第三帧模拟 Request CRC 错误。这不是原日志的内容。

预期：第三帧显示 CRC BAD；BAD 变为 1；请求值保持未知；后来的 Accept、PS_RDY 不能使状态进入 POWER READY。后续好帧会让最近一帧 CRC 恢复 OK，但 BAD 仍为 1。测试结束把宏恢复为 0。

## 验证与限制

- 主机 C 测试通过：完整八帧、请求值、GoodCRC 不回退阶段、坏 CRC/长度、错误顺序/发送方、越界 PDO、非 Fixed PDO、重置。
- WCH GCC12 根据当前 MRS makefile 的活动源文件列表和编译参数，全新编译并链接 341 个源文件成功。没有使用旧 ELF 作为验证依据。
- 验证产物位于工程的 `build/day2-verify/`，没有替换 MRS 的 `V5F/obj` 输出。构建仍有保留旧演示函数未使用的警告。
- 链接占用：FLASH 129740/131072 B，余量 1332 B；RAM_CODE 129104/131072 B。继续增加功能前要整理代码/字体占用并复查链接容量，不能直接扩大链接脚本中的内存声明。
- 当前字体的代表性字符串 advance 宽度检查最大约 129 px，小于内容区 150 px；此为初步检查，仍需用户实屏验收。
- 原逐像素同步 SPI 刷屏未改造。本次回放没有增加长时间等待，LVGL 定时回调由主循环执行，但整条显示链路尚未达到非阻塞。
- 只覆盖这次固定 PDO 协商，不是完整 PD 状态机；没有实时输入、原始字符串解析、数据过期、按键切页、双缓冲或 DMA。
- 本轮未烧录、复位、控制硬件。上板效果待用户反馈。

复现源码验证（在文档项目根目录执行）：

```powershell
python tools/verify_pd_replay.py `
  --project 'D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST' `
  --wch-bin 'D:\software\Code\MounRiver_Studio3\resources\app\resources\win32\components\WCH\Toolchain\RISC-V Embedded GCC12\bin' `
  --host-gcc 'D:\software\Code\MinGW\mingw64\bin\gcc.exe' `
  --output 'D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST\build\day2-verify'
```

原始证据：用户提供的 `1022LISTEN.txt`；协议逐字段说明见同目录 `protocol.md`。Source_Capabilities 日志打印的 revision 与 raw header 存在待核对之处，所以页面暂不展示版本号。
