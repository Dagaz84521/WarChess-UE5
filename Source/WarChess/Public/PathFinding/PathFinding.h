#pragma once

#include "CoreMinimal.h"
#include "Movement/MoveTypes.h"
#include "PathFinding/MoveRangeResult.h"
#include "Templates/Function.h"

enum class EMoveRangeSearchStatus : uint8
{
    Success,
    InvalidBudget,
    InvalidEdgeCost
};

/** 无状态的预算桶式 Dijkstra；只处理节点与非负整数费用边。 */
class WARCHESS_API FPathFinding
{
public:
    // 直接分配 Budget+1 个桶的资源保护，不表示单位的玩法移动力上限。
    static constexpr int32 MaxSupportedBudget = 4096;

    using FNeighborProvider = TFunctionRef<TArray<FMoveNeighbor>(const FHexCoord&)>;

    /**
     * 搜索 Budget 内所有可达节点，生成最低累计消耗与前驱树。
     * GetNeighbors 仅在调用期间同步使用，不能在搜索期间修改输入图。
     * 图必须有限，费用为非负整数；负费用返回 InvalidEdgeCost。
     * 按累计费用递增处理桶，严格改善时更新前驱并入桶，过期条目跳过。
     * 邻接及桶内 FIFO 顺序稳定，等费用保留已有前驱。
     * 包含预算边界，并继续处理零费用边；零预算可得到零费用可达闭包。
     * Budget 必须处于 [0, MaxSupportedBudget]，否则返回 InvalidBudget。
     * 调用方验证 Start 和单位有效性；搜索器将 Start 视为有效节点。
     * 任意失败均清空 OutResult；先比较剩余预算再相加，超预算边跳过而不溢出。
     */
    static EMoveRangeSearchStatus FindReachable(
        const FHexCoord& Start,
        int32 Budget,
        FNeighborProvider GetNeighbors,
        FMoveRangeResult& OutResult
    );
};
