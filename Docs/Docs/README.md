# 战棋框架文档

## 项目简介

本项目计划使用 Unreal Engine 和 C++ 构建回合制战棋框架，主要参考《杀戮尖塔 2》的 Hook & Model 架构，强调游戏内容的行为内聚和规则扩展能力。采用 Code-First 开发方式，不采用原生 GAS 作为核心战斗架构。

当前处于架构设计初期，优先完成最小可运行 MVP，长期目标是形成模块化、易扩展、便于理解和测试的开源框架。多人网络同步、复杂可视化技能编辑器暂不纳入范围。

当前已有 [TypeScript 与 HTML 对战原型](../Prototype/README.md)、[2v2 双打](../Prototype/DuoREADME.md)及 [历史独立伤害 Hook 演示](../DamagePrototype/README.md)。整体 Prototype 已按 ADR-005 实现四阶段伤害、概率判定、百分比伤害、毒疗与反应队列；C++ 与 UE 集成、完整战斗流程及未决扩展仍需完善。项目根目录已有本地 Git 回退基线，使用说明见 Prototype README。

## 文档体系与完整导航

总体设计记录项目方向与系统协作；Rules 维护默认玩法规则，当前已有基础规则草案与填写模板；Subsystems 只按业务能力组织职责和边界。公共机制、引擎集成、ADR、实现记录与内容案例分别归类。

- 入口：[文档说明](README.md)
- 基础规则：[基础战斗规则（草案）](Rules/基础战斗规则.md) · [填写模板](Rules/基础战斗规则模板.md)
- 总体设计：[总体架构设计](Architecture/总体架构设计.md)
- 子系统设计：[目录与分工](Architecture/Subsystems/README.md)
  - [单位系统（含属性、Buff、技能模块）](Architecture/Subsystems/单位系统.md)
  - [伤害结算系统](Architecture/Subsystems/伤害结算系统.md)
  - [战斗流程系统](Architecture/Subsystems/战斗流程系统.md)
  - [棋盘与移动系统](Architecture/Subsystems/棋盘与移动系统.md)
- 公共机制：[机制目录](Architecture/Mechanisms/README.md)
  - [Model](Architecture/Mechanisms/Model机制.md)
  - [Hook 派发](Architecture/Mechanisms/Hook派发机制.md)
  - [Command 执行](Architecture/Mechanisms/Command执行机制.md)
- 引擎与表现：[集成目录](Architecture/Integration/README.md)
  - [UE 集成与表现调度](Architecture/Integration/UE集成与表现调度.md)
- 实现记录：[TypeScript 原型接口、行为与验证](Implementation/README.md)
- 架构决策：
  - [ADR 通用模板](Architecture/Decisions/ADR-Template.md)
  - [ADR-001 选择 Hook 与 Model 架构](Architecture/Decisions/ADR-001-选择Hook与Model架构.md)
  - [ADR-002 采用 Code-First 开发方式](Architecture/Decisions/ADR-002-采用Code-First开发方式.md)
  - [ADR-003 Buff 改变 Attribute 的方式](Architecture/Decisions/ADR-003-Buff改变Attribute的方式.md)
  - [ADR-004 战斗流程中的 Hook（草案）](Architecture/Decisions/ADR-004-战斗流程中的Hook.md)
  - [ADR-005 伤害结算流程（已确定）](Architecture/Decisions/ADR-005-伤害结算流程.md)
  - [ADR-006 Hook 统一派发与 Model 自主判断（已确定）](Architecture/Decisions/ADR-006-Hook统一派发与Model自主判断.md)
  - [ADR-007 移动系统与寻路算法解耦（已确定）](Architecture/Decisions/ADR-007-移动系统与寻路算法解耦.md)
  - [ADR-005 伤害结算流程（Codex 讨论版，归档草案）](Architecture/Archive/Decisions/ADR-005-伤害结算流程-Codex版.md)
- 使用案例：
  - [01 实现一个火球术](Examples/01-实现一个火球术.md)
  - [02 实现一个燃烧 Buff](Examples/02-实现一个燃烧Buff.md)
  - [03 扩展一种地形规则：泥地、飞行与占位](Examples/03-扩展一种地形规则.md)
