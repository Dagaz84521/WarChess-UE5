#pragma once

#include "CoreMinimal.h"
#include "Grid/HexCoord.h"

/** 移动规则与搜索器共享的有向边；桶式 Dijkstra 接受非负整数 Cost。 */
struct WARCHESS_API FMoveNeighbor
{
    FHexCoord To;
    int32 Cost = 1;
};
