# V3 → V5 共享队列传输与验收

日期：2026-11-06。板卡：CH32H417QEU-R0-1v1 / CH32H417QEU6。沿用 CONTEXT.md 已确认的 V3 数据侧、V5 显示侧以及 0x20178000 / 32 KB 共享区。本轮为人工数据传输验证，不是 PD 协议解析或真实采集验收。

## 改动与接口

固件根目录为 `D:/01_Workspace/12_Dev_Projects/AllCode/QinHen/ST77789_LVGL/ST7789_TEST`。

- `Common/Common/pd_transport.h`：字段检查、槽内独立 CRC32、共享 SPSC 队列 Push/Pop、V5 Attach、人工测试记录。
- `Common/Common/pd_shared.h`：布局升级为版本 2，80 B 控制/统计 +16×52 B 槽（48 B 记录+4 B CRC32），共912 B；仍在原32 KB NOLOAD区内。两核必须配套更新，800 B 的版本1不再适用。
- `Common/Common/pd_capture.h`：提交接口由声明阶段转为实现阶段，明确共享队列语义。
- V3 main 实现外部 `PD_Capture_Submit()`，成功 Push 后设置 IPC CH0 bit0。人工记录约每100 ms一条，V5 attach 前不生成。原计数函数已替换。
- V5 main 每轮最多 Pop 四条，复制完成后更新 read_seq 释放槽，然后做测试内容比对；诊断页最多5 Hz更新。每轮清通知后仍查队列，不依赖通知次数，不会因通知合并而遗漏队列里的数据。
- 两份头文件及两个 main 保留 `.before-pd-transport-20261106.bak`。没有修改链接分区、显示DMA或硬件引脚。

只有 V3 主循环单一生产者可提交，只有 V5 主循环单一消费者可取出；禁止 ISR、多生产者和运行时清空共享区。接收目标必须是 V5 本地变量。使用对齐32位读写位置，复制前后 fence 控制发布/释放顺序；volatile 本身不代替屏障。实际跨核并发还需硬件验收。

两核使用相同编译 ABI 的固定结构布局，静态断言约束大小与偏移；不是将任意局部变量地址传给另一核，也不是 PC 串口线上格式。当前不需要分片/重组。独立 CRC32 对48字节记录计算，不与采集器对线上PD CRC的检查混淆，也不是安全认证。

## 成功与错误语义

`PD_Capture_Submit()` 返回 OK：记录已复制并发布，可复用调用者缓冲区，尚未保证显示。FULL：新记录拒收，已入队记录保留，不等待。INVALID：接口字段不符合约定。NOT_READY：布局/接收端未就绪或传输故障锁定；布局不正确时不写共享统计。

`PD_Transport_Pop()` 返回 OK：完整记录已复制给调用者；EMPTY：没有数据；DROPPED：槽已释放，但损坏/非法记录不会交给调用者；NOT_READY/FAULT：停止正常交付。

- 非法指针、种类/枚举、标志位、长度>28、未使用字节非零、有效位与字段冲突：V3拒收并统计；V5再次检查。
- CRC FAIL/UNKNOWN、长度检查失败、截断或不认识的 Header 内容：作为采集证据保留，不假装是有效协商。不在传输层按 NDO 或消息类型拒绝原始坏报文。
- 槽 CRC32 不匹配：复制后丢弃并释放槽，增加 CRC；可继续处理后续记录。原采集缓冲已可复用，因此没有虚假的自动重传承诺。
- 写读位置差>16：锁定本核故障，另一核检测后停止；不自行归零或重置共享区。
- 队列有积压且2秒无消费进展：诊断页可显示 STALLED，不自动重启。此检测依赖V5主循环继续运行，不能替代看门狗或检测所有核挂死。
- 不支持运行时单核复位/热重连，版本升级或故障恢复需要用户配套双核冷启动。诊断计数为32位，可回绕。

## 默认屏幕与速度

