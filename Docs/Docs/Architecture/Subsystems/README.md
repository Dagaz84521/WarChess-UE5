# 子系统目录

这里按业务能力划分系统。每份文档说明该系统负责什么、维护什么状态、怎样与其他系统协作，以及哪些边界尚未确定。目录划分不等于已经确定 UE 的 Subsystem 类、组件或模块实现。

| 子系统 | 核心职责 | 主要状态或对象 |
| --- | --- | --- |
| [单位系统](单位系统.md) | 单位身份与状态归属，内部包含属性、Buff、技能三个模块。 | Unit、AttributeSet、BuffSet、SkillSet。 |
| [伤害结算系统](伤害结算系统.md) | 一次伤害的规则调用、数值结算与结果输出。 | DamageCmd、伤害上下文与结果。 |
| [战斗流程系统](战斗流程系统.md) | 战斗阶段、全局回合、单位行动资格与胜负推进。 | CombatManager、对战流程控制器。 |
| [棋盘与移动系统](棋盘与移动系统.md) | 解耦棋盘与移动规则，一次搜索生成最短路径树，范围显示与路径恢复共享结果。 | Board、MoveRule、MoveContext、MovementRangeResult、PathFinding、MoveResolver；架构已确定，接口与接入待验证。 |

单位组成、属性、Buff 与技能统一写在一份 [单位系统.md](单位系统.md) 中，通过章节说明各部分职责。

文档分工：

- 规则选择、Hook 时序、计算公式、上下文权限和取舍记录在 [ADR](../../README.md)，子系统通过链接引用。
- Model、Hook 派发和 Command 是跨系统机制，见 [公共机制](../Mechanisms/README.md)。
- UE 对象、Actor 与表现接入见 [集成设计](../Integration/README.md)。
- TypeScript 接口、具体示例、原型限制和验证记录见 [原型实现文档](../../Implementation/README.md)。
- 内容案例见 [Examples](../../Examples/01-实现一个火球术.md)。具体技能或 Buff 不单独算一个子系统。

后续增加子系统，应先说明独立职责与状态归属；不因出现一种新 Hook、公式或 Model 子类就增加系统条目。

[总体架构](../总体架构设计.md) · [文档入口](../../README.md)
