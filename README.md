# StarCraft-switch

目标：将《星际争霸》1.16.1 中文版移植到 Nintendo Switch / Atmosphère，保留原版界面与游戏行为，仅适配触屏和按键。

**当前状态：notaz/Pandora 路线可行性核查；尚未完成 Switch 移植，没有可交付的 NSP。**

按项目要求，OpenSnowstorm 在确认战役 AI 不完整后不再作为交付基础。

## 已核实

- 已取得并编译 notaz 的 `ia32rtools` 转换工具，锁定提交 `75b4a70dafd7faf3c27b963fa8e67462538f91d4`。
- 已检查 Pandora 官方发布包 1.16.1.8：游戏模块为 ARM ELF32，依赖 `libwine.so.1`、`libm.so.6`、`libc.so.6`。
- 发布包中的启动脚本通过 ARM Wine 启动游戏，不是无 Wine 的独立引擎。
- 发布包包含 201 个常规文件，未发现 `.c/.cpp/.h/.asm/.s` 游戏转换源文件。
- `ia32rtools` 是转换工具，不包含完整《星际争霸》转换工程；目前尚未取得可重建 Pandora 游戏模块的完整输入、修补与构建工程。

## 尚未解决

1. 取得可重建游戏模块的完整转换工程。
2. 解决 32 位指针与目标 ABI 的迁移；不能把旧版转换 C 文件直接作为 AArch64 C 源码编译。
3. 适配 Win32/Winelib、图形、音频、线程、文件系统等运行环境到 Switch Horizon；Linux Wine 库不能直接链接为 libnx 程序。
4. 验证中文补丁、原版界面、战役、遭遇战 AI、存档、技能与科技等功能。
5. 完成 Switch 触屏/按键适配、实机测试，再构建和发布 NSP。

这些是实际工程缺口，不能通过封装 NSP 解决。详见 [核查报告](docs/notaz-assessment.zh-CN.md)。

## 复核方法

```sh
git clone --recurse-submodules https://github.com/zqzhuhai-commits/StarCraft-switch.git
cd StarCraft-switch
make -C vendor/ia32rtools/tools -j2
python3 -m unittest discover -s tests -v
python3 -m pip install -r requirements-audit.txt
python3 tools/audit_pandora.py /path/to/starcraft_b8.pnd --output audit.json
```

转换工具构建成功不表示游戏构建成功。`audit_pandora.py` 仅检查文件，不执行发布包内程序，不生成 NSP。

仓库不包含原版游戏资源、Windows 可执行文件、Pandora 游戏模块或任何主机密钥。公开的转换工具子模块保留上游许可证。本仓库自有检查脚本和文档使用 MIT 许可证。

## 来源

- [notaz 的发布说明](https://pyra-handheld.com/boards/threads/starcraft.73844/)
- [作者对转换工具和 32 位限制的说明，#57](https://pyra-handheld.com/boards/threads/starcraft.73844/page-3)
- [Pandora 官方发布包页面](https://repo.openpandora.org/?app=package.starcraft&page=detail)
- [转换工具](https://github.com/notaz/ia32rtools)
- [Wine 的 Pandora 分支](https://github.com/notaz/wine/tree/pandora_hacks)
