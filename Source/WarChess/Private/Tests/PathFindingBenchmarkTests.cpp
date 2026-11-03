#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Board/Board.h"
#include "Dom/JsonObject.h"
#include "Grid/HexGridDataAsset.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PathFinding/PathFinding.h"
#include "Rules/MoveRule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace MoveSearchComparison
{
    // Test-only snapshot of the old heap-based search, for valid unit-cost inputs.
    // Production exposes only bucket Dijkstra. This reference is not a weighted-search API.
    struct FEntry { FHexCoord Cell; int32 Cost; uint64 Order; };
    struct FLess
    {
        bool operator()(const FEntry& A, const FEntry& B) const
        {
            return A.Cost < B.Cost || (A.Cost == B.Cost && A.Order < B.Order);
        }
    };

    void Dijkstra(const FHexCoord& Start, int32 Budget, FPathFinding::FNeighborProvider Edges, FMoveRangeResult& Out)
    {
        Out = FMoveRangeResult();
        FMoveRangeResult Result;
        Result.Start = Start;
        Result.CostMap.Add(Start, 0);
        TArray<FEntry> Queue;
        const FLess Less;
        uint64 Order = 0;
        Queue.HeapPush({Start, 0, Order++}, Less);
        while (!Queue.IsEmpty())
        {
            FEntry Current;
            Queue.HeapPop(Current, Less, EAllowShrinking::No);
            if (Current.Cost != Result.CostMap.FindChecked(Current.Cell)) { continue; }
            for (const FMoveNeighbor& Edge : Edges(Current.Cell))
            {
                const int64 Cost = static_cast<int64>(Current.Cost) + Edge.Cost;
                if (Cost > Budget) { continue; }
                const int32* Known = Result.CostMap.Find(Edge.To);
                if (Known && Cost >= *Known) { continue; }
                Result.CostMap.Add(Edge.To, static_cast<int32>(Cost));
                Result.PreviousMap.Add(Edge.To, Current.Cell);
                Queue.HeapPush({Edge.To, static_cast<int32>(Cost), Order++}, Less);
            }
        }
        Out = MoveTemp(Result);
    }

    template <typename TSearch>
    double Measure(int32 Iterations, TSearch Search)
    {
        const double Begin = FPlatformTime::Seconds();
        for (int32 Index = 0; Index < Iterations; ++Index)
        {
            FMoveRangeResult Result;
            Search(Result);
        }
        return (FPlatformTime::Seconds() - Begin) * 1000000.0 / Iterations;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingBucketComparisonTest, "WarChess.Performance.PathFindingComparison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingBucketComparisonTest::RunTest(const FString& Parameters)
{
    using namespace MoveSearchComparison;
    TArray<TSharedPtr<FJsonValue>> Cases;
    for (const int32 Radius : {4, 16, 32})
    {
        UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
        for (int32 Q = -Radius; Q <= Radius; ++Q)
        {
            for (int32 R = FMath::Max(-Radius, -Q - Radius); R <= FMath::Min(Radius, -Q + Radius); ++R)
            {
                FHexCellData Definition;
                Definition.Coordinate = FHexCoord(Q, R);
                Definition.bStandable = true;
                Asset->GridData.Add(FVector2D(Q, R), Definition);
            }
        }
        UBoard* Board = NewObject<UBoard>();
        if (!TestTrue(TEXT("Initialize benchmark board"), Board->Initialize(Asset))
            || !TestTrue(TEXT("Place benchmark unit"), Board->PlaceUnit(0, FHexCoord()))) { return false; }
        const FMoveRule Rule;
        const auto Edges = [&](const FHexCoord& Cell) { return Rule.GetNeighbors(*Board, 0, Cell); };
        const auto Bucket = [&](FMoveRangeResult& Out) { FPathFinding::FindReachable(FHexCoord(), Radius, Edges, Out); };
        const auto Old = [&](FMoveRangeResult& Out) { Dijkstra(FHexCoord(), Radius, Edges, Out); };
        FMoveRangeResult BucketResult, OldResult;
        Bucket(BucketResult);
        Old(OldResult);
        TestTrue(TEXT("Same complete range"), BucketResult.GetMoveRange() == OldResult.GetMoveRange());
        for (const TPair<FHexCoord, int32>& Pair : OldResult.CostMap)
        {
            int32 Cost = -1;
            TestTrue(TEXT("Same minimum cost"), BucketResult.GetCost(Pair.Key, Cost) && Cost == Pair.Value);
            const FHexCoord* Parent = OldResult.PreviousMap.Find(Pair.Key);
            const FHexCoord* NewParent = BucketResult.PreviousMap.Find(Pair.Key);
            TestTrue(TEXT("Same tie-selected predecessor"), Parent ? NewParent && *NewParent == *Parent : !NewParent);
        }
        if (HasAnyErrors()) { return false; }
        const int32 Iterations = Radius == 4 ? 300 : Radius == 16 ? 80 : 20;
        for (int32 Warmup = 0; Warmup < 3; ++Warmup) { Bucket(BucketResult); Old(OldResult); }
        TArray<double> BucketTimes, OldTimes;
        for (int32 Batch = 0; Batch < 5; ++Batch)
        {
            // Alternate order to reduce warm-cache / scheduling bias; never assert a timing threshold.
            if (Batch % 2 == 0)
            {
                BucketTimes.Add(Measure(Iterations, Bucket));
                OldTimes.Add(Measure(Iterations, Old));
            }
            else
            {
                OldTimes.Add(Measure(Iterations, Old));
                BucketTimes.Add(Measure(Iterations, Bucket));
            }
        }
        BucketTimes.Sort();
        OldTimes.Sort();
        const TSharedRef<FJsonObject> Case = MakeShared<FJsonObject>();
        Case->SetNumberField(TEXT("cellCount"), Board->GetCellCount());
        Case->SetNumberField(TEXT("budget"), Radius);
        Case->SetNumberField(TEXT("iterationsPerBatch"), Iterations);
        Case->SetNumberField(TEXT("bucketMedianMicroseconds"), BucketTimes[2]);
        Case->SetNumberField(TEXT("dijkstraMedianMicroseconds"), OldTimes[2]);
        Cases.Add(MakeShared<FJsonValueObject>(Case));
        AddInfo(FString::Printf(TEXT("%d cells: bucket Dijkstra %.2f us, old Dijkstra %.2f us (median of 5 batch averages)"),
            Board->GetCellCount(), BucketTimes[2], OldTimes[2]));
    }
    const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("scope"), TEXT("UE 5.4 Development Editor, open hex boards, real Board+MoveRule, Cost=1"));
    Report->SetStringField(TEXT("note"), TEXT("Both searches query budget-boundary outgoing edges; the bucket implementation scans Budget+1 containers. Includes result allocation and destruction. Local timings are not a performance guarantee."));
    Report->SetArrayField(TEXT("cases"), Cases);
    FString Json;
    const FString Output = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests/Movement/pathfinding_benchmark.json"));
    return TestTrue(TEXT("Serialize comparison"), FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)))
        && TestTrue(TEXT("Create comparison directory"), IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output), true))
        && TestTrue(TEXT("Write comparison"), FFileHelper::SaveStringToFile(Json, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
}

#endif
