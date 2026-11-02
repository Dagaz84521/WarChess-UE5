# Hook 派发实现

> 更新日期：2026-10-23。整体 Prototype 已接入 [ADR-005](../../Architecture/Decisions/ADR-005-伤害结算流程.md) 的四阶段检查点；完整战斗时机与通用优先级仍依赖 ADR-004。

## 参与者与公共方法

Unit.GetModelsSnapshot 从 BuffSet 和 SkillSet 收集参与者，顺序为 Buff 加入顺序、技能加入顺序。调用前用 IsModelActive 检查当前归属；已移除或被同标识实例替换的旧对象跳过。冷却中的 Skill 不因冷却失去公共 Hook 参与资格。

公共虚方法定义于 [AbstractModel](../../../Prototype/Model/AbstractModel.ts)。伤害使用 BeforeCrit / AfterCrit、BeforeBlock / AfterBlock、BeforeCalc / AfterCalc、BeforeApply / AfterApply，以及致死后的 OnDeath；实际扣血另派发 AfterDamageTaken / AfterCauseDamage 和可归因 OnKill。Buff 专属 OnApply、OnStackChanged、OnRemove 保留在 BuffModel，技能效果仍为 SkillModel.OnPlay。

不建立通用注册表或事件总线。每个检查点调用当前有效 Model 的对应方法，新内容通过覆盖方法参与。公共 Context 与 DispatchModelHook 位于 [ModelHooks](../../../Prototype/Model/ModelHooks.ts)。

## 快照与调用顺序

- 全局 OnTurnEnd 在阶段开始时统一拍全部单位的 Model 快照，按 CombatManager 的单位顺序派发；跨单位新增 Model 也等到下一个阶段。
- Buff 命令完成通知在持有者的有效 Model 中派发。
- 伤害的每个检查点同时收集来源与目标快照，稳定按来源后目标调用；来源与目标相同时去重。当前检查点新增的对象下一检查点才参与。
- 每次调用前重新检查实例有效性，防止已移除对象继续执行。

检查点内稳定遍历是 MVP 的确定性约定，不代表已经决定通用优先级。数值因子的组合按 ADR-005 的数学规则执行，不能以遍历顺序定义加乘法。

## 伤害 Context 权限

BeforeCrit / BeforeBlock 只能修改本次未完成判定的参数；参数在检查点结束后冻结。已经确定的概率结果不能回写。AfterCrit / AfterBlock 可向之后的计算提交独立因子，或入队反应。

BeforeCalc 提供本次公式输入和 AddModifier；百分比伤害不接受通用乘数。AfterCalc 只可准备后续施加操作，不能重算公式。BeforeApply 可否定、替换或修正施加量。AfterApply 与 OnDeath 读取不可变结果，通过队列请求后续效果。

具体类型、检查点适用性、否定和替换、随机数及结果契约统一见 [伤害结算实现](伤害结算实现.md)，本记录不复制整套业务规则。

## 生命周期与事件

BuffCmd 完成 Apply、层数变化、移除后分别派发 AfterBuffApplied、AfterBuffStackChanged、AfterBuffRemoved。Buff 自行在 OnTurnEnd 请求效果及扣时间，派发器不统一决定每个 Buff 的持续效果。

伤害的 AfterDamageTaken / AfterCauseDamage 仅在实际扣血大于零时执行；AfterApply 可观察零伤害、免疫和替换。死亡先由零生命体现，OnDeath 在 AfterApply 之后、反应出队之前派发。OnKill 只处理有来源且允许归因的正生命归零；毒蚀关闭归因，双打乘胜追击仍由 OnKill 驱动。

伤害后续请求通过 FIFO 队列执行，不递归进入同一运行时。Hook 同步异常继续抛出，已提交状态不回滚；剩余反应队列清空，防止意外带入下一条命令。

## 验证与后续工作

[流水线测试](../../../Prototype/Combat/Tests/DamagePipeline.test.ts)覆盖检查点顺序、快照中增删、自伤去重、参数冻结、后阶段影响边界、FIFO 和防循环。既有 Buff 与 Skill 测试继续覆盖回合与命令通知中的集合归属。

回合开始、角色行动前后、休息与中断时的通用 Hook，以及跨 Model 优先级与多替换冲突规则仍需在 ADR-004 中确认。本实现不增加没有具体业务需求的扩展点。

[返回公共机制](../../Architecture/Mechanisms/README.md)
