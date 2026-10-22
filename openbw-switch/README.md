# 星际争霸 Switch 原生遭遇战 0.3.0

操作、安装、构建状态与限制见 [开发记录](../docs/development-0.3.0.zh-CN.md)。

```sh
./build.sh
KEYS_FILE=/private/path/prod.keys HACBREWPACK=/path/to/hacbrewpack ./package-nsp.sh
```

产物 `build/StarCraft-Switch-0.3.0-skirmish-test.nsp`，NSP 内嵌原生引擎，可通过 `ROMFS_DIR=/path/to/game-data` 内嵌 MPQ 和 maps；未内嵌时从 SD 加载。已使用实际资源推进 16,000 逻辑帧；尚未经过 Switch 实机验证。
