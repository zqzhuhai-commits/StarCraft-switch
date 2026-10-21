# StarCraft 1.16.1（母巢之战）Switch 原生移植 — OpenBW 引擎

在 Nintendo Switch / Atmosphère 上原生运行《星际争霸 1.16.1 中文版》。
不依赖 Wine / 兼容层；游戏逻辑为 OpenBW 精确逆向引擎（逐帧对齐原版，
包含完整原版 AI：战役 / 遭遇战电脑玩家），渲染走 SDL2 软件管线。

## 构建环境（本机已验证）

- devkitA64 gcc 13.2.0（devkitPro r25.1，2024-02 构建，兼容 glibc 2.35）
- libnx 4.9.0、switch-sdl2 2.28.5、switch-mesa 20.1.0、switch-libdrm_nouveau 1.0.1
- switch-tools（elf2nro）
- 全部安装于 `/opt/devkitpro`

> devkitPro 官方源 apt.devkitpro.org 对数据中心 IP 返回 Cloudflare 403，
> 本环境工具链从第三方存档 wii.leseratte10.de/devkitPro 获取。

## 构建

```sh
./build.sh
# 产物: build/openbw-switch.nro
```

## 部署（SD 卡 · 单文件夹自包含）

1. 把 `StarCraft-switch.nro` 与游戏文件放进 SD 卡 **`sdmc:/switch/StarCraft/`**：

```
switch/StarCraft/
├── StarCraft-switch.nro     ← 主程序（hbmenu 内浏览到该目录启动）
├── StarDat.mpq              ← 必需
├── BrooDat.mpq              ← 必需
├── Patch_rt.mpq             ← 必需
└── maps/                    ← 遭遇战/战役地图（自动加载第一张 .scm/.scc）
```

2. hbmenu 启动后浏览到 `switch/StarCraft/`，运行 `StarCraft-switch.nro`，
   自动加载 `maps/` 下第一张地图开始对战。
3. 日志写入 `sdmc:/switch/StarCraft/log.txt`。

## 操作映射

| Switch | 作用 |
|---|---|
| 右摇杆 | 鼠标移动 |
| 触屏 | 鼠标（按住拖动 = 框选） |
| A | 左键（选择/命令） |
| B | 右键（移动/取消） |
| 左摇杆 | 镜头卷动（方向键） |
| 左摇杆按下 + 拖动 | 小地图移动（后续版本） |

## 当前状态（v1 · 可玩验证版）

已完成：
- [x] 交叉编译工具链与工程（CMake + toolchain + build.sh）
- [x] OpenBW 引擎 + SDL2 后端编译为 homebrew NRO
- [x] 数据/地图从 SD 卡加载（sdmc:/StarCraft）
- [x] 手柄/触屏 → 鼠标输入层（SDL 事件注入）
- [x] 地图自动加载与游戏循环（原版 AI 运行）

待办（下一迭代）：
- [ ] 文本渲染层（资源/人口数字、提示文字）
- [ ] 中文显示（任务简报、对话、单位名；需 CJK 字库渲染，OpenBW 上游无文字层，需自建）
- [ ] 地图选择菜单（多图、战役列表）
- [ ] 玩家种族/对手设置（当前按地图默认）
- [ ] 声音验证（SDL2 音频后端）
- [ ] 存档/读档
- [ ] HOME 菜单图标与 NSP forwarder

## 目录

```
openbw-switch/
├── openbw/            # OpenBW 源码（bwgame.h 引擎 + ui/ SDL2 后端），MIT
├── src/main.cpp       # 移植入口：数据加载、地图枚举、输入层、主循环
├── CMakeLists.txt     # 交叉编译配置
├── toolchain.cmake    # devkitA64 toolchain
└── build.sh           # 一键构建 -> .nro
```

## 来源与许可

- OpenBW: https://github.com/OpenBW/openbw（MIT）
- 游戏数据：用户本地正版《星际争霸 V1.16.1 完美硬盘版》，未打包进工程
