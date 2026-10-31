# 存储布局与整块刷屏：两个顺序实验

日期：2026-10-29。实际工程根目录：`D:\01_Workspace\12_Dev_Projects\AllCode\QinHen\ST77789_LVGL\ST7789_TEST`。

本轮采用手把手教学：实际源码未修改。候选文件、ELF、map 和测试放在工程 `build/memory-block-lesson/`。用户先完成存储实验并确认上板正常，再修改刷屏。候选编译结果不等于硬件验收。

## 实验一：将只读常量移入 DTCM，扩大 V5 FLASH 配额

原理依据：本地 CH32H417DS0.PDF V1.8，PDF 第 8 页的存储器映射、第 10–11 页的 1.4.2。当前 ITCM 128 KB，DTCM 256 KB；当前 V5 `.highcode` 同时收集代码和 `.rodata`，从 FLASH 搬入 ITCM。本实验让 `.rodata` 随 `.data` 搬到 DTCM，复用启动文件已经存在的 `_data_lma → _data_vma ... _edata` 复制循环。

1. 先编译当前版本，保存编译结果；备份 `V5F/Ld/Link_v5f.ld` 为 `Link_v5f.ld.before-memory.bak`。
2. 在 MEMORY 块仅将 V5 FLASH 的 LENGTH 从 `128K` 改为 `256K`，起点仍为 `0x00010000`。RAM_CODE、RAM、RAM_LOAD 都不改。
3. 在 `.highcode` 内删除以下两行：

```ld
*(.rodata)
*(.rodata*)
```

4. 在 `.data` 的 `. = ALIGN(4);` 后面插入上述两行，放在原来的 `*(.gnu.linkonce.r.*)` 前面。其余内容不变。
5. 重新构建，对比 ELF 大小和 map 中的符号地址。确认是“常量转移”，不是单纯扩大比例表的分母。
6. 用户手动烧录，观察文字、颜色、回放和循环。至少观察三个完整循环；此为基本回归，不是长期稳定性证明。

候选完整文件：`build/memory-block-lesson/Link_v5f.candidate.ld`，仅供对照。复制回工程前应核对是否还有自己的新修改。

### 本轮独立验证结果

当前源码保持 `PD_REPLAY_BAD_CRC_DEMO=1`，异常回放行为应保持不变。本轮从实际源码重新编译 341 个文件，不沿用上一轮正常模式 ELF。

| 区域 | 当前基线 | 仅存储调整后 | 两项调整后 |
|---|---:|---:|---:|
| FLASH 使用 | 129744 B | 129748 B | 129724 B |
| FLASH 配额 | 131072 B | 262144 B | 262144 B |
| ITCM 使用 | 129108 B | 111904 B | 111880 B |
| DTCM RAM 区使用 | 71972 B | 89180 B | 89180 B |

基线 size：text=129420，data=324，bss=71648。

仅存储调整后：text=112216，data=17532，bss=71648。

候选符号地址：`glyph_bitmap=0x200C19F8`，`pd_replay_events=0x200C4320`，均在 DTCM；`_data_vma=0x200C0300`，`_edata=0x200C477C`，说明它们被启动复制范围覆盖。ITCM 的 `_highcode_vma_end=0x200BB520` 未越过 `0x200C0000`。

这不是压缩：FLASH 负载基本不变，约 17 KB 常量换到了 DTCM。C 的 const 语义不变；放在可写 RAM 也不等于获得硬件只读保护。启动时会增加常量复制量，性能和实屏效果需用户验收。

扩大的 V5 FLASH 地址区间为 `[0x00010000, 0x00050000)`，没有侵入当前 V3 的 `[0,0x00010000)`，也未超过手册中较小的 480 KB 用户区范围。本结论限定当前两个链接脚本；后续集成 IAP、参数区或其他固件必须重新核对。

若显示异常，先恢复链接脚本备份并重新构建、由用户手动烧录；不要继续叠加刷屏改动。

