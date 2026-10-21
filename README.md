# StarCraft-switch

目标：将《星际争霸》1.16.1 中文版移植到 Nintendo Switch / Atmosphère，保留原版界面与游戏行为，仅适配触屏和按键。

## 当前路线（2026-10-22）：OpenBW 原生移植（v1 可玩验证版）

采用 **OpenBW 精确逆向引擎**原生编译为 Switch homebrew（AArch64，无 Wine/兼容层），
逻辑逐帧对齐原版 1.16.1，包含**完整原版 AI**（战役/遭遇战电脑玩家）。

- 交叉编译链路已验证：devkitA64 + libnx + switch-sdl2 + mesa → `openbw-switch.nro`（HOMEBREW/NRO0）。
- 数据与地图从 SD 卡加载（`sdmc:/switch/StarCraft/`，需 StarDat/BrooDat/Patch_rt.mpq）。
- 手柄/触屏 → 鼠标输入层已完成，自动加载第一张地图开始对战。
- 详见 [`openbw-switch/`](openbw-switch/)（构建、部署、操作映射、待办清单）。

已知缺口（下一迭代）：文本渲染层（OpenBW 上游无文字层，需自建，含中文 CJK 字库）、
地图选择菜单、种族/对手设置、存档。

## 已核查并放弃的路线（历史）

### notaz/Pandora 路线（不可重建，已停止）

按项目要求，OpenSnowstorm 在确认战役 AI 不完整后不再作为交付基础；notaz/Pandora
路线核查结论如下（详见 [核查报告](docs/notaz-assessment.zh-CN.md)）：

- 已取得并编译 notaz 的 `ia32rtools` 转换工具，锁定提交 `75b4a70dafd7faf3c27b963fa8e67462538f91d4`。
- 已检查 Pandora 官方发布包 1.16.1.8：游戏模块为 ARM ELF32，依赖 `libwine.so.1`、`libm.so.6`、`libc.so.6`。
- 发布包中的启动脚本通过 ARM Wine 启动游戏，不是无 Wine 的独立引擎。
- 发布包包含 201 个常规文件，未发现 `.c/.cpp/.h/.asm/.s` 游戏转换源文件。
- `ia32rtools` 是转换工具，不包含完整《星际争霸》转换工程；目前尚未取得可重建 Pandora 游戏模块的完整输入、修补与构建工程。
- 未解决：32 位指针迁移、Win32/Winelib 运行环境适配 Horizon、中文补丁与实机验证。

这些是实际工程缺口，不能通过封装 NSP 解决。

## 复核方法

```sh
git clone --recurse-submodules https://github.com/zqzhuhai-commits/StarCraft-switch.git
cd StarCraft-switch
make -C vendor/ia32rtools/tools -j2
python3 -m unittest discover -s tests -v
python3 -m pip install -r requirements-audit.txt
python3 tools/audit_pandora.py /path/to/starcraft_b8.pnd --output audit.json
cd openbw-switch && ./build.sh   # 需要 /opt/devkitpro 工具链，产物 build/openbw-switch.nro
```

转换工具构建成功不表示游戏构建成功。`audit_pandora.py` 仅检查文件，不执行发布包内程序，不生成 NSP。
OpenBW 路线已产出可构建的 `.nro`（本环境交叉编译通过），待实机验证。

仓库不包含原版游戏资源、Windows 可执行文件、Pandora 游戏模块或任何主机密钥。公开的转换工具子模块保留上游许可证。本仓库自有检查脚本和文档使用 MIT 许可证；`openbw-switch/` 内 OpenBW 源码为 MIT。
- [Pandora 官方发布包页面](https://repo.openpandora.org/?app=package.starcraft&page=detail)
- [转换工具](https://github.com/notaz/ia32rtools)
- [Wine 的 Pandora 分支](https://github.com/notaz/wine/tree/pandora_hacks)
