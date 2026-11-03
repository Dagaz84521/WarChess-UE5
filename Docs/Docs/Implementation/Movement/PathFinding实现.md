# PathFinding 与 MoveRangeResult 实现记录

> 当前实现：预算桶式 Dijkstra（ADR-010），支持非负整数费用。不包含 Resolver、缓存、ZOC 规则或真实移动执行。
>
> 决策：[寻路算法确定](../../Architecture/Decisions/ADR-010-寻路算法确定.md)。ADR-009 的 BFS 选型已被替代，ADR-007 的完整范围树和结果复用方向继续保留。

## 数据与接口

代码：[PathFinding.h](../../../../Source/WarChess/Public/PathFinding/PathFinding.h)、[PathFinding.cpp](../../../../Source/WarChess/Private/PathFinding/PathFinding.cpp)、[MoveRangeResult.h](../../../../Source/WarChess/Public/PathFinding/MoveRangeResult.h)、[结果操作](../../../../Source/WarChess/Private/PathFinding/MoveRangeResult.cpp)。

`FPathFinding` 是无状态的普通 C++ 工具类，提供静态 SearchRange(Start, Budget, GetNeighbors, OutResult)。它只消费邻接回调，不直接依赖 Board 或 MoveRule；共享边 FMoveNeighbor 定义在 [MoveTypes.h](../../../../Source/WarChess/Public/Movement/MoveTypes.h)。

| 字段 | 含义 |
| --- | --- |
| Start | 起点 |
| CostMap | 预算内所有可达格的最低累计行动力，起点为 0 |
| PreviousMap | 每个非起点可达格的一个最优前驱 |

GetMoveRange 按 q/r 排序返回全部格子（含起点）；GetCost 查询最低累计费用；BuildPath 沿树恢复完整路径，含两端，不搜索。不可达、前驱缺失或成环时 BuildPath 返回 false 并清空输出。结果生成后按只读快照使用，棋盘改变不会自动更新。

## 搜索契约

- 直接创建 Budget+1 个桶，按累计费用 0～Budget 扫描，桶内 FIFO。
- 预算支持 0～4096，由 MaxSupportedBudget 控制分配上界；超限或负数返回 InvalidBudget。它是实现资源保护，不是单位移动力的游戏设定。
- Cost 是 int32 非负整数；负数返回 InvalidEdgeCost。0、1、2 和其他非负整数均允许。
- 仅严格降低费用时更新前驱并入桶。旧桶条目不主动删除，出桶时比较最新距离跳过；等费用保留先前前驱。
- 预算边界继续查询零费用边；同桶动态追加的条目继续处理，因此零预算可得到完整零费用闭包。零费用环不会反复入桶。
- 先判断边费用是否超过剩余预算，再相加，避免溢出；不可负担的极大费用直接跳过。
- 图有限，搜索期间输入固定，邻接顺序稳定。调用方校验起点、单位与行动资格，搜索不改占位、不扣资源。
- 仅在同步调用期间使用 TFunctionRef，不保存回调。任意失败清空 OutResult，不暴露部分树。

普通费用搜索不需要路径历史；如果以后出现“仅第一次穿越加费”等规则，需要扩展节点状态。算法不会自动处理这些语义。

## Board 与 MoveRule 接入示例

以下变量由调用方提供：有效的 Board、MovingUnitId、Start、MoveBudget 和预览目标 Destination。上层应先通过 Board.GetUnitCell 等方式取得并验证起点。

```cpp
const FMoveRule Rule;
FMoveRangeResult Result;
const EMoveRangeSearchStatus Status = FPathFinding::SearchRange(
    Start,
    MoveBudget,
    [&](const FHexCoord& From)
    {
        return Rule.GetNeighbors(*Board, MovingUnitId, From);
    },
    Result
);

if (Status == EMoveRangeSearchStatus::Success)
{
    const TArray<FHexCoord> Range = Result.GetMoveRange();
    TArray<FHexCoord> Path;
    if (Result.BuildPath(Destination, Path))
    {
        const int32 TotalCost = Result.CostMap.FindChecked(Destination);
        // 将 Range、Path、TotalCost 交给上层预览或后续执行入口。
    }
}
```

上例是规划查询，不会修改 Board、移动单位或扣除行动力。多个目标的预览读取同一 Result 即可；不需要对每个目标重跑搜索。MoveRule 可以为 ZOC 等规则返回非负整数附加费用；当前默认规则仍为每步 1。

## 验证

[PathFindingTests.cpp](../../../../Source/WarChess/Private/Tests/PathFindingTests.cpp) 覆盖单位费用、稳定平局、预算、普通环、错误输出与前驱损坏保护；BucketRelaxation 验证带权绕路、严格改善和旧桶条目跳过；ZeroCostClosure 验证零费用环及预算边界零费用边。

[JSON 场景与可视化](../../../../Tests/Fixtures/Movement/README.md) 调用实际 C++ 搜索，输出算法名 Bucket Dijkstra。默认样例仍为 61 格、42 格可达、目标消耗 7；Python 只读取实际结果，校验逻辑仍针对默认费用为 1 的样例。

2026-11-03：UE 5.4 Editor Development 编译通过，15 项自动化测试全部通过（进程退出码 0）。包括新增的带权松弛、过期条目和零费用闭包，以及现有棋盘、规则、JSON 集成和堆式算法比较。

本机证据：[报告](../../../../Saved/Automation/BucketDijkstra/index.json)、[日志](../../../../Saved/Logs/BucketDijkstraTests.log)、[性能比较](../../../../Saved/Tests/Movement/pathfinding_benchmark.json)。Saved 不纳入版本控制。

性能比较仅将当前桶式搜索与测试中的旧堆搜索对照，使用真实 Board+MoveRule，在 61、817、3169 格的开放棋盘执行，比较范围、费用、前驱及耗时。每组使用 5 批均值的中位数，没有时间阈值断言。历史 BFS 的 11.16/258.40/1222.63 微秒测量不表示当前桶式实现的性能。

本轮本机 Development Editor 测量如下，包含结果分配与释放，不代表所有硬件或 Shipping 构建：

| 格子数 | 桶式 Dijkstra（微秒/次） | 测试用堆式基准（微秒/次） |
| --- | --- | --- |
| 61 | 14.16 | 16.11 |
| 817 | 295.78 | 316.61 |
| 3169 | 1269.66 | 1408.67 |

[返回实现记录](../README.md) · [MoveRule 实现](MoveRule实现.md) · [Board 实现](Board实现.md)
