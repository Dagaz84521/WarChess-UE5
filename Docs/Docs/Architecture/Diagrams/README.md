# 移动架构图

本图说明 [ADR-007](../Decisions/ADR-007-移动系统与寻路算法解耦.md) 的核心决策：一次预算搜索生成最短路径树，由 MoveContext 缓存；GetMoveRange 与 BuildPath 消费同一份结果。已有 A* 保留独立能力，不作为移动主流程无缓存时的回退。图不作为已有代码结构或实现完成度的证据。

- [交互 HTML](movement-architecture.html)：独立查看，支持主题切换和原生导出。
- [PNG](movement-architecture.png)：用于 ADR 和子系统 Markdown 内嵌。
- [SVG](movement-architecture.svg)：矢量版，由同一 HTML 原生导出。
- [JSON 图源](movement-architecture.json)：使用 Archify architecture schema 维护。
- [交付收据](movement-architecture.delivery.json)：图源 / HTML 的 SHA-256、字节数与验收状态。
- [浏览器检查](movement-architecture.visual-check.json)与 [截图对照](movement-architecture.visual-check.html)：绑定同一 HTML。
- [导出收据](movement-architecture.exports.json)：PNG / SVG 的 SHA-256 与原生导出记录。

当前验收：结构与布局检查 9/9，零错误、零警告；1440×900、1600×1000、1920×1080、2048×1320 的浏览器布局检查通过。已目视检查两个端点尺寸的浅色 / 深色截图，以及原生 PNG，未见文字遮挡、无关节点穿线或桌面溢出。

维护时先修改 JSON 图源，再重新校验、交付和导出；不要只手改 HTML 或 PNG，避免图源和文档图片失去一致性。图聚焦结果数据流，省略 MoveResolver 调度与 CombatState 输入来源；完整模块职责、规则、缓存与交接语义以 [子系统设计](../Subsystems/棋盘与移动系统.md)为准。