- 移动重构配套：
  - [方格移动原型：地面 / 飞行与逐格事件](../Prototype/MovementREADME.md)
  - [方格移动实现记录](Implementation/Movement/方格移动原型.md)
  - [六边形网格与 A* 迁移清单（计划）](Implementation/Movement/六边形网格与AStar迁移清单.md)
  - [移动架构交互图](Architecture/Diagrams/movement-architecture.html)

## 推荐阅读顺序

1. 阅读本入口和总体设计，了解项目范围及尚未解决的问题。
   需要了解默认玩法时，阅读基础战斗规则草案；当前第 1～3 节已初步填写，标注待定的细节及后续章节继续完善。新建玩法规则可使用模板，TODO 不代表已确定规则。
2. 阅读 ADR-001 至 ADR-003、ADR-005 至 ADR-007，理解已经确定的架构方向、伤害结算规则、公共 Hook 参与范围与移动系统边界。ADR-004 仍为战斗流程中的 Hook 决策草案；归档的 ADR-005 Codex 讨论版仅供历史对照，正式决策以 Decisions 下的 ADR-005 为准。新增决策时复制 ADR 通用模板。
3. 先阅读《单位系统》一文中的单位组成、属性、Buff 与技能，再查看伤害结算与战斗流程的协作边界。
4. 公共行为与状态变更方式见 Mechanisms，原型接口与验证见 Implementation；引擎及既有移动接入分别见 Integration 与棋盘移动子系统。
5. 阅读火球术和燃烧案例，将内容需求反馈到相应子系统；移动重构结合 ADR-007、迁移清单及地形规则案例阅读，均以实际接入与验证证据确认实现进展。

## 文档状态说明

| 状态 | 含义 |
| --- | --- |
| 草案 | 尚在讨论，允许进行结构性修改。 |
| 已确定 | 设计方向已经确定，但不代表代码已经实现。 |
| 已实现 | 代码已经实现，且文档与当前实现保持一致。 |

子系统文档描述职责与当前边界，不以篇目存在代表完整实现。ADR-001 至 ADR-003、ADR-005 至 ADR-007 记录已确定决策，ADR-004 仍为草案。ADR-007 于 2026-10-27 确认移动系统与寻路算法解耦：一次预算搜索生成最短路径树，由 MoveContext 缓存，移动范围显示与路径恢复共享该结果；已有 A* 保留为独立单目标能力。具体接口、未决选型及迁移验证继续按文档待办推进。ADR-005「伤害结算流程」于 2026-10-23 确认采用 DSH 修订版，以 Decisions 下不带版本后缀的文件作为正式记录；Codex 讨论版保留在 Archive/Decisions 供历史对照。ADR-006 于 2026-10-24 确认公共 Hook 统一派发与 Model 自主判断，当前原型尚待迁移。架构决策已确定不表示接口已定稿或实现已完成；原型接口与验证信息见 Implementation，需依据正式 ADR 另行核对实现差异。

## 文档维护约定

- 使用简体中文 Markdown，保留 Model、Hook、Command、Action、Context 等术语；使用相对路径链接。
- Rules 维护默认玩法的现行规则，Subsystems 维护业务职责，Mechanisms 维护公共机制，Integration 维护引擎边界，Implementation 维护原型接口与验证，Examples 维护内容案例。
- 规则选择的理由、Hook 时序和契约决策记录在 ADR；规则草案尚未填写的部分仍以既有正式 ADR 为依据。将既有公式或时序迁入规则文档时同步修改原文引用，保留 ADR 的决策理由与历史，避免多处维护同一规范或把原型行为升级为确定结论。
- 将事实、已确定决策、待验证假设分开描述；假设必须标注“待验证”，未决问题使用 TODO。
- 修改高层方向时更新或新增 ADR，保留历史背景与决策依据，并同步受影响的索引和案例。
- 确定接口或流程前先明确职责与边界；实现后补充实际代码位置和验证依据，再调整状态。
- 更新文档时检查相对链接及状态一致性；不以计划、示例或预期测试替代实现证据。
