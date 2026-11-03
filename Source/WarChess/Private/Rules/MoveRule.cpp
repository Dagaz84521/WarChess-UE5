#include "Rules/MoveRule.h"

#include "Board/Board.h"

TArray<FMoveNeighbor> FMoveRule::GetNeighbors(
    const UBoard& Board,
    int32 MovingUnitId,
    const FHexCoord& From
) const
{
    TArray<FMoveNeighbor> Neighbors;
    if (MovingUnitId < 0 || !Board.HasCell(From))
    {
        return Neighbors;
    }

    const TArray<FHexCoord> AdjacentCells = Board.GetAdjacentCells(From);
    Neighbors.Reserve(AdjacentCells.Num());
    for (const FHexCoord& Cell : AdjacentCells)
    {
        FHexCellData Definition;
        if (!Board.GetCellDefinition(Cell, Definition) || !Definition.bStandable)
        {
            continue;
        }

        const int32 Occupant = Board.GetOccupant(Cell);
        if (Occupant != INDEX_NONE && Occupant != MovingUnitId)
        {
            continue;
        }

        Neighbors.Add({Cell, 1});
    }
    return Neighbors;
}
