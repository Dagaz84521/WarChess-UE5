#include "Board/Board.h"

#include "Grid/HexGridDataAsset.h"

bool UBoard::Initialize(const UHexGridDataAsset* GridAsset)
{
    if (!IsValid(GridAsset))
    {
        return false;
    }

    TMap<FHexCoord, FHexCellData> Definitions;
    Definitions.Reserve(GridAsset->GridData.Num());
    for (const TPair<FVector2D, FHexCellData>& Pair : GridAsset->GridData)
    {
        const FHexCellData& Definition = Pair.Value;
        // 兼容现有 FVector2D 资产键，但不截断、取整或猜测冲突坐标。
        if (Pair.Key != FVector2D(Definition.Coordinate.Q, Definition.Coordinate.R)
            || Definition.WorldLocation.ContainsNaN())
        {
            return false;
        }
        Definitions.Add(Definition.Coordinate, Definition);
    }

    CellDefinitions = MoveTemp(Definitions);
    CellStates.Empty();
    return true;
}

void UBoard::Reset()
{
    CellStates.Empty();
    CellDefinitions.Empty();
}

bool UBoard::HasCell(const FHexCoord& Cell) const
{
    return CellDefinitions.Contains(Cell);
}

bool UBoard::GetCellDefinition(const FHexCoord& Cell, FHexCellData& OutDefinition) const
{
    const FHexCellData* Definition = CellDefinitions.Find(Cell);
    OutDefinition = Definition ? *Definition : FHexCellData();
    return Definition != nullptr;
}

bool UBoard::GetCellState(const FHexCoord& Cell, FBoardCellState& OutState) const
{
    OutState = FBoardCellState();
    if (!HasCell(Cell))
    {
        return false;
    }
    if (const FBoardCellState* State = CellStates.Find(Cell))
    {
        OutState = *State;
    }
    return true;
}

int32 UBoard::GetOccupant(const FHexCoord& Cell) const
{
    const FBoardCellState* State = CellStates.Find(Cell);
    return State ? State->OccupantUnitId : INDEX_NONE;
}

TArray<FHexCoord> UBoard::GetAdjacentCells(const FHexCoord& Cell) const
{
    TArray<FHexCoord> Neighbors;
    if (!HasCell(Cell))
    {
        return Neighbors;
    }

    static const FHexCoord Directions[] = {
        {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}
    };
    Neighbors.Reserve(6);
    for (const FHexCoord& Direction : Directions)
    {
        // 使用宽整数，避免资产处于坐标边界时发生有符号溢出。
        const int64 Q = static_cast<int64>(Cell.Q) + Direction.Q;
        const int64 R = static_cast<int64>(Cell.R) + Direction.R;
        if (Q < MIN_int32 || Q > MAX_int32 || R < MIN_int32 || R > MAX_int32)
        {
            continue;
        }
        const FHexCoord Neighbor(static_cast<int32>(Q), static_cast<int32>(R));
        if (HasCell(Neighbor))
        {
            Neighbors.Add(Neighbor);
        }
    }
    return Neighbors;
}

bool UBoard::GetUnitCell(int32 UnitId, FHexCoord& OutCell) const
{
    OutCell = FHexCoord();
    if (UnitId < 0)
    {
        return false;
    }
    for (const TPair<FHexCoord, FBoardCellState>& Pair : CellStates)
    {
        if (Pair.Value.OccupantUnitId == UnitId)
        {
            OutCell = Pair.Key;
            return true;
        }
    }
    return false;
}

bool UBoard::PlaceUnit(int32 UnitId, const FHexCoord& Cell)
{
    FHexCoord ExistingCell;
    if (UnitId < 0 || !HasCell(Cell) || GetOccupant(Cell) != INDEX_NONE
        || GetUnitCell(UnitId, ExistingCell))
    {
        return false;
    }
    FBoardCellState State;
    State.OccupantUnitId = UnitId;
    CellStates.Add(Cell, State);
    return true;
}

bool UBoard::MoveUnit(int32 UnitId, const FHexCoord& Destination)
{
    FHexCoord Source;
    if (!HasCell(Destination) || GetOccupant(Destination) != INDEX_NONE
        || !GetUnitCell(UnitId, Source))
    {
        return false;
    }

    // 所有失败判断都在写入前完成。后续增加其他动态字段时，不要删除源格其他状态。
    CellStates.Remove(Source);
    FBoardCellState State;
    State.OccupantUnitId = UnitId;
    CellStates.Add(Destination, State);
    return true;
}

bool UBoard::RemoveUnit(int32 UnitId)
{
    FHexCoord Cell;
    if (!GetUnitCell(UnitId, Cell))
    {
        return false;
    }
    CellStates.Remove(Cell);
    return true;
}
