# Board 实现记录

> 2026-11-03：已实现运行时 UObject Board；编译及自动化运行结果见本文验证部分。
>
> 依据：[ADR-008：Board 对象与静态动态数据分离](../../Architecture/Decisions/ADR-008-Board对象与静态动态数据分离.md)。本次不包含 MoveRule、寻路、资源提交、动画和 ZOC 计算。

## 数据与接口

`UBoard` 位于 `Source/WarChess/Public/Board` 和 `Private/Board`，不依赖旧项目的 HexGridCore。两个私有 Transient Map 保存静态定义快照与动态占位，查询返回副本。

| 接口 | 行为 |
| --- | --- |
| Initialize(GridAsset) | 验证并复制定义，成功清空旧占位，失败保留旧棋盘 |
| Reset() | 清空定义和占位 |
| GetCellCount / HasCell | 检查地图中已定义格子 |
| GetCellDefinition | 返回定义副本；未知格子失败并清空输出 |
| GetCellState | 已有格子返回当前或默认空状态；未知格子失败 |
| GetOccupant | 返回单位 ID；空格和未知格均为 INDEX_NONE |
| GetAdjacentCells | 固定六方向，只检查格子存在，不检查高度或占位 |
| GetUnitCell | 从占位推导单位坐标；未知单位失败并清空输出 |
| PlaceUnit / MoveUnit / RemoveUnit | 统一维护占位；错误返回 false，不改变状态 |

单位 ID 为本场战斗内唯一非负整数。没有独立单位位置表，调用方不要再保存可独立修改的位置。

`PlaceUnit` / `MoveUnit` 只检查存在与占位一致性；上层移动入口结合 MoveRule、搜索结果与行动状态，在提交前校验地形、路径、ZOC、预算等玩法条件。MoveRule 负责合法转移和费用，不负责累计预算或行动资格。因此低层接口允许占据基础定义为不能站立的格子，以便以后飞行等规则复用。它们不是可以直接交给玩家输入的完整移动命令。

## 宿主接入示意

下面两个片段分别位于宿主 UObject 类的声明和初始化方法；`GridAsset` 由宿主配置或加载。本次没有自动在 GameMode 中创建 Board。

```cpp
UPROPERTY(Transient)
TObjectPtr<UBoard> Board;
```

```cpp
Board = NewObject<UBoard>(this);
if (!Board->Initialize(GridAsset))
{
    // 报告地图数据无效，不开始战斗。
    return;
}
```

未来编辑工具在资产上修改 `bStandable`。Board 已初始化后不会随资产编辑实时改变，需要重新初始化或新建对象。现有 `FHexCellData.Coordinate` 与资产键必须一致；初始化不推测如何修复错误数据。

## ZOC TODO

代码 TODO 位于 `Board.h`。先接入单位注册表和规则，再按需查询控制来源，允许双方和同方多个来源重叠，具体移动限制交给 MoveRule。需要缓存时再增加来源集合和失效机制。当前不提供假返回值查询；详细步骤见 ADR-008 第 4.5 节。

## 验证

- 构建目标：UE 5.4 `WarChessEditor Win64 Development`，包括 UHT 反射生成。
- 自动化：`WarChess.Board` 的初始化、邻接、占位与属性重定向四项；`WarChess.Grid.Generation` 覆盖生成资产到 Board 的接入回归。
- 实际运行结果（2026-11-03）：UE 5.4 Editor Development 编译及 UHT 通过；4 项 Board 测试与 1 项生成器集成回归全部通过，进程退出码为 0。
- 本机证据：[自动化报告](../../../../Saved/Automation/Board/index.json)、[运行日志](../../../../Saved/Logs/BoardTests.log)。Saved 目录不纳入版本控制，其他机器可用下面命令重现。
- 旧字段 `bIsPassable` 通过 `Config/DefaultEngine.ini` 中的 PropertyRedirects 映射为 `bStandable`。测试验证名称解析；当前没有旧地图 DataAsset 样本，未声称通过真实旧资产重载。

本机命令示例（先构建，再在无打开编辑器的情况下运行）：

```powershell
& 'D:/Program/UE_5.4/Engine/Build/BatchFiles/Build.bat' WarChessEditor Win64 Development '-Project=D:/Study/WarChess/WarChess/WarChess.uproject' -WaitMutex -NoHotReloadFromIDE
& 'D:/Program/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'D:/Study/WarChess/WarChess/WarChess.uproject' -unattended -nop4 -nosplash -nullrhi '-ExecCmds=Automation RunTests WarChess.Board+WarChess.Grid.Generation' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/Study/WarChess/WarChess/Saved/Automation/Board' '-abslog=D:/Study/WarChess/WarChess/Saved/Logs/BoardTests.log'
```

若 UE 5.4 的 Git 工作集解析因中文转义路径失败，仅为构建进程设置 `GIT_CONFIG_COUNT=1`、`GIT_CONFIG_KEY_0=core.quotepath`、`GIT_CONFIG_VALUE_0=false`，无需改动仓库或全局 Git 设置。

[返回实现记录](../README.md)
