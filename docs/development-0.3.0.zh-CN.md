# 0.3.0 原生遭遇战开发版

此版本提供标准地图上的人族 1 对 1 遭遇战入口，已使用用户资源做主机端逻辑测试，仍需 Switch 实机验证。不是完整原版中文版，也不支持原版战役 AI。

## 运行内容

- 从地图选择器挑选标准双人/多人 melee 地图；使用前两个有效玩家槽位。
- 双方固定人族，重新创建基地和四名工人，各有 50 矿。
- 读取地形、矿物、瓦斯和出生位置，不读取战役/UMS 触发器；因此不会调用上游未完成的战役 AI 脚本。
- 本方单位选择、框选、右键默认命令，原生 HUD 显示矿物、瓦斯、人口。
- 命令面板接入停止、保持、攻击、移动/采集、训练、建筑放置、附属建筑、研究和升级。按钮按当前选中单位和引擎前置条件生成；资源和人口仍由引擎校验。
- 触屏可选命令后点目标，建造显示位置预览。右键取消待放置命令。
- 基础原生人族 bot：采矿、训练 SCV、修建补给站和兵营、生产陆战队、分批进攻；消灭一方全部建筑后显示胜负并暂停。它不等同于原版电脑，也不支持战役脚本。
- SDL 核心 WAV 解码、格式转换和多通道声音混合；无需 SDL_mixer。真实游戏声音仍需验证。
- 手柄通过 libnx 读取实体 A/B/X/Y，避免 SDL 不同布局造成 A/B 互换。

## 操作

| 输入 | 操作 |
|---|---|
| 触屏点按/拖动 | 选择/框选；点击面板；点击目标与建筑位置 |
| A / B | 鼠标左键 / 右键；B 取消待放置命令 |
| 右摇杆 / 左摇杆 | 鼠标 / 镜头 |
| L | Shift，加选及默认命令排队 |
| 方向键左/右、R | 命令面板翻页 |
| Y / X | 切换高亮按钮 / 执行高亮命令 |
| + / - | 暂停 / 退出 |

选图界面使用上下、A 开始、+ 退出。触屏主要用于地图内操作。

## 数据与安装

NSP 内嵌原生 NSO，引擎不依赖外置 NRO。设置 `ROMFS_DIR` 打包时可内嵌游戏 MPQ 和地图；启动优先读取内嵌数据，没有内嵌数据时从 SD 读取：

```
switch/StarCraft/StarDat.mpq
switch/StarCraft/BrooDat.mpq
switch/StarCraft/Patch_rt.mpq
switch/StarCraft/maps/example.scx
```

本次单文件 NSP 使用用户提供的 MPQ 与 Bottleneck 标准双人地图。游戏资源和主机密钥不会提交到公开仓库。

日志位于 `switch/StarCraft/log.txt`。首次请使用普通双人遭遇战地图，不使用战役或 UMS 地图。

## 验证范围

- 交叉编译：devkitA64 r25.1 / GCC 13.2、libnx 4.12.0、SDL2 2.28.5、Mesa 20.1.0-5；导出 NRO、NSO。
- 主机回归：地图递归扫描/大小写/符号链接；手柄连接、拖动、触屏按下松开、断连释放；WAV 转换、播放、停止和销毁时释放通道。
- HUD 测试：面板点击/释放、攻击模式取消、键盘焦点、底部/右侧绘制边界。AddressSanitizer 检查；环境限制关闭 LeakSanitizer。上游侵入式链表用空指针计算成员偏移，会触发 UBSan，不能声称完整 UBSan 通过。
- 现有 7 项 Python 审计测试不验证游戏游玩。
- 真实资源集成测试：用户提供的 1.16.1 MPQ、`(2)Bottleneck.scm`，加载图像、地形、单位后推进 16,000 逻辑帧（相当于约 11 分钟游戏时间），无异常退出。电脑从基地与工人起步，最终有 12 名工人、20 名陆战队和 8 座建筑；这验证了采矿、生产和建造链路，不等同于 Switch 实机通过。
- 未验证：真实设备上的音频、NSP 安装、触屏与按键手感、帧率和完整胜负流程。没有实机通过证据。

## 尚未完成

原版主菜单、三族完整技能面板、中文字体和文本、原版 AI、战役、存读档、联网。当前引擎仍有未实现分支；生成 NSP 不能证明所有地图/单位/建筑行为正确。

## 可复现的真实资源测试

`tests/native_skirmish_smoke.cpp` 复用生产程序的 `load_skirmish`、命令面板、bot 和逻辑帧实现。为测试经济链路，前 8,000 帧在测试程序内也自动操作玩家一侧；正式程序不会自动操作玩家。它需要本地 MPQ 和地图，不会在 CI 或仓库中携带这些资源。

```sh
g++ -std=c++17 -O1 -DOPENBW_NO_SDL_IMAGE -DOPENBW_NO_SDL_MIXER \
  -Iopenbw-switch/openbw/ui -Iopenbw-switch/openbw $(sdl2-config --cflags) \
  tests/native_skirmish_smoke.cpp openbw-switch/openbw/ui/sdl2.cpp \
  $(sdl2-config --libs) -pthread -o /tmp/starcraft-smoke
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /tmp/starcraft-smoke /path/to/data /path/to/map.scm
```
