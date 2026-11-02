> 文档分类：TypeScript 原型实现与既有讨论记录。由原子系统文档迁入；保留接口、边界和验证信息，不代表新增架构决策。子系统职责见 [子系统目录](../../Architecture/Subsystems/README.md)，规则选择见 [ADR 索引](../../README.md)。

# 战斗状态与 Model 系统设计

> 状态：草案

## 1. 系统概述

讨论 Unit 的状态归属，以及内容对象的数据和行为如何内聚。当前已明确 Unit 由 AttributeSet、BuffSet 和 SkillSet 三个主要部分组成；各部分的物理对象类型、构造方式及 UE 生命周期仍待设计。

本文维护 Unit 的组成与职责边界，属性计算、Buff 生命周期和技能行为分别由对应子系统文档维护。文档整体为草案，已确认的结构与未决实现问题分开记录。

### 1.1 设计目标

使状态归属清晰，内容行为容易理解和扩展，并便于独立验证。

当前只讨论不含棋盘、位置和寻路的最小回合制战斗结构，不为这些未来需求增加对象或接口。

### 1.2 职责范围

明确三个 Set 管理什么状态、BuffModel 与 SkillModel 承担什么玩法行为，以及 Modifier 如何关联到属性和来源效果。

### 1.3 非职责范围

本文不定义完整的技能执行链、通用注册表或事件系统，也不确定 UObject、Actor、组件或普通 C++ 对象等实现选型。

TODO：明确内容状态与战斗调度、UE 表现的协作边界，避免重复保存同一事实。

## 2. 核心职责

| 对象 | 已确定的职责 | 与相邻对象的边界 |
| --- | --- | --- |
| Unit | 由三个 Set 组成，作为本单位状态与内容的归属对象。 | 不将所有 Buff、技能的玩法分支集中写入 Unit。 |
| AttributeSet | 管理本单位当前拥有的 Attribute。 | 属性计算见属性子系统，不管理 Buff 实例或技能冷却。 |
| BuffSet | 持有、查找、遍历本单位 BuffModel 实例，涵盖增益、减益和被动效果。 | 生命周期与计数变更由 BuffCmd 统一执行；具体效果保留在 BuffModel。 |
| BuffCmd | 统一执行 Buff 的施加、增减层、扣时间和移除，触发已定义规则 Hook。 | 不替代 BuffModel 决定自身的触发条件。 |
| SkillSet | 管理本单位拥有的 SkillModel。 | 技能行为与冷却由各 SkillModel 自行负责。 |
| BuffModel | 以 Code-First 定义效果行为，可添加并清理自己的 Modifier，也可响应 Hook。 | 不按属性 Tag 批量删除其他效果创建的 Modifier。 |
| SkillModel | 定义技能玩法，主要提供 OnPlay，自行管理冷却。 | SkillSet 不接管每种技能的行为或冷却规则。 |

以上为逻辑职责，不代表已经确定类继承、成员指针类型或代码模块依赖。

## 3. 核心对象与数据结构

### 3.1 Unit 的逻辑组成

```text
Unit
├── AttributeSet
│   └── 多个 Attribute（各自以独立 GameplayTag 标识）
│       ├── BaseValue
│       ├── CurrentValue
│       └── Modifiers
├── BuffSet
│   └── 本单位当前拥有的 BuffModel 实例
└── SkillSet
    └── 本单位当前拥有的 SkillModel
```

原型的 PrototypeAttributeSet 包括 Health、MaxHealth、AttackPower、DefendPower。Unit 已连接 AttributeSet、BuffSet 与 SkillSet。BuffModel、SkillModel 均继承 AbstractModel，复用 Owner 与公共 Hook 契约。SkillModel.OnPlay 定义效果，具体变更调用对应 CMD；Play 校验并在成功后开始冷却，技能在全局回合末推进自身冷却。

### 3.2 运行时状态归属

Attribute 的值与 Modifier 集合由 AttributeSet 管理；BuffSet 保存实例，BuffModel 响应 Hook 并请求 BuffCmd，BuffCmd 执行生命周期与计数变更；SkillModel 的冷却状态由其自身管理。采用 Handle 追踪 Modifier 时，来源 BuffModel 需保留足以定位自己创建项的信息，细节见 [Buff 设计](../../Architecture/Subsystems/单位系统.md#buff)。

TODO：确定 Unit 与 Model 的身份、引用有效期、初始化和释放顺序，以及战斗整体状态由谁持有。

## 4. 对外接口与模块依赖

已明确的协作关系：BuffModel 可以向指定 Attribute 添加 Modifier，失效时移除自己创建的 Modifier；SkillSet 持有的 SkillModel 通过 OnPlay 表达具体技能行为。

具体契约分别见 [AttributeSet](../../Architecture/Subsystems/单位系统.md#属性)、[BuffSet 与 BuffModel](../../Architecture/Subsystems/单位系统.md#buff)、[SkillSet 与 SkillModel](../../Architecture/Subsystems/单位系统.md#技能)。Set 是职责划分，不要求为三者再建立一层通用管理抽象。

TODO：确定查询、修改和错误反馈的最小接口，以及与 [执行机制](../../Architecture/Mechanisms/Command执行机制.md) 的边界；UE 集成留待后续。

## 5. 核心执行流程

当前局部协作为：BuffCmd 驱动效果的生效与失效，BuffSet 管理实例集合，具体 BuffModel 定义其对属性或 Hook 的影响；AttributeSet 根据属性与 Modifier 表达最终数值；SkillModel 负责自身玩法和冷却。

这些关系不构成完整的战斗调用顺序。TODO：明确初始化、移除和退出时的生命周期，以及失效引用的处理方式。

## 6. 扩展机制

新增效果和技能时，将玩法行为分别写入 BuffModel 和 SkillModel，维持 Code-First 与内容内聚方向。Set 管理当前拥有的内容，不按每种内容的名称增加玩法分支。

TODO：明确最小扩展接口；暂不建立通用内容注册表或额外的可视化数据编辑体系。

## 7. 约束与边界情况

TODO：讨论单位死亡、跨单位引用与战斗退出。属性和 Buff 原型已验证实例状态隔离；Buff 合并、封顶、刷新与移除规则见 Buff 文档，Health 的 CurrentValue 语义见属性文档。

## 8. 验证与测试

以下为验证目标；Attribute、Buff、Skill 归属、OnPlay、冷却与公共 Hook 部分已由原型自动化测试覆盖，详见各子系统文档：

- 验证不同 Unit 的属性、Buff 实例和技能冷却不会意外共享可变状态。
- 验证移除一个 Buff 只清理它创建的 Modifier，不影响其他效果。
- 验证 SkillSet 管理技能归属，具体玩法和冷却仍由对应 SkillModel 负责。
- TODO：接口确定后补充非法访问、单位释放和跨对象引用的测试输入与期望结果。

## 9. 待解决问题

- 已实现 BuffModel 与 SkillModel 继承 AbstractModel；基类统一声明 Owner、OnTurnEnd、ADR-005 四阶段检查点、OnDeath、实际受伤/造成伤害及 OnKill 通知和三个 AfterBuff 通知。Buff 自身生命周期回调由 BuffModel 定义，技能 OnPlay 由 SkillModel 定义。
- TODO：逻辑归属如何映射到实际对象生命周期，如何处理对象移除后的引用？
- TODO：三个 Set 的初始化、查询和修改契约是什么？
- TODO：共享计算规则与单个内容对象的行为如何划界？
- TODO（后续）：是否需要快照、序列化或状态比较能力？当前不展开。

[返回总体设计](../../Architecture/总体架构设计.md) · [返回文档入口](../../README.md)
