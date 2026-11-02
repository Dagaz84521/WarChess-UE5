> 文档分类：TypeScript 原型实现与既有讨论记录。由原子系统文档迁入；保留接口、边界和验证信息，不代表新增架构决策。子系统职责见 [子系统目录](../../Architecture/Subsystems/README.md)，规则选择见 [ADR 索引](../../README.md)。

# Buff 与状态效果系统设计

> 状态：第一版 TypeScript 原型已实现；已接入技能释放、CauseDamage 前后 Hook 与公共派发，完整战斗流程仍待设计。
>
> 实现位于 [Prototype/Model/Buff](../../../Prototype/Model/Buff)，运行及示例见 [Prototype README](../../../Prototype/README.md)。

## 1. 职责划分

| 对象 | 职责 |
| --- | --- |
| BuffSet | 持有、按 BuffId 查找、遍历当前 Buff；不推进生命周期或自动计数。 |
| BuffModel | 继承 AbstractModel，定义自身效果、生命周期回调与 Hook 响应，决定何时请求命令。 |
| BuffCmd | 统一执行施加、增层、减层、扣减持续时间和移除，并派发相关规则 Hook。 |
| CombatManager | 提供全局回合结束阶段派发与结算后受击通知，不代替 Buff 判断持续条件。 |
| Unit | 持有者，连接 AttributeSet、BuffSet 与 SkillSet，统一收集有效 Model。 |

Buff 模块不定义移动、棋盘、伤害公式、死亡或技能执行流程。技能 OnPlay 可通过 BuffCmd 施加效果。不引入通用命令队列、事件总线或可取消规则框架。

## 2. BuffModel 状态

| 状态 | 语义 |
| --- | --- |
| Id | 受类型约束的字符串标识符，采用精确匹配。 |
| Owner | 持有者，由首次施加绑定；未绑定时访问会报错。 |
| Source | 首次施加者，可为空，仅作信息追踪，不参与合并。 |
| MaxStackCount | 最大层数，正安全整数，默认 1，对外只读。 |
| StackCount | 当前层数，未施加为 0，生效后至少为 1，移除后为 0。 |
| Duration | 此实例构造时配置的完整持续时间；正安全整数或 null。 |
| RemainingTurns | 剩余回合，null 表示不使用回合限制。重复施加时取本次传入实例的 Duration。 |
| State | Created、Applying、Active、Removing、Removed。 |

层数和时间统一存在 BuffModel 中，具体 Buff 可以不使用；无叠层效果将最大层数设为 1，无回合限制效果将持续时间设为 null。公开 getter 只读，玩法代码通过 BuffCmd 修改状态。

同一单位只按 BuffId 合并，不区分施加者；不同单位相互独立。原型要求同一 BuffId 对应同一个实现类与 MaxStackCount，冲突时抛错，避免同一个标识对应两套效果定义。

来源归因尚未确定。原型保留首次 Source；后续施加者通过本次 AfterBuffApplied 的上下文提供，不据此决定周期伤害归属。

## 3. 施加与叠层

BuffCmd.Apply(owner, freshBuff, options) 接受一个未施加的实例，可通过 options 指定 Source 和本次 StackCount，默认添加 1 层。返回实际持有的实例，调用方应使用返回值。

首次施加：

1. 校验初始层数和实例状态。
2. 绑定 Owner、Source，层数限制在 MaxStackCount 内。
3. 标记 Applying，加入 BuffSet。
4. 执行 OnApply。
5. 标记 Active，开始具备常规 Hook 参与资格。
6. 在持有者的当前有效 Model（Buff 与技能）中派发 AfterBuffApplied，IsRefresh=false。

已有同标识 Buff：

1. 匹配 Owner.BuffSet 中的 BuffId，不比较 Source。
2. RemainingTurns 重置为本次传入实例的完整 Duration。
3. 新层数取“当前层数加本次添加层数”与 MaxStackCount 的较小值。
4. 层数实际变化时调用 OnStackChanged，并派发 AfterBuffStackChanged。
5. 派发 AfterBuffApplied，IsRefresh=true。

即使满层也刷新持续时间。不重复执行 OnApply。传入的临时实例未被施加，保持 Created，返回值是已有实例。

AddStack 仅增层并封顶，RemoveStack 仅减层，它们本身不刷新时间。非零层数变化调用 OnStackChanged；减层至 0 直接进入完整移除流程，不再对 0 层调用 OnStackChanged。

## 4. 回合与持续时间

一个全局回合是双方阵营的一个来回。调用方在整轮结束后调用 CombatManager.EndTurn；当前管理器不自动判断单位是否完成行动。

Buff 自己实现 OnTurnEnd：先执行该 Buff 的回合结束效果，再请求 BuffCmd.DecrementDuration。基类不自动扣时间，管理器也不另扣一次。

- 当前回合中途施加的 Buff 参与当前回合末结算。
- 持续 1 回合的 Buff 在当前回合末执行效果、扣至 0、移除。
- 剩余 1 回合时仍能执行最后一次效果。
- DecrementDuration 对 null 持续时间不生效；有限时间扣至 0 立即走统一移除。
- 周期效果如何随 StackCount 变化，由具体 Buff 定义。

派发开始时对所有单位的 Buff 与技能统一拍快照，按单位输入顺序、Buff 加入顺序、技能加入顺序调用。每次调用前检查集合归属，Buff 还需为 Active。派发中被移除的实例跳过，新加入的实例等待下一次阶段派发；同标识替代实例也不接替旧快照项。这一阶段快照规则与“回合结束前施加的 Buff 本回合计数”区分。

