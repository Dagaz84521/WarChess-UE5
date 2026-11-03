#pragma once

#include "CoreMinimal.h"
#include "Grid/HexCoord.h"

/** 一次预算范围搜索生成的最短路径树；不持有 Board、规则或搜索器。 */
struct WARCHESS_API FMoveRangeResult
{
    FHexCoord Start;
    TMap<FHexCoord, int32> CostMap;
    TMap<FHexCoord, FHexCoord> PreviousMap;

    // 范围包含起点，按 Q、R 排序，不依赖 TMap 的迭代顺序。
    TArray<FHexCoord> GetMoveRange() const;

    // 查询最低累计行动力消耗；不可达时返回 false，输出置为 INDEX_NONE。
    bool GetCost(const FHexCoord& Cell, int32& OutCost) const;

    // 路径包含起点和终点；起点即目标时仅包含起点。
    // 不可达、前驱缺失或成环时失败并清空输出，不返回部分路径。
    bool BuildPath(const FHexCoord& Destination, TArray<FHexCoord>& OutPath) const;
};
