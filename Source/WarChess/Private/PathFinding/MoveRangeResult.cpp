#include "PathFinding/MoveRangeResult.h"

#include "Algo/Reverse.h"

TArray<FHexCoord> FMoveRangeResult::GetMoveRange() const
{
    TArray<FHexCoord> Cells;
    CostMap.GenerateKeyArray(Cells);
    Cells.Sort([](const FHexCoord& A, const FHexCoord& B)
    {
        return A.Q < B.Q || (A.Q == B.Q && A.R < B.R);
    });
    return Cells;
}

bool FMoveRangeResult::GetCost(const FHexCoord& Cell, int32& OutCost) const
{
    const int32* Cost = CostMap.Find(Cell);
    OutCost = Cost ? *Cost : INDEX_NONE;
    return Cost != nullptr;
}

bool FMoveRangeResult::BuildPath(const FHexCoord& Destination, TArray<FHexCoord>& OutPath) const
{
    OutPath.Reset();
    const int32* StartCost = CostMap.Find(Start);
    if (!StartCost || *StartCost != 0 || PreviousMap.Contains(Start) || !CostMap.Contains(Destination))
    {
        return false;
    }

    TArray<FHexCoord> ReversedPath;
    TSet<FHexCoord> Visited;
    FHexCoord Current = Destination;
    while (true)
    {
        const int32* Cost = CostMap.Find(Current);
        if (!Cost || *Cost < 0 || Visited.Contains(Current))
        {
            return false;
        }
        Visited.Add(Current);
        ReversedPath.Add(Current);
        if (Current == Start)
        {
            break;
        }

        const FHexCoord* Previous = PreviousMap.Find(Current);
        const int32* PreviousCost = Previous ? CostMap.Find(*Previous) : nullptr;
        if (!PreviousCost || *PreviousCost > *Cost)
        {
            return false;
        }
        Current = *Previous;
    }

    Algo::Reverse(ReversedPath);
    OutPath = MoveTemp(ReversedPath);
    return true;
}
