# Command 执行机制

> 状态：BuffCmd 最小原型已实现；通用 Command 与 Action 机制仍为草案。

> DamageCmd.CauseDamage 按 ADR-005 接受已确认的 Direct / PercentMaxHealth 请求。直接伤害执行暴击、格挡、计算、施加，百分比跳过判定与通用乘数；两者共用扣血。伤害反应通过战斗级 FIFO 队列执行，当前契约见 [伤害结算实现](../../Implementation/Prototype/伤害结算实现.md)。通用 Command / Action 队列仍未定稿。

## 1. 系统概述

讨论战斗行为如何表达、校验和执行。Command 是计划使用的核心概念，Action 是否需要独立存在仍待讨论。

移动场景的架构决策见 [ADR-007](../Decisions/ADR-007-移动系统与寻路算法解耦.md)：MoveCommand 表达 UnitId 与 Destination，MoveResolver 取得 Context 中有效的最短路径树，无缓存时先重建并缓存，再调用 BuildPath 生成含路径、费用和版本依据的计划。范围查询以 GetMoveRange 消费同一 Result；已有 A* 作为独立能力保留。此处的 MoveAction 暂为移动计划数据名，不提前确定通用 Action 基类或执行队列；范围查询与指令解析均不直接修改权威状态。

本轮已实现 BuffCmd：同步提供 Apply、AddStack、RemoveStack、DecrementDuration、Remove，统一执行 Buff 状态变更及对应完成后 Hook。BuffSet 不自行驱动生命周期，BuffModel 在自身逻辑中提出命令请求。具体时序、合并、上限与失败反馈见 [Buff 设计](../Subsystems/单位系统.md#buff)。该实现不预设通用命令队列或 Action 层。

### 1.1 设计目标

使行为执行契约清晰、容易扩展，并能验证成功和失败的结果。

### 1.2 职责范围

TODO：界定行为请求、合法性校验、执行和结果反馈的职责。

### 1.3 非职责范围

TODO：划清执行机制与具体内容规则、回合控制及动画播放的边界。

## 2. 核心职责

TODO：确定谁接受请求、谁执行状态修改，以及失败如何向调用方反馈。

## 3. 核心对象与数据结构

TODO：讨论 Command 与 Action 是否分离，是否需要 Context，以及参数、结果和错误信息如何表达。

## 4. 对外接口与模块依赖

TODO：定义提交和查询契约，明确与 [Model](Model机制.md)、[Hook](Hook派发机制.md)、[回合管理](../Subsystems/战斗流程系统.md) 的协作边界。

## 5. 核心执行流程

TODO：确定校验与结算的时机、同步或分步执行方式，以及是否需要队列、嵌套行为和取消机制。

## 6. 扩展机制

TODO：明确新增行为需要提供的最小契约，以及组合行为的支持范围。

## 7. 约束与边界情况

TODO：讨论重复提交、目标失效、部分执行失败和递归触发；是否需要原子性或补偿处理尚未确定。

## 8. 验证与测试

TODO：设计合法行为、非法请求、执行中状态变化及失败后状态一致性的验证场景。

## 9. 待解决问题

- TODO：Command 与 Action 的区分是否带来足够收益？
- TODO：规则执行是否必须等待表现完成？参见 [UE 集成](../Integration/UE集成与表现调度.md)。
- TODO：MVP 对顺序稳定性和执行记录有什么要求？

[返回总体设计](../总体架构设计.md) · [返回文档入口](../../README.md)
