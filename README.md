# StarCraft-switch

目标：将《星际争霸》1.16.1 中文版移植到 Nintendo Switch / Atmosphère，保留原版界面与游戏行为，仅适配触屏和按键。

## 当前路线（2026-10-22）：OpenBW 原生开发测试版 0.2.0

已修复原入口的地图路径、触屏按下/松开、手柄拖选和未推进逻辑帧等问题，
加入递归地图选择及选中单位的右键默认命令。提供原生 NRO/NSO 构建与应用 NSP 打包脚本。
图标根据用户提供的母巢之战参考图重新生成，无水印。

**这仍不是完整的《星际争霸 1.16.1 中文版》。**
代码核查发现，当前 vendored OpenBW 的 UI 主要是回放查看器，完整建造/生产界面、
原版主菜单、中文文本、存档尚未接入；`bwgame.h` 的 AI 脚本处理仅支持共享视野等有限脚本，
多数脚本会报 `unknown ai script`。原先文档的“完整原版 AI”“可玩验证版”表述不成立。
当前交付用于验证原生启动、地图加载、输入与基础单位命令，未经过 Switch 实机验证。

构建与安装见 [`openbw-switch/README.md`](openbw-switch/README.md)。NSP 内嵌引擎程序，
不是 NRO forwarder；MPQ 和地图仍从 SD 卡读取，不包含在 NSP 中。

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
