# ADR-003：Buff改变Attribute的方式

> 状态：已确定
>
> 本记录确认核心架构方向，不表示具体接口已确定或代码已实现。

## 1. 背景

战棋游戏中存在一些Buff需要对Attribute进行改变，同时buff移除后Attribute应该复原。

## 2. 面临的问题

如何能够实现Buff通过扩展定义的Hook，来实现Attribute的变化。目前决定计算公式是：

一个Attribute有两个值BaseValue和CurrentValue。与GAS中的方法类似，目前阶段的计算公式是：$CurrentValue = (BaseValue + \sum A_i)*（1+\sum P_i）$

其中 \(A_i\) 为固定值修正，\(P_i\) 为百分比修正。

所以需要决策的问题是，如何管理Buff的Modifier？

## 3. 候选方案

1. Buff各自修改Hook（ModifyAttribute），然后每次需要CurrentValue的时候，调用各个Buff的ModifyAttribute，计算出CurrentValue。
2. Buff各自修改Hook（OnApply），然后每次Apply的时候，添加Modifier，并且持有对应的Handle。再每次移除的时候（OnRemove），根据持有的Handle移除Modifier

## 4. 最终决定

选择方案2。

## 5. 决策理由

每次计算都需要遍历所有的buff，可能会带来很多不必要的开销。选择2.

## 6. 代价与影响

## 7. 后续验证事项

后续已确认 Health 表示剩余生命，不接收 Modifier，因此 BaseValue 与 CurrentValue 相等，DamageCmd 直接写入扣血后的值。生命上限增益放在 MaxHealth，继续遵循本文的 Modifier 公式。该约束避免属性重算覆盖已受伤害，不改变其他属性的计算方式。

原型已验证 Health 拒绝 Modifier、MaxHealth 增益变更不重置受伤结果，以及 DamageCmd 过量伤害与受击通知。实现见 [属性与伤害](../Subsystems/单位系统.md#属性)。


后续设计见 [总体设计](../总体架构设计.md)、[Model](../Mechanisms/Model机制.md)、[Hook](../Mechanisms/Hook派发机制.md)。

[返回文档入口](../../README.md)
