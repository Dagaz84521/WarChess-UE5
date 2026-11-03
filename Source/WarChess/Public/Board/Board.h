#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Board/BoardCellState.h"
#include "Grid/HexCoord.h"
#include "Board.generated.h"

class UHexGridDataAsset;

/**
 * 一场战斗的运行时棋盘。由宿主通过 NewObject 创建并以 UPROPERTY 强引用持有。
 * 只维护地图事实与占位，不负责费用、通行资格、寻路、动画或行动资源。
 * 所有查询和写入在游戏线程使用；调用方在提交占位前完成移动规则与行动资格校验。
 */
UCLASS(BlueprintType)
class WARCHESS_API UBoard : public UObject
{
    GENERATED_BODY()

public:
    // 成功时复制定义并清空占位；空资产有效。失败不改变现有棋盘。
    UFUNCTION(BlueprintCallable, Category = "Board")
    bool Initialize(const UHexGridDataAsset* GridAsset);

    UFUNCTION(BlueprintCallable, Category = "Board")
    void Reset();

    UFUNCTION(BlueprintPure, Category = "Board")
    int32 GetCellCount() const { return CellDefinitions.Num(); }

    UFUNCTION(BlueprintPure, Category = "Board")
    bool HasCell(const FHexCoord& Cell) const;

    // 失败时 OutDefinition 恢复默认值。返回副本，不暴露可写定义。
    UFUNCTION(BlueprintPure, Category = "Board")
    bool GetCellDefinition(const FHexCoord& Cell, FHexCellData& OutDefinition) const;

    // 存在但没有状态记录的格子返回默认空状态；不存在返回 false。
    UFUNCTION(BlueprintPure, Category = "Board")
    bool GetCellState(const FHexCoord& Cell, FBoardCellState& OutState) const;

    // 不存在或无人占据均返回 INDEX_NONE；需要区分时先查 HasCell。
    UFUNCTION(BlueprintPure, Category = "Board")
    int32 GetOccupant(const FHexCoord& Cell) const;

    // 固定轴向顺序的几何邻居；包含不可站立、已占据和高度不同的格子。
    UFUNCTION(BlueprintPure, Category = "Board")
    TArray<FHexCoord> GetAdjacentCells(const FHexCoord& Cell) const;

    // 位置由 CellStates 中的占位推导，MVP 不维护第二份单位位置表。
    UFUNCTION(BlueprintPure, Category = "Board")
    bool GetUnitCell(int32 UnitId, FHexCoord& OutCell) const;

    // 结构约束：ID 非负、格子存在且为空、同一单位只能占据一个格子。
    // bStandable、邻接、ZOC 等玩法资格由 MoveRule 在调用前判断。
    UFUNCTION(BlueprintCallable, Category = "Board")
    bool PlaceUnit(int32 UnitId, const FHexCoord& Cell);

    // 单次提交终点占位，不执行逐格移动。不允许空起点或起终点相同。
    UFUNCTION(BlueprintCallable, Category = "Board")
    bool MoveUnit(int32 UnitId, const FHexCoord& Destination);

    UFUNCTION(BlueprintCallable, Category = "Board")
    bool RemoveUnit(int32 UnitId);

    // TODO(ZOC)：单位系统与 MoveRule 接入后增加按需查询：
    // 1. 取得影响目标格的单位 ID，结合单位注册表查询阵营和控制能力。
    // 2. MoveRule 决定控制范围/资格；查询返回全部来源，允许双方和多个来源重叠。
    // 3. MoveRule 根据查询结果解释移动限制，Board 不写死“进入即停止”等规则。
    // 先直接计算；确有性能需求再在 CellStates 缓存来源集合，并在移动、移除、
    // 阵营/控制能力变化后失效。不能只存一个阵营或 bool，也不提供假返回值接口。

private:
    UPROPERTY(Transient)
    TMap<FHexCoord, FHexCellData> CellDefinitions;

    // 稀疏保存：当前只为有占据者的格子创建记录。
    UPROPERTY(Transient)
    TMap<FHexCoord, FBoardCellState> CellStates;
};
