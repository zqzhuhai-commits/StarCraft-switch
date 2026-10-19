# notaz/Pandora 路线核查

核查日期：2026-10-20（北京时间）。目标为 Atmosphère/Horizon 下运行的《星际争霸》1.16.1 中文版。

## 结论

已按要求停止把 OpenSnowstorm 作为交付基础，改为核查 notaz/Pandora 路线。
notaz 的既有版本更接近直接保留原版逻辑，但目前没有取得完整转换工程，也没有完成 Switch 运行时移植，不能据此交付可玩的 NSP。

这是技术可行性核查结果，不是游戏完成报告。

## 已执行的检查

| 检查 | 结果 | 能证明什么 |
|---|---|---|
| 克隆 `notaz/ia32rtools` | 成功，提交 `75b4a70dafd7faf3c27b963fa8e67462538f91d4` | 已取得转换工具源代码 |
| `make -C vendor/ia32rtools/tools -j2` | 成功，有编译器警告 | 工具能在本次 Linux 主机编译；不等于游戏能构建 |
| 下载官方 StarCraft 1.16.1.8 PND | 成功，12,433,486 字节 | 取得历史发行包供检查 |
| 只读检查 SquashFS 包 | 201 个常规文件 | 未发现 `.c/.cpp/.h/.asm/.s` 源文件；不排除作者另有未取得的工程 |
| ELF 头部 | `libscr_sa_arm.exe.so` 为 ELF32、ARM、EABI5 soft-float | 它是 ARM32 共享模块，不是 Switch AArch64 程序 |
| ELF 动态依赖 | `libwine.so.1`、`libm.so.6`、`libc.so.6` | 需要相应 Wine/Linux 运行环境 |
| `run.sh` | 通过 `wine/bin/wine` 启动 launcher 与 game | Wine 是该发布包的实际依赖 |
| Switch 游戏构建/实机运行 | 未完成 | 没有可发布的 NSP |

发行包 SHA-256：`a0a48d08a151915ddddcb37d4924659c46b69de413453eb7c150d79084877ef3`。
可复核的机器输出见 [pandora-package-audit.json](pandora-package-audit.json)。不将包内游戏代码或资源上传本仓库。

## 原作者说明与源码核查

- 原作者发布帖说明转换步骤是 IDA 反汇编、转换成 C、编译并链接 ARM Winelib。
- 第 57 楼说明转换结果依赖 32 位架构，并非可直接用于 64 位的普通可移植 C 工程。
- `ia32rtools/winelib.mak` 实际使用 `winegcc`；ARM 参数包括 Cortex-A8、ARM 模式和 softfp，另一个分支使用 `-m32`。
- `c_auto.h` 中的 `u32` 与通过整数恢复指针的代码说明，需要逐项处理地址模型，不能简单替换编译器。
- 这些工具以调整后的反汇编、头文件和人工修补信息为输入；拥有原版 EXE 和 MPQ 并不等于已经拥有可重建的游戏工程。

### 当前阻碍

1. 缺少完整游戏转换输入、修补记录和构建工程。公开工具和已编译的发行包不能替代这些输入。
2. ARM32/32 位地址模型需要迁移或适配。不能将 ARM32 ELF 改扩展名后交给 Switch 加载。
3. Horizon/libnx 不提供上述 Linux 动态库与 Wine 系统环境。需开发或移植运行时，并处理窗口、输入、音频、线程、内存和文件系统。
4. 中文补丁涉及 Windows 二进制组件时，需要单独适配；不能仅复制字体就宣称完全兼容。
5. 原版行为、存档恢复、战役完成性、中文显示和 Switch 操作必须通过实际运行验收。

移除母巢之战内容不消除上述运行环境和地址模型问题。

## 后续验收门槛

- 可重建游戏模块，清楚记录原始文件版本与转换步骤。
- 在目标系统启动到原版菜单；正确显示中文与原版资源。
- 三族战役、任务触发、遭遇战 AI、科技技能和声音验证。
- 跨进程的保存/恢复验证；不能只验证同一进程内回滚。
- Switch 触屏、拖选、右键模拟、按键和连续视角移动验证。
- 完成实机稳定性与性能测试，再构建 NSP。生成安装容器本身不构成功能验收。

## 直接来源

1. [notaz 发布帖](https://pyra-handheld.com/boards/threads/starcraft.73844/)
2. [作者第 57 楼：工具、32 位限制、人工处理](https://pyra-handheld.com/boards/threads/starcraft.73844/page-3)
3. [官方发行页：版本与 Wine 依赖](https://repo.openpandora.org/?app=package.starcraft&page=detail)
4. [转换工具固定提交](https://github.com/notaz/ia32rtools/tree/75b4a70dafd7faf3c27b963fa8e67462538f91d4)
5. [Wine Pandora 分支](https://github.com/notaz/wine/tree/pandora_hacks)
