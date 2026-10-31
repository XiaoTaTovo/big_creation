# 2026-10-30 软件验证记录

环境：当前 Windows，Python，本机 MinGW GCC；板端模型路径和本轮构建哈希见 build/model-source.json。

执行：`python pc_replay/run.py --test`。
结果：9 个自动测试通过，退出码 0；C 源文件以 -Wall -Wextra -Werror 编译通过。

1. 正常八帧：逐帧 phase、frames、t8_raw、请求有效性、12000 mV/1670 mA、最终 GoodCRC。
2. 交互：第九次前进不越界、重置、重新播放、退出。
3. 模拟坏 Request CRC：最终 HAVE_CAPS、请求无效、bad_frames=1，后续正常运行不受污染。
4. 输入事件 CSV 导出再导入，结果保持一致。
5. 输入 CSV 确实参与计算：修改 Request crc_ok 后结果改变。
6. 已有事件文件拒绝覆盖；输入路径与结果输出路径相同时拒绝覆盖。
7. 中文文件名导出导入。
8. 多组格式错误拒绝导入：空文件/仅表头、未知类型、ID/NDO/对象溢出、空单元格、负数、未使用对象非零、超长行。
9. 编译运行现有 tests/test_pd_replay.c：包括错序、无效 PDO、非 Fixed PDO、坏长度、错误发送方、能力列表更新和重置。

另已实际生成本机验收示例：

- build/1022-events.csv：从共享模型导出的八条事件。
- build/normal.csv 与 normal.log：最终 READY、PDO3、12000 mV、1670 mA、8 帧、0 坏帧。
- build/bad-request.csv 与 bad-request.log：最终 HAVE_CAPS、请求 --、8 帧、1 坏帧。

编译器依赖输出确认引用外部固件 User/pd_replay_model.h，没有在 PC 目录复制模型。
所有 build 产物仅供本机查看，已忽略 Git。

限制：这是软件与终端交互验证；没有新上板验收，没有重新核对已经失效的原始 TXT，没有验证完整 PD 解析、真实采集、串口或 GUI。
