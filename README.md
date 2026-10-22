# StarCraft-switch

目标：将《星际争霸》1.16.1 中文版移植到 Nintendo Switch / Atmosphère，保留原版界面与游戏行为，仅适配触屏和按键。

## 当前开发路线：OpenBW 原生遭遇战 0.3.0

正在接通标准地图的人族 1 对 1 遭遇战：资源/人口 HUD、生产建造与研究命令、触屏和实体按键、基础原生电脑对手、WAV 声音后端以及原生 NRO/NSO/NSP 构建。

**当前仍是待实机验证的开发版本，不能标记为已验证可玩或完整原版游戏。**
原版 AI、战役、中文文字、存档和完整技能界面尚未完成。NSP 不包含 MPQ 和地图。

具体操作、部署路径、验证范围与限制见 [0.3.0 开发记录](docs/development-0.3.0.zh-CN.md)。

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
OpenBW 路线可构建 NRO/NSO 并打包开发测试 NSP，待实机验证。

仓库不包含原版游戏资源、Windows 可执行文件、Pandora 游戏模块或任何主机密钥。公开的转换工具子模块保留上游许可证。本仓库自有检查脚本和文档使用 MIT 许可证；`openbw-switch/` 内 OpenBW 源码为 MIT。
- [Pandora 官方发布包页面](https://repo.openpandora.org/?app=package.starcraft&page=detail)
- [转换工具](https://github.com/notaz/ia32rtools)
- [Wine 的 Pandora 分支](https://github.com/notaz/wine/tree/pandora_hacks)
