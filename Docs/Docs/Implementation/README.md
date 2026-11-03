# 原型实现记录

这里维护 TypeScript 原型的接口、当前行为、示例、验证信息及实现边界。文档由原先混合在 Subsystems 中的内容迁入，保留既有讨论；未决事项不是已确定的正式规则。

| 记录 | 内容 |
| --- | --- |
| [单位与 Model](Prototype/单位与Model实现.md) | 三个 Set、归属、公共基类与原型状态。 |
| [属性与伤害](Prototype/属性与伤害实现.md) | Attribute 接口、Modifier、Health、CauseDamage 与已有数值约束。 |
| [伤害结算实现](Prototype/伤害结算实现.md) | 接入完整 12 Hook 后的判定输入、阶段权限、真实扣血与兼容边界。 |
| [Buff](Prototype/Buff实现.md) | 施加、叠层、持续时间、移除、清理和失败反馈。 |
| [技能](Prototype/技能实现.md) | SkillSet、Play / OnPlay、冷却与技能示例。 |
| [Hook 派发](Prototype/Hook派发实现.md) | 当前参与者快照、有效性检查与派发行为。 |
| [回合与对战](Prototype/回合与对战实现.md) | 既有回合约定、原型边界及尚未实现的完整流程。 |
| [方格移动原型](Movement/方格移动原型.md) | 单角色地面 / 飞行、预算最短路径树复用、逐格状态提交与离开 / 进入事件。 |
| [UE Board 实现](Movement/Board实现.md) | UObject 棋盘、静态定义快照、动态占位、生成器接入与 ZOC TODO。 |
| [UE MoveRule 实现](Movement/MoveRule实现.md) | 普通 C++ 基类、默认合法邻居与 Cost 查询，以及派生类覆盖。 |
| [UE PathFinding 实现](Movement/PathFinding实现.md) | 预算桶式 Dijkstra、MoveRangeResult、最低消耗与前驱、路径恢复。 |

打开与运行方式以各原型 README 为准：[整体原型](../../Prototype/README.md)、[双打](../../Prototype/DuoREADME.md)、[独立伤害 Hook 演示](../../DamagePrototype/README.md)。测试数量和实现状态应结合当前代码核对，不能作为尚未实现规则的验证证据。

移动模块运行方式见 [方格移动说明](../../Prototype/MovementREADME.md)。另有 [六边形网格与 A* 迁移清单](Movement/六边形网格与AStar迁移清单.md)，它是已有项目接入当前仓库的计划，尚未接入对应 C++ 源码，与本次方格 TypeScript 演示分别记录。

业务职责见 [子系统目录](../Architecture/Subsystems/README.md)，规则选择与取舍见 [ADR 索引](../README.md)。
