#pragma once

#include "CoreMinimal.h"
#include "BoardCellState.generated.h"

/** 本场战斗的格子状态；不重复保存地形、单位阵营或单位属性。 */
USTRUCT(BlueprintType)
struct WARCHESS_API FBoardCellState
{
    GENERATED_BODY()

    // 本场战斗内唯一且不复用的单位实例 ID；INDEX_NONE 表示无人占据。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board")
    int32 OccupantUnitId = INDEX_NONE;

    bool HasOccupant() const { return OccupantUnitId != INDEX_NONE; }
};