IPC DEMO / SHM OK 为人工数据标识。TX/RX：成功提交/有效接收数；SEQ：最近采集序号；Q：近似瞬时占用；HI：保守的占用高水位；FULL：拒收次数；TINV/RINV：发送/接收字段错误；CRC：共享记录完整性错误；GAP：序号不连续事件次数（不是丢失条数）；DIFF：人工记录与预期字段不一致次数；PDERR：采集CRC失败、长度失败或截断记录数；RX/s：约1秒窗口有效接收速率；POLLms：V5接收任务两次进入的最大间隔。

同一行统计不是跨核原子快照，运行中TX/RX瞬时不等不一定错误。CRC未知不会算成通过，也不会计入PDERR。测试记录采用固定Header和序号生成的28字节负载，未声称真实PD语义，未伪造时间戳。

当前发送约10条/秒只是便于验收的测试负载，不是吞吐上限。取数与页面更新分离，但 lv_timer_handler 仍可能在缓冲复用时等待 DMA。容量应依据实测最大服务间隔、到达突发和高水位决定。若接收平均速度小于发送，扩大队列只能推迟溢出；后续应先优化接收调度和显示预算，再考虑IPC中断或扩容。

## 软件验证

`tools/test_pd_transport.c` 直接编译实际 pd_transport.h，无第二套队列实现。MinGW GCC `-std=c99 -O2 -Wall -Wextra -Werror` 通过。15组覆盖：未attach、空队列、复制所有权、16槽满与恢复、32位序号回绕、非法字段、坏采集CRC保留、Reset事件、共享槽损坏、接收侧非法字段、布局不兼容、两种索引故障、随机收发交错、CRC32独立已知值。随机交错20万次，交付89066条，内容和FIFO顺序一致。

这是单线程交错逻辑测试，不证明真实多核内存顺序或缓存行为。HOST_TEST 屏障只作编译器屏障，硬件构建使用 RISC-V fence。

软件构建使用 MRS3 WCH GCC12.2.0、现有makefile，两核编译链接成功；V5仅有既有演示函数/变量未使用警告。日志和HEX SHA256在固件 `build/pd-transport-20261106/`。未执行硬件下载/复位，命令行make未更新Merge.bin。最终下载前必须在MRS重新生成合并文件。

主机测试复现（PowerShell，路径按实际安装）：

```powershell
$r = 'D:/01_Workspace/12_Dev_Projects/AllCode/QinHen/ST77789_LVGL/ST7789_TEST'
& 'D:/software/Code/MinGW/mingw64/bin/gcc.exe' -std=c99 -O2 -Wall -Wextra -Werror -I "$r/Common/Common" tools/test_pd_transport.c -o "$r/build/pd-transport-20261106/test_pd_transport.exe"
& "$r/build/pd-transport-20261106/test_pd_transport.exe"
```

## 用户上板验收（每次只改变一项）

1. 正常：保留两个测试开关为0；MRS先构建V3再V5，确认最新Merge.bin并手动下载，完整断电重上电。观察60秒。SHM OK，TX/RX递增、RX/s约10，Q在0～16内，FULL/TINV/RINV/CRC/GAP/DIFF/PDERR均为0。RX可短暂落后TX；观察POLLms与HI。若错误持续增加或显示FAULT，停止本轮验收，保留照片。
2. 队列满：只将V5 main的 `PD_DEMO_HOLD_RX_MS` 从0改2500。重新配套构建下载冷启动。V5仍刷新页面，但起始2.5秒不取数据。应看到HI=16、FULL>0，之后RX继续增加、Q下降，CRC/DIFF为0；GAP可出现。恢复该宏为0。
3. 非法字段：保持暂停宏0，只将V3 `PD_DEMO_INVALID_EVERY_10` 改1。每第10条长度29被拒收，TINV递增，接收继续；后续收到记录时GAP增加，RINV/CRC/DIFF保持0。测试后恢复宏0。

以上硬件步骤由用户执行。上板未完成前不宣称16槽足够、最大流量达标或长时间稳定。将来接真实采集，替换人工生成入口并移除人工DIFF判断，再接入协议解析；不要把真实报文交给演示内容比对器后将DIFF解释为传输损坏。
