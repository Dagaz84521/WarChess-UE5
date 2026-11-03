#pragma once

#include "CoreMinimal.h"
#include "Movement/MoveTypes.h"

class UBoard;

/**
 * 可继承的普通 C++ 移动规则类，由调用方持有，不保存 Board 或搜索状态。
 * 默认提供 MVP 移动规则；派生类可覆盖查询来改变通行关系和费用。
 */
class WARCHESS_API FMoveRule
{
public:
    virtual ~FMoveRule() = default;

    /**
     * 读取棋盘并返回合法邻居，保持 Board 的几何邻接顺序。
     * 默认仅允许进入 bStandable 且没有其他单位占据的格子，每步 Cost = 1。
     * From 是搜索中的假想位置，不要求等于单位当前真实位置；自身占位不阻挡。
     * 负数单位 ID 或不存在的 From 返回空集合。单位有效性由上层入口校验。
     * 不修改占位，不检查预算，不比较格间高度，也不执行真实移动。
     * 派生规则可返回非负整数费用，由预算桶式 Dijkstra 计算最低累计消耗。
     */
    virtual TArray<FMoveNeighbor> GetNeighbors(
        const UBoard& Board,
        int32 MovingUnitId,
        const FHexCoord& From
    ) const;

    // TODO(ZOC)：接入单位注册表和控制来源查询后，由规则解释 ZOC 的移动影响。
    // 如果 ZOC 影响“到达后能否继续扩展”，这里可以根据 From 判断是否返回后续边。
    // 具体规则尚未确定；若依赖起点或路径历史，需要显式扩展只读输入或搜索节点，
    // 不能在本对象保存“查询到第几步”等可变状态，也不在此预设停止或费用规则。
};
