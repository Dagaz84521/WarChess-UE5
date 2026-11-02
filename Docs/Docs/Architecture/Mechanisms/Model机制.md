# Model 机制

> 状态：AbstractModel、BuffModel、SkillModel 已有普通类原型；UE 类型映射待确定。

Model 表达内容自身的状态与行为。公共虚函数放在 AbstractModel，BuffModel 与 SkillModel 按需覆盖；Buff 生命周期回调与技能 OnPlay 留在各自子类。

## 机制边界

- Model 定义自身条件与效果，通过命令请求相关系统执行状态变更。
- Set 管理内容实例，不承担每种内容的玩法分支。
- Unit 关联三个 Set；内容身份与状态归属见 [单位系统](../Subsystems/单位系统.md)。
- 新内容优先覆盖已有公共时机；确有需求时再讨论新增 AbstractModel Hook。
- 公共战斗 Hook 的目标参与规则见 [ADR-006](../Decisions/ADR-006-Hook统一派发与Model自主判断.md)：Owner 不限定关注对象，Model 在具体 Hook 内判断是否产生效果；当前原型尚待迁移。

选择该方向的理由见 [ADR-001](../Decisions/ADR-001-选择Hook与Model架构.md)，Code-First 方向见 [ADR-002](../Decisions/ADR-002-采用Code-First开发方式.md)。一次伤害的正式阶段契约见 [ADR-005](../Decisions/ADR-005-伤害结算流程.md)，原型已接入的检查点和未决扩展约定见 [伤害结算实现](../../Implementation/Prototype/伤害结算实现.md)，不在这里复制清单。

构造、Owner、状态归属与既有待办见 [单位与 Model 实现记录](../../Implementation/Prototype/单位与Model实现.md)。物理所有权、释放顺序、跨对象引用和 UObject 映射仍待确定。

[返回公共机制](README.md)
