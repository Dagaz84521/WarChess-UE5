# JSON 棋盘与移动结果可视化

流程：版本控制中的 JSON 棋盘 → UE 自动化测试 → 真实 C++ MoveRangeResult JSON → Python 图像。可视化辅助人工检查，数值断言继续负责判断正确性；不使用图片像素作为回归标准。

## 文件

| 文件 | 用途 |
| --- | --- |
| [hex_board.json](hex_board.json) | 61 格六边形棋盘、站立标记、3 个单位、移动查询与数值预期 |
| [PathFindingJsonTests.cpp](../../../Source/WarChess/Private/Tests/PathFindingJsonTests.cpp) | 加载数据并运行 UBoard、FMoveRule、FPathFinding，校验后导出 |
| [plot_move_range.py](../../../Scripts/plot_move_range.py) | 读取实际结果，以轴向坐标绘制网格和箭头，不重新寻路 |
| `Saved/Tests/Movement/move_range_result.json` | 自动生成的结果，包含此次输入的棋盘/单位快照 |
| `Saved/Tests/Movement/move_range.png` | 自动生成的左右对照图 |

输入纳入版本控制；输出放在忽略提交的 Saved 目录，每次运行重新生成。本次 JSON 读写放在自动化测试中，不给 Board 或搜索器增加正式序列化职责。模块增加私有 Json 依赖。

## 数据约定

输入 `schemaVersion = 1`，包含：

- `board.cells`：每格的整数 `q`、`r`，三维 `worldPosition` 数组，以及布尔 `bStandable`。本样例的 worldPosition 全部为 `[0,0,0]`，导入时对应 `FHexCellData.WorldLocation`。
- `board.units`：单位实例 `unitId` 与所在 `q`、`r`，和静态格子定义分开。
- `query`：`movingUnitId`、`moveBudget`、`destination`。起点从 Board 占位查询，不在输入中再保存一份。
- `expected`：可达数量、少量关键最低消耗，以及不可达格子。修改场景时应有意识地更新预期，不能将新结果自动覆盖为正确答案。

样例半径为 4，共 61 格。`q = 0` 是障碍带，仅 `(0,-2)` 留作缺口。移动单位 7 位于 `(-3,1)`，另有两个阻挡单位。预算 7，可达 42 格（含起点），目标 `(3,-1)` 的最低累计行动力为 7。

输出 `moveResult.reachable` 按 q/r 排序，每项包含 `q`、`r`、最低累计 `cost`、`previous` 坐标。起点 cost 为 0、previous 为 null。不可达格不出现在 reachable 中，但仍保留在 board 快照中。

`moveResult.selectedPath` 包含目标、总消耗以及实际 C++ `BuildPath` 恢复的坐标序列。输出的 `searchAlgorithm` 为 `Bucket Dijkstra`；结果不包含时间戳，不将等费用的某一条具体路径作为黄金文件断言。

## 运行

从 UE 项目目录 `D:/Study/WarChess/WarChess` 执行。新加测试后先构建；只调整 JSON 时，可以直接重跑已有二进制。

```powershell
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'core.quotepath'
$env:GIT_CONFIG_VALUE_0 = 'false'
& 'D:/Program/UE_5.4/Engine/Build/BatchFiles/Build.bat' WarChessEditor Win64 Development '-Project=D:/Study/WarChess/WarChess/WarChess.uproject' -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Unreal build failed' }
```

上述 Git 环境变量用于绕过本机 UE 5.4 工作集解析中文转义路径的问题，仅需在构建进程环境中设置，不需要更改 Git 配置。

```powershell
& 'D:/Program/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'D:/Study/WarChess/WarChess/WarChess.uproject' -unattended -nop4 -nosplash -nullrhi '-ExecCmds=Automation RunTests WarChess.PathFinding.JsonFixture' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=D:/Study/WarChess/WarChess/Saved/Automation/MovementJson' '-abslog=D:/Study/WarChess/WarChess/Saved/Logs/MovementJsonTests.log'
if ($LASTEXITCODE -ne 0) { throw 'Movement fixture test failed' }
python Scripts/plot_move_range.py
if ($LASTEXITCODE -ne 0) { throw 'Movement visualization failed' }
```

Python 需要 Pillow，本机已具备，无需新增安装。其他环境可使用：

```powershell
python -m pip install -r Scripts/requirements-movement.txt
```

脚本支持 `--result` 和 `--output` 指定路径，默认路径相对于脚本所在项目计算，不依赖当前工作目录。

## 图像含义与校验

左图显示整个输入棋盘：X 为不可站立格，S 为移动单位，U 为其他单位，格子均标有 q/r 坐标。

右图在同一棋盘上显示可达范围与最低累计消耗，灰色可站立格表示当前预算内不可达；细蓝色箭头从前驱指向子节点，表示整棵最短路径树。橙色边框和箭头高亮起点到选定目标 G 的路径。前驱树不是要求单位依次走遍所有箭头的一条路线。

绘图根据 `x = sqrt(3) * (q + r/2)`、`y = 1.5 * r` 布局，并按画布缩放；不读取 worldPosition 作为绘制位置，因此所有世界位置相同也不会重叠。

C++ 测试检查关键数值、每格路径合法性、路径费用、前驱数量、预算内邻接闭包/松弛条件，以及单位位置不变。失败不导出新结果，并在开始前移除旧 JSON，避免旧输出伪装成最新成功。

Python 在绘图前再次检查坐标唯一性、占位、前驱方向、成本一致性、范围完整性及选定路径。校验使用当前默认规则的每步费用 1，不运行第二套寻路。未来增加带权地形时，需要重新评估搜索器并扩展文件契约和校验逻辑。

本样例验证 JSON 到真实移动逻辑的集成及可视化；当前另有带权松弛、过期桶条目、零费用闭包、负费用拒绝及预算上界等单元测试。算法调整依据见 [ADR-010](../../../Docs/Docs/Architecture/Decisions/ADR-010-寻路算法确定.md)。

## 本次验证

2026-11-03：替换为预算桶式 Dijkstra 后，UE 5.4 Editor Development 编译通过，JSON 集成、移动回归与性能比较共 15 项全部通过（进程退出码 0）。默认样例仍为 42 个可达格、41 条前驱边，目标路径 7 步、消耗 7 AP。Python 根据实际结果重新绘制 PNG，标题读取导出的算法名。

本机产物：[结果 JSON](../../../Saved/Tests/Movement/move_range_result.json)、[网格和结果图](../../../Saved/Tests/Movement/move_range.png)、[本轮自动化报告](../../../Saved/Automation/BucketDijkstra/index.json)、[本轮运行日志](../../../Saved/Logs/BucketDijkstraTests.log)。这些文件不提交版本控制，可按上面的命令重新生成。
