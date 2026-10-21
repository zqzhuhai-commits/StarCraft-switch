# 星际争霸 Switch — OpenBW 原生开发测试版 0.2.0

此版本用于验证移植链路，**不是完整原版游戏**，尚未实机验证。

## 本次修复

- 使用 stdio 路径枚举地图，修复原生 Fs API 不接受 `sdmc:` 前缀的问题。
- 支持递归目录及大小写混合的 `.scm/.scx/.scc`，启动时用方向键选图、A 开始、+ 退出。
- 保持映射后的 SDL 控制器连接，修复每帧反复打开、使用原始摇杆索引的问题。
- 触屏产生完整按下、拖动、松开事件；关闭 SDL 自带触屏鼠标转换，避免重复点击。
- 移动事件携带按键状态，断开手柄会释放鼠标和方向键，修复框选和卡键。
- 通过基础 `state_functions::next_frame()` 推进逻辑；不调用回放播放器的 next_frame，避免零长度回放报错。
- 右键向所选本地玩家单位发送默认命令（移动/采集/攻击，具体由引擎判断）；回放 UI 原有右键拖屏在游戏模式中禁用。
- 取消重复载入全部图像、重复提交画面，统一日志写入通道，异常写入日志后退出。
- 重新生成无水印 HOME 图标，构建时重新生成中文名称 NACP。

## 安装

安装 `StarCraft-Switch-0.2.0-test.nsp` 后，在 SD 卡准备：

```
switch/StarCraft/
├── StarDat.mpq
├── BrooDat.mpq
├── Patch_rt.mpq
└── maps/
    ├── example.scx
    └── campaign/example.scm
```

NSP **直接内嵌引擎 NSO**，无需外部 NRO。游戏资源需要自行放在上述目录。
另附 `StarCraft-switch.nro` 供 hbmenu 全内存模式调试；不建议相册 applet 模式。
日志：`switch/StarCraft/log.txt`。

## 操作

- 右摇杆：鼠标；左摇杆：镜头；触屏：选择/拖选。
- SDL A/B：左键/右键（实体位置取决于 SDL 控制器映射，待实机确认）。
- 左肩键：Shift，加选及排队命令。
- +：暂停；-：退出（选图界面使用 + 退出）。

## 已知限制

- 尚无完整建造/生产/技能面板、原版主菜单、种族/对手设置、中文文本或存档。
- 当前 OpenBW 高层 AI 脚本不完整：大部分 AI 脚本、部分地图触发器会抛异常并退出，日志中可见原因。不能宣称完整战役或完整遭遇战。
- `OPENBW_NO_SDL_MIXER` 构建不提供实际声音播放。
- 中文图形字库未接入，选图界面的中文文件名可能不显示。
- 1280×720 渲染、Switch 安装/启动、触屏和实体按键映射仍需实机验证。
- 不兼容的地图会在日志中记录异常；请从基础地图开始测试。

## 构建

需要 devkitA64、libnx、switch-sdl2、switch-mesa、switch-libdrm_nouveau、switch-tools、CMake。
本次构建组合：devkitA64 r25.1 / GCC 13.2、libnx 4.12.0、SDL2 2.28.5、Mesa 20.1.0-5、libdrm_nouveau 1.0.1-2、switch-tools 1.13.1。

```sh
./build.sh
# build/StarCraft-switch.nro、StarCraft-switch.nso、StarCraft-switch.nacp
KEYS_FILE=/private/path/prod.keys HACBREWPACK=/path/to/hacbrewpack ./package-nsp.sh
# build/StarCraft-Switch-0.2.0-test.nsp
```

密钥仅本地使用，不应提交或加入发布包。NSP title ID：`0100534352460000`。
打包工具：[hacBrewPack](https://github.com/rlaphoenix/hacBrewPack)（GPLv2）；NPDM 配置由其模板适配。

## 验证

- AArch64 原生交叉编译及 NRO/NSO 导出。
- 地图枚举回归：SCX、混合大小写、嵌套目录、目录符号链接、稳定排序和缺失目录。
- SDL 输入状态回归：映射控制器复用、手柄拖选、触屏点击/拖选/释放、断连释放。
- 既有 ELF/Pandora 审计测试。
- 打包后结构检查只能确认 NSP/NCA 容器及元数据，不能替代 Switch 实机验证。

## 来源

OpenBW：https://github.com/OpenBW/openbw（MIT）。修改保留在 `openbw/ui/`，通过可选标志启用游戏命令。
SDL2：zlib 许可证；Mesa、libdrm_nouveau、libnx：各自 MIT 等许可证。
图标用内置图像生成工具，根据用户参考图生成。提示词：方形、Kerrigan 居中、人类左侧、Protoss 右侧，绿色黑色氛围，STARCRAFT / BROOD WAR 金属标题，无水印。