## 实验二：每个 LVGL 刷新区域只设置一次窗口

前提：实验一上板正常；当前仍为 RGB565、PARTIAL 模式、170×320，`LV_DRAW_BUF_STRIDE_ALIGN=1`，所以本例像素逐行紧密排列。修改行对齐、颜色格式或 DIRECT 模式后需重新处理缓冲区步长，不能照搬这个循环。

1. 备份 `V5F/User/lv_port_disp.c` 为 `lv_port_disp.c.before-block.bak`。
2. 在文件底部找到完整的 `disp_flush()` 定义，替换为：

```c
static void disp_flush(lv_display_t *disp_drv,
                       const lv_area_t *area,
                       uint8_t *px_map)
{
    if(disp_flush_enabled) {
        LCD_SetWindows((u16)area->x1, (u16)area->y1,
                       (u16)area->x2, (u16)area->y2);

        const uint16_t *color_p = (const uint16_t *)px_map;

        for(int32_t y = area->y1; y <= area->y2; y++) {
            for(int32_t x = area->x1; x <= area->x2; x++) {
                Lcd_WriteData_16Bit(*color_p);
                color_p++;
            }
        }
    }

    lv_display_flush_ready(disp_drv);
}
```

3. 重新构建并由用户手动烧录，检查字体、颜色、屏幕边缘、数值变化及旧文字是否残留。至少观察三个完整回放循环。

说明：`LCD_SetWindows()` 已包含 0x2A、0x2B、0x2C 和 X 偏移 35，不重复添加偏移或 RAMWR。`Lcd_WriteData_16Bit()` 保留既有高字节先发的规则；不要改成直接按 px_map 原始字节发送，以免颠倒 RGB565 字节顺序。

手册依据：`docs/LCD资料/ST7789P3.pdf` 第 159、161、163 页（CASET、RASET、RAMWR）。窗口内连续写像素，控制器按 MADCTL 配置推进地址。原 `LCD_Clear()` 也已经采用设置一次窗口、发送多个像素的方式。

### 为什么少了很多传输

满宽十行是 1700 像素，窗口端点包含在区域内：

```text
N = (x2 - x1 + 1) × (y2 - y1 + 1)
```

当前 `LCD_SetWindows()` 每次发 11 个 SPI 字节，像素颜色每个发 2 字节。

- 原版：1700 × (11 + 2) = 22100 字节。
- 新版：11 + 1700 × 2 = 3411 字节。

这是这一块区域的 SPI 字节量计算，不是实测整页速度提高 6.48 倍。每次 flush 不一定有 1700 像素；屏幕全帧为 54400 像素。

本步骤仍然每像素切换 CS，SPI1_Write 仍轮询等待，flush_ready 仍在发送结束后调用。因此只解决重复窗口设置，不宣称 DMA、持续片选或非阻塞。

### 本轮验证与上板验收边界

- 候选函数通过 WCH 编译并与实验一布局链接。
- 主机替身测试覆盖 170×10、非零坐标 7×3、右下角 1×1、禁用刷新：检查一次窗口、准确像素顺序和数量、发送完才通知 ready。
- 主机替身没有访问真实 SPI，不能证明 LCD 实物地址推进或颜色正确。
- 若有逻辑分析仪，可在保持同一页面条件下观察连续像素数据之间不再重复出现 0x2A/0x2B/0x2C。没有仪器时，以源码/主机测试确认调用次数，以实屏检查功能；暂不宣称测得准确加速倍数。
- 若出现花屏、移位或颜色异常，只恢复本次 lv_port_disp.c 备份；保留已独立验收的存储改动。

编译产物与报告：工程 `build/memory-block-lesson/` 中的 `baseline/`、`step1.elf/map`、`step2.elf/map`、`verification.txt`、`test_block_flush.c/exe`。这些都不是自动烧录产物；用户在 MRS 修改后应构建自己的当前版本。
