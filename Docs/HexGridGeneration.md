# 六边形网格生成

生成工具直接位于 `WarChess` 游戏模块的 `Public/Grid` 和 `Private/Grid`，不依赖 `HexGridCore` 插件或 GAS。

本次保留：轴向坐标与空间转换、Box 区域裁剪、地形高度与坡度采样、站位空间检测、线框/坐标/贴花预览，以及导出地图 DataAsset。不包含寻路、可达范围、单位移动、回合或技能系统。

## 编辑器使用

1. 编译 `WarChessEditor`，在关卡中放置项目模块的 `HexGridGenerator` Actor。
2. 调整 Bounds 范围、位置和 Yaw；缩放影响范围，不拉伸六边形。地形采样沿世界 Z 轴，建议仅绕 Z 轴旋转生成区域。
3. 设置 `HexSize`（中心到顶点的距离）、地面检测通道和站位检测参数。地面需要阻挡 Trace Channel；站位阻挡物需要响应该检测通道。
4. 点击 `Preview Grid Wires` 查看平面坐标布局。此预览不采样地形，不代表可通行结果。
5. 将 `/Game/Grid/M_HexDecal` 赋给 `DecalMaterial`，点击 `Generate Runtime Grid` 生成地形数据和贴花；不指定材质也能生成数据。材质使用 `Color` 和 `EnableCenterCircle` 参数。
6. 创建 `HexGridDataAsset` 类型的数据资产，指定到 `TargetGridDataAsset`，点击 `Generate Grid` 导出采样结果，再保存资产。该操作不自动写入磁盘。
7. `Clear Grid` 清除当前生成器的格子、贴花和坐标标签，不清空已经导出的 DataAsset。

数据保留坐标、世界中心点与 `bIsPassable`。没有地面或采样不合格的候选格仍会被保存，但标记为不可通行。DataAsset 保留旧格式，以 `FVector2D(Q, R)` 为键。

`Preview Grid Wires` 沿用原来的持久调试线实现，刷新/清理预览会清除当前世界的持久调试线。它适合编辑器生成调试，不作为正式游戏画面。

新项目中的完整旧插件已移除，生成工具直接使用项目模块中的代码和 `/Game/Grid/M_HexDecal` 材质。没有迁移旧蓝图或地图资产，也没有增加旧插件类型的重定向，请使用项目模块的新类型创建资源。原始插件仍保留在旧项目 `HexagonalGridGame` 中。

## 验证

自动化测试 `WarChess.Grid.Generation` 覆盖平地采样、旋转后的格子定位、站位阻挡、数据导出、重复生成清理、无地面与零格子尺寸。
