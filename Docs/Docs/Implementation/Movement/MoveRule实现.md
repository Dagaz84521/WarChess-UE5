# MoveRule 实现记录

> 2026-11-03：实现普通 C++ 基类 `FMoveRule` 及默认移动邻接规则。未接入 GameMode、搜索或真实移动执行；ZOC 保留 TODO。

## 对象与接口

代码：[MoveRule.h](../../../../Source/WarChess/Public/Rules/MoveRule.h)、[MoveRule.cpp](../../../../Source/WarChess/Private/Rules/MoveRule.cpp)。

`FMoveRule` 是可实例化、可继承的普通 C++ 类，具有虚析构函数和 `virtual const GetNeighbors`。当前直接提供默认规则，不额外拆出空抽象接口和默认子类。派生类可以覆盖合法转移及费用计算。

职责限定为移动的合法邻接和费用，以及后续 ZOC 对移动的影响。回合、技能、伤害和胜负等规则由各自模块负责。

当前搜索器按 [ADR-010：寻路算法确定](../../Architecture/Decisions/ADR-010-寻路算法确定.md) 使用预算桶式 Dijkstra，接受非负整数费用，包括 0、1、2。默认规则仍为每步 1，派生类可通过费用表达 ZOC 附加消耗；具体 ZOC 规则尚未实现。

```cpp
TArray<FMoveNeighbor> GetNeighbors(
    const UBoard& Board,
    int32 MovingUnitId,
    const FHexCoord& From
) const;
```

`FMoveNeighbor` 是普通值结构体，包含目标 `To` 与本步 `Cost`，定义于 [Movement/MoveTypes.h](../../../../Source/WarChess/Public/Movement/MoveTypes.h)，由规则与搜索器共享，不增加反射宏。已可通过邻接回调接入 [PathFinding 范围搜索](PathFinding实现.md)。

Board 按只读引用传入，规则不持有它，也不保存搜索过程中的状态。可以直接实例化规则，或由后续负责组织战斗的对象以 `TUniquePtr<FMoveRule>` 持有；本次没有增加管理类。

## 默认移动行为

1. 单位 ID 为负数或 From 格子不存在时，返回空集合。
2. 查询 Board 的六边形几何邻居。
3. 排除不能站立以及被其他单位占据的格子；自身占据不阻挡。
4. 返回剩余邻居，保留 Board 顺序，每步 Cost 固定为 1。格间高度差不影响合法性或费用。

`From` 是正在扩展的搜索节点，**不要求它等于单位的真实位置**。例如实际单位在 A，搜索扩展至 B 时，可以用 B 继续查询；期间不改变真实占位。否则搜索只能获得真实起点的一层邻居。

当前没有单位注册表，所以本接口只检查 ID 非负；单位是否存在、是否允许行动等由未来上层入口校验。它也不检查预算、扣资源、缓存前驱或调用 Board 移动接口。

调用示意（Board、MovingUnitId 和 CurrentCell 由调用方提供）：

```cpp
const FMoveRule Rule;
const TArray<FMoveNeighbor> Neighbors =
    Rule.GetNeighbors(*Board, MovingUnitId, CurrentCell);
```

## ZOC TODO

代码在 `MoveRule.h` 预留 TODO。接入单位信息和控制来源查询后，再确定 ZOC 是改变边、费用还是到达后的继续扩展资格。当前不预设具体玩法。

如果规则区分真实起点与沿途格，需要把起点等条件显式加入只读输入；如果依赖路径历史，需要扩展搜索节点状态，不能通过修改规则对象来记录搜索进度。控制来源与缓存的设计依据见 [ADR-008 第 4.5 节](../../Architecture/Decisions/ADR-008-Board对象与静态动态数据分离.md#45-zoc-todo-与实现思路)。

## 验证

测试：[MoveRuleTests.cpp](../../../../Source/WarChess/Private/Tests/MoveRuleTests.cpp)。

- `WarChess.MoveRule.Movement`：不能站立/其他单位阻挡、忽略高度差、固定费用与顺序、假想位置扩展、自身占位、非法输入、查询不修改状态，以及下一次查询读取最新占位。
- `WarChess.MoveRule.Override`：通过基类引用调用派生类，改变费用且保留默认合法邻居。
- 2026-11-03：UE 5.4 Editor Development 编译通过；2 项 MoveRule 测试与 5 项 Board/生成器回归全部通过，进程退出码为 0。
- 本机证据：[自动化报告](../../../../Saved/Automation/MoveRule/index.json)、[运行日志](../../../../Saved/Logs/MoveRuleTests.log)。Saved 目录不纳入版本控制。

[返回实现记录](../README.md) · [Board 实现](Board实现.md)
