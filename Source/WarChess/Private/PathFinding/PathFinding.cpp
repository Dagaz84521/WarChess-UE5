#include "PathFinding/PathFinding.h"

EMoveRangeSearchStatus FPathFinding::FindReachable(
    const FHexCoord& Start,
    int32 Budget,
    FNeighborProvider GetNeighbors,
    FMoveRangeResult& OutResult
)
{
    const FHexCoord StartCell = Start;
    OutResult = FMoveRangeResult();
    if (Budget < 0 || Budget > MaxSupportedBudget)
    {
        return EMoveRangeSearchStatus::InvalidBudget;
    }

    FMoveRangeResult Result;
    Result.Start = StartCell;
    Result.CostMap.Add(StartCell, 0);
    TArray<TArray<FHexCoord>> Buckets;
    Buckets.SetNum(Budget + 1);
    Buckets[0].Add(StartCell);

    for (int32 Cost = 0; Cost <= Budget; ++Cost)
    {
        // 同桶可追加零费用邻居，所以必须读取动态 Num，而不是固定长度或 range-for。
        TArray<FHexCoord>& Bucket = Buckets[Cost];
        for (int32 Head = 0; Head < Bucket.Num(); ++Head)
        {
            const FHexCoord Current = Bucket[Head]; // 同桶 Add 可能重新分配，按值保存。
            if (Result.CostMap.FindChecked(Current) != Cost)
            {
                continue; // 更低费用已将节点加入其他桶，跳过过期条目。
            }
            for (const FMoveNeighbor& Edge : GetNeighbors(Current))
            {
                if (Edge.Cost < 0)
                {
                    return EMoveRangeSearchStatus::InvalidEdgeCost;
                }
                // 先比较剩余预算，避免相加溢出；超预算的边不入桶。
                if (Edge.Cost > Budget - Cost)
                {
                    continue;
                }
                const int32 NextCost = Cost + Edge.Cost;
                const int32* Known = Result.CostMap.Find(Edge.To);
                if (Known && NextCost >= *Known)
                {
                    continue; // 只严格改善；等费用不改前驱，零费用环不会反复入桶。
                }
                Result.CostMap.Add(Edge.To, NextCost);
                Result.PreviousMap.Add(Edge.To, Current);
                Buckets[NextCost].Add(Edge.To);
            }
        }
    }
    OutResult = MoveTemp(Result);
    return EMoveRangeSearchStatus::Success;
}