## 5. 移除与效果清理

1. BuffCmd.Remove 检查实例仍为 Active 且由 Owner.BuffSet 持有。
2. 标记 Removing、层数归零，立即失去常规 Hook 参与资格。
3. 执行 OnRemove，仍能访问 Owner 和自己的 Modifier Handle。
4. 兜底清理本实例登记的 Modifier。
5. 从 BuffSet 删除，标记 Removed。
6. 向持有者剩余有效 Model（Buff 与技能）派发 AfterBuffRemoved。

重复移除、递归移除或对未施加实例操作返回 false，不重复清理。已施加或已移除的对象不能再次 Apply；同类效果重新施加需创建新实例。

BuffModel 的 protected AddModifier 保存 AttributeSet 与 Handle，ClearModifiers 只清理自己的登记项。BuffCmd 在移除、首次施加失败时兜底清理，不能按属性标识批量移除其他效果的 Modifier。直接绕过此辅助方法创建的效果需自行负责清理。

## 6. Hook 与失败反馈

公共接口包括 OnTurnEnd、伤害来源/目标前后 Hook，以及 AfterBuffApplied、AfterBuffStackChanged、AfterBuffRemoved。后三项由 BuffCmd 在完成对应变更后派发给目标单位当前有效的 Buff 与技能，暂不提供阻止、取消或回滚 Buff 命令的能力。

这些公共 Hook 声明在 AbstractModel 中，由 BuffModel 和 SkillModel 继承。公共上下文与通用派发放在 Model/ModelHooks.ts；Buff 自身的 OnApply、OnStackChanged、OnRemove 仍在 BuffModel。运行时通过 Unit 收集 Buff 与技能，命令完成后事件向持有者的全部有效 Model 派发。

CauseDamage 按 ADR-005 使用 Direct / PercentMaxHealth。毒蚀每层回合末造成当前最大生命的 1/32，不参与暴击、格挡或通用修正；御势在 BeforeCalc 为 Direct 提交 0.5 独立因子，只有该包实际扣血后才移除。毒疗在 BeforeApply 识别 Poison 标签，取消扣血并排队独立治疗。

四阶段每阶段各有 Before / After 检查点，Context 按阶段冻结。乘数通过 AddModifier 收集后统一相乘；免疫使用 Negate，不使用零因子。ActualDamage > 0 才通知 AfterDamageTaken / AfterCauseDamage，AfterApply 也可观察零输入和免疫。Buff 仍通过 BuffCmd 请求生命周期变更。

正式规则见 [ADR-005](../../Architecture/Decisions/ADR-005-伤害结算流程.md)，实际接口与暂定的首个替换决定、FIFO 队列、防循环和失败行为见 [伤害结算实现](伤害结算实现.md)。不再以旧的固定减伤、忽略标志方案作为已确定依据；护盾、分摊与通用优先级仍待设计。

参数错误与非法实例复用抛错；对不再有效实例的增减层、扣时间、移除返回 false。增层已满或无限时长扣减也返回 false。

异常策略：

- OnApply 抛错：撤销集合成员和已登记 Modifier，实例标记 Removed，异常继续抛出。
- OnStackChanged 抛错：移除实例并清理已登记 Modifier，避免层数和效果不一致。
- OnRemove 抛错：仍清理已登记 Modifier、从集合删除，再向调用方抛错。
- 规则 Hook 和阶段 Hook 抛错：直接向调用方抛出，已完成的操作不回滚，整个回合不自动重试。当前不提供跨实例事务或任意玩法副作用回滚。

## 7. 实例与验证

AttackPowerBuff 展示每层 +10 攻击力、最多 6 层、默认 3 回合；OnStackChanged 重建本实例的 Modifier，OnRemove 清理，OnTurnEnd 请求扣时间。

KillActionBuff 展示永久被动，Duration=null、MaxStackCount=1，响应 AbstractModel.OnKill 请求恢复行动。仅在双打“乘胜追击”独立场景给玄锋施加；中毒不派发击杀通知。流程在本次技能完成并消耗行动后兑现请求，一次技能最多恢复一次，冷却不刷新，敌方全灭则直接结束。规则与运行入口见 [双打说明](../../../Prototype/DuoREADME.md)。

原型中 23 项 Buff 自动化测试覆盖：

- 不同施加者合并、层数封顶、满层刷新、独立增减层与减至 0。
- 当前回合末到期、最后一次效果、无限持续时间。
- 攻击力修正随层数更新、移除后属性恢复且不影响其他 Modifier。
- 不同单位状态隔离、条件受击移除。
- 派发中的自身移除、移除其他 Buff、同标识替换和跨单位新增。
- 命令规则 Hook、参数错误、重复施加对象、异常路径清理与递归移除。

与属性模块、公共 Model Hook、技能、Damage 及两版对战集成测试合计 111 项测试通过，构建通过。双打另有两回合防御 -35% 的 DuoFractureBuff，复用 Modifier Handle；毒蚀可被引毒或回春诀通过 BuffCmd 提前移除。运行方式见 Prototype README。

## 8. 后续议题

- Source 的多来源归因及周期伤害、击杀归属。
- 完整护盾、概率、死亡与战斗退出时的统一清理。
- 完整行动阶段、技能与更多 Hook，规则优先级及取消能力。
- UE 对象生命周期映射、持久化与表现。

[返回总体设计](../../Architecture/总体架构设计.md) · [返回文档入口](../../README.md)
