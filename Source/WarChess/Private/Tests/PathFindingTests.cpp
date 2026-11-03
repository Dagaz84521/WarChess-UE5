#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Board/Board.h"
#include "Grid/HexGridDataAsset.h"
#include "PathFinding/PathFinding.h"
#include "Rules/MoveRule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingBreadthFirstTest, "WarChess.PathFinding.UnitCostRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingBreadthFirstTest::RunTest(const FString& Parameters)
{
    const FHexCoord Start(0, 0), A(1, 0), B(2, 0), Goal(3, 0), Beyond(4, 0);
    TMap<FHexCoord, TArray<FMoveNeighbor>> Graph;
    Graph.Add(Start, {{B, 1}, {A, 1}});
    Graph.Add(B, {{Start, 1}, {Goal, 1}});
    Graph.Add(A, {{Goal, 1}});
    Graph.Add(Goal, {{Beyond, 1}});
    TMap<FHexCoord, int32> Calls;
    int32 TotalCalls = 0;
    const auto GetNeighbors = [&](const FHexCoord& Cell)
    {
        ++TotalCalls;
        ++Calls.FindOrAdd(Cell);
        const TArray<FMoveNeighbor>* Edges = Graph.Find(Cell);
        return Edges ? *Edges : TArray<FMoveNeighbor>();
    };
    FMoveRangeResult Result;
    TestTrue(TEXT("Unit cost range succeeds"), FPathFinding::FindReachable(Start, 2, GetNeighbors, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("All budget-reachable nodes retained"), Result.CostMap.Num(), 4);
    TestEqual(TEXT("Goal has minimum step cost"), Result.CostMap.FindChecked(Goal), 2);
    TestFalse(TEXT("Beyond budget excluded"), Result.CostMap.Contains(Beyond));
    TestEqual(TEXT("Root cost zero"), Result.CostMap.FindChecked(Start), 0);
    TestFalse(TEXT("Root has no predecessor"), Result.PreviousMap.Contains(Start));
    TestTrue(TEXT("Equal distance keeps first neighbor as parent"), Result.PreviousMap.FindChecked(Goal) == B);
    TestEqual(TEXT("Shared goal is only enqueued once"), Result.PreviousMap.Num(), 3);
    TestEqual(TEXT("Cycle does not expand root twice"), Calls.FindChecked(Start), 1);
    TestEqual(TEXT("Budget boundary queries possible zero-cost edges"), Calls.FindChecked(Goal), 1);
    const int32 CallsBeforeQueries = TotalCalls;
    const TArray<FHexCoord> Expected = {Start, B, Goal};
    TArray<FHexCoord> Path;
    TestTrue(TEXT("Restore shortest path"), Result.BuildPath(Goal, Path));
    TestTrue(TEXT("Path includes both endpoints"), Path == Expected);
    TestTrue(TEXT("Root path succeeds"), Result.BuildPath(Start, Path));
    TestTrue(TEXT("Root path contains only root"), Path == TArray<FHexCoord>{Start});
    TestFalse(TEXT("Unreachable path fails"), Result.BuildPath(Beyond, Path));
    TestTrue(TEXT("Failed path clears output"), Path.IsEmpty());
    int32 Cost = 99;
    TestTrue(TEXT("Read cost"), Result.GetCost(Goal, Cost));
    TestEqual(TEXT("Reported cost"), Cost, 2);
    TestFalse(TEXT("Missing cost fails"), Result.GetCost(Beyond, Cost));
    TestEqual(TEXT("Missing cost clears output"), Cost, INDEX_NONE);
    const TArray<FHexCoord> ExpectedRange = {Start, A, B, Goal};
    TestTrue(TEXT("Range has stable coordinate order"), Result.GetMoveRange() == ExpectedRange);
    TestEqual(TEXT("Result queries do not search again"), TotalCalls, CallsBeforeQueries);
    FMoveRangeResult Broken = Result;
    Broken.PreviousMap.Remove(Goal);
    TestFalse(TEXT("Missing predecessor fails"), Broken.BuildPath(Goal, Path));
    TestTrue(TEXT("Missing predecessor returns no partial path"), Path.IsEmpty());
    Broken = Result;
    Broken.PreviousMap.Add(Goal, Goal);
    TestFalse(TEXT("Cyclic predecessor fails"), Broken.BuildPath(Goal, Path));
    TestTrue(TEXT("Cyclic predecessor returns no partial path"), Path.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingBudgetTest, "WarChess.PathFinding.BudgetAndCycles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingBudgetTest::RunTest(const FString& Parameters)
{
    const FHexCoord Start(0, 0), A(1, 0), B(2, 0), Goal(3, 0);
    TMap<FHexCoord, TArray<FMoveNeighbor>> Graph;
    Graph.Add(Start, {{B, 1}, {A, 1}});
    Graph.Add(B, {{Start, 1}, {A, 1}, {Goal, 1}});
    Graph.Add(A, {{B, 1}, {Goal, 1}});
    int32 Calls = 0;
    const auto GetNeighbors = [&](const FHexCoord& Cell)
    {
        ++Calls;
        const TArray<FMoveNeighbor>* Edges = Graph.Find(Cell);
        return Edges ? *Edges : TArray<FMoveNeighbor>();
    };
    FMoveRangeResult Result;
    TestTrue(TEXT("Zero budget succeeds"), FPathFinding::FindReachable(Start, 0, GetNeighbors, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Zero budget includes only root"), Result.CostMap.Num(), 1);
    TestEqual(TEXT("Zero budget queries root for free edges"), Calls, 1);
    TestTrue(TEXT("Zero budget has no parents"), Result.PreviousMap.IsEmpty());
    TestTrue(TEXT("Budget one succeeds"), FPathFinding::FindReachable(Start, 1, GetNeighbors, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Budget one includes first layer only"), Result.CostMap.Num(), 3);
    TestFalse(TEXT("Second layer excluded"), Result.CostMap.Contains(Goal));
    TestTrue(TEXT("Budget two succeeds"), FPathFinding::FindReachable(Start, 2, GetNeighbors, Result)
        == EMoveRangeSearchStatus::Success);
    TestFalse(TEXT("Cycle cannot assign root parent"), Result.PreviousMap.Contains(Start));
    TArray<FHexCoord> Path;
    const TArray<FHexCoord> Expected = {Start, B, Goal};
    TestTrue(TEXT("Stable tie path"), Result.BuildPath(Goal, Path) && Path == Expected);
    FMoveRangeResult Repeated;
    TestTrue(TEXT("Repeat succeeds"), FPathFinding::FindReachable(Start, 2, GetNeighbors, Repeated)
        == EMoveRangeSearchStatus::Success);
    TestTrue(TEXT("Repeated search retains tie choice"), Repeated.BuildPath(Goal, Path) && Path == Expected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingInvalidInputTest, "WarChess.PathFinding.InvalidInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingInvalidInputTest::RunTest(const FString& Parameters)
{
    const FHexCoord Start(0, 0), A(1, 0);
    int32 Calls = 0;
    const auto EmptyProvider = [&](const FHexCoord& Cell)
    {
        ++Calls;
        return TArray<FMoveNeighbor>();
    };
    FMoveRangeResult Result;
    Result.CostMap.Add(A, 99);
    TestTrue(TEXT("Negative budget rejected"), FPathFinding::FindReachable(Start, -1, EmptyProvider, Result)
        == EMoveRangeSearchStatus::InvalidBudget);
    TestTrue(TEXT("Failure clears old result"), Result.CostMap.IsEmpty() && Result.PreviousMap.IsEmpty());
    TestEqual(TEXT("Invalid budget does not invoke provider"), Calls, 0);
    for (const int32 InvalidCost : {-1, MIN_int32})
    {
        const auto InvalidProvider = [&](const FHexCoord& Cell)
        {
            // Also validate unsupported costs on edges leading to an already visited node.
            return Cell == Start ? TArray<FMoveNeighbor>{{A, 1}} : TArray<FMoveNeighbor>{{Start, InvalidCost}};
        };
        TestTrue(TEXT("Negative cost rejected"), FPathFinding::FindReachable(Start, 2, InvalidProvider, Result)
            == EMoveRangeSearchStatus::InvalidEdgeCost);
        TestTrue(TEXT("Invalid edge leaves no partial tree"), Result.CostMap.IsEmpty() && Result.PreviousMap.IsEmpty());
    }
    const auto UnitProvider = [&](const FHexCoord& Cell)
    {
        return Cell == Start ? TArray<FMoveNeighbor>{{A, 1}} : TArray<FMoveNeighbor>();
    };
    TestTrue(TEXT("Oversized budget rejected before allocation"), FPathFinding::FindReachable(Start, MAX_int32, UnitProvider, Result)
        == EMoveRangeSearchStatus::InvalidBudget);
    TestTrue(TEXT("Largest supported budget succeeds"), FPathFinding::FindReachable(Start, FPathFinding::MaxSupportedBudget, UnitProvider, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Unit cost remains one"), Result.CostMap.FindChecked(A), 1);
    Result.Start = A;
    TestTrue(TEXT("Aliased start input supported"), FPathFinding::FindReachable(Result.Start, 0, EmptyProvider, Result)
        == EMoveRangeSearchStatus::Success);
    TestTrue(TEXT("Nonzero aliased start preserved"), Result.Start == A && Result.CostMap.Num() == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingBucketRelaxationTest, "WarChess.PathFinding.BucketRelaxation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingBucketRelaxationTest::RunTest(const FString& Parameters)
{
    const FHexCoord Start(0, 0), A(1, 0), B(2, 0), C(3, 0), Goal(4, 0), Beyond(5, 0);
    TMap<FHexCoord, TArray<FMoveNeighbor>> Graph;
    Graph.Add(Start, {{A, 5}, {B, 1}, {C, 2}, {Beyond, MAX_int32}});
    Graph.Add(B, {{A, 1}, {Goal, 5}});
    Graph.Add(C, {{Goal, 1}});
    Graph.Add(A, {{Goal, 1}});
    Graph.Add(Goal, {{Beyond, 6}});
    TMap<FHexCoord, int32> Calls;
    const auto Edges = [&](const FHexCoord& Cell)
    {
        ++Calls.FindOrAdd(Cell);
        const TArray<FMoveNeighbor>* Found = Graph.Find(Cell);
        return Found ? *Found : TArray<FMoveNeighbor>();
    };
    FMoveRangeResult Result;
    TestTrue(TEXT("Weighted bucket search succeeds"), FPathFinding::FindReachable(Start, 6, Edges, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Weighted range complete"), Result.CostMap.Num(), 5);
    TestEqual(TEXT("Relaxation improves earlier expensive discovery"), Result.CostMap.FindChecked(A), 2);
    TestTrue(TEXT("Improved predecessor"), Result.PreviousMap.FindChecked(A) == B);
    TestEqual(TEXT("Goal minimum cost"), Result.CostMap.FindChecked(Goal), 3);
    TestTrue(TEXT("FIFO inside bucket preserves first equal-cost predecessor"), Result.PreviousMap.FindChecked(Goal) == C);
    TestEqual(TEXT("Old expensive bucket entry skipped"), Calls.FindChecked(A), 1);
    TestEqual(TEXT("Old goal entry skipped"), Calls.FindChecked(Goal), 1);
    TestFalse(TEXT("Huge and over-budget costs safely skipped"), Result.CostMap.Contains(Beyond));
    TArray<FHexCoord> Path;
    const TArray<FHexCoord> ExpectedA = {Start, B, A};
    TestTrue(TEXT("Weighted path follows improved tree"), Result.BuildPath(A, Path) && Path == ExpectedA);
    const TArray<FHexCoord> ExpectedGoal = {Start, C, Goal};
    TestTrue(TEXT("Weighted path cost need not equal number of steps"), Result.BuildPath(Goal, Path) && Path == ExpectedGoal);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingFreeEdgesTest, "WarChess.PathFinding.ZeroCostClosure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingFreeEdgesTest::RunTest(const FString& Parameters)
{
    const FHexCoord Start(0, 0), A(1, 0), B(2, 0), Paid(3, 0);
    TMap<FHexCoord, TArray<FMoveNeighbor>> Graph;
    Graph.Add(Start, {{A, 0}, {Paid, 1}});
    Graph.Add(A, {{Start, 0}, {B, 0}});
    Graph.Add(B, {{A, 0}});
    int32 Calls = 0;
    const auto Edges = [&](const FHexCoord& Cell)
    {
        ++Calls;
        const TArray<FMoveNeighbor>* Found = Graph.Find(Cell);
        return Found ? *Found : TArray<FMoveNeighbor>();
    };
    FMoveRangeResult Result;
    TestTrue(TEXT("Zero budget processes new entries in same bucket"), FPathFinding::FindReachable(Start, 0, Edges, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Full free closure"), Result.CostMap.Num(), 3);
    TestEqual(TEXT("Zero cycle terminates"), Calls, 3);
    TestFalse(TEXT("Root never gains predecessor"), Result.PreviousMap.Contains(Start));
    TestFalse(TEXT("Positive cost stays outside budget"), Result.CostMap.Contains(Paid));
    TArray<FHexCoord> Path;
    const TArray<FHexCoord> Expected = {Start, A, B};
    TestTrue(TEXT("Free path restores without cycle"), Result.BuildPath(B, Path) && Path == Expected);
    TestEqual(TEXT("Free node cost zero"), Result.CostMap.FindChecked(B), 0);
    const auto BoundaryEdges = [&](const FHexCoord& Cell)
    {
        if (Cell == Start) { return TArray<FMoveNeighbor>{{A, 2}}; }
        if (Cell == A) { return TArray<FMoveNeighbor>{{B, 0}}; }
        return TArray<FMoveNeighbor>();
    };
    TestTrue(TEXT("Free edge at nonzero budget boundary"), FPathFinding::FindReachable(Start, 2, BoundaryEdges, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Boundary zero edge included"), Result.CostMap.FindChecked(B), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingBoardIntegrationTest, "WarChess.PathFinding.BoardIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingBoardIntegrationTest::RunTest(const FString& Parameters)
{
    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    const FHexCoord Blocked(1, 0), Occupied(-1, 0);
    for (int32 Q = -2; Q <= 2; ++Q)
    {
        for (int32 R = -2; R <= 2; ++R)
        {
            if (FMath::Abs(Q + R) > 2)
            {
                continue;
            }
            FHexCellData Definition;
            Definition.Coordinate = FHexCoord(Q, R);
            Definition.bStandable = !(Definition.Coordinate == Blocked);
            Definition.WorldLocation.Z = Q * 500.0 + R * 100.0;
            Asset->GridData.Add(FVector2D(Q, R), Definition);
        }
    }
    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Initialize radius-two board"), Board->Initialize(Asset));
    TestTrue(TEXT("Place moving unit"), Board->PlaceUnit(0, FHexCoord()));
    TestTrue(TEXT("Place blocking unit"), Board->PlaceUnit(9, Occupied));
    const FMoveRule Rule;
    int32 Calls = 0;
    const auto GetNeighbors = [&](const FHexCoord& From)
    {
        ++Calls;
        return Rule.GetNeighbors(*Board, 0, From);
    };
    FMoveRangeResult Result;
    TestTrue(TEXT("Board and rule bind to generic search"), FPathFinding::FindReachable(FHexCoord(), 2, GetNeighbors, Result)
        == EMoveRangeSearchStatus::Success);
    TestEqual(TEXT("Complete legal radius-two range"), Result.CostMap.Num(), 15);
    TestFalse(TEXT("Blocked cell excluded"), Result.CostMap.Contains(Blocked));
    TestFalse(TEXT("Other unit cell excluded"), Result.CostMap.Contains(Occupied));
    TestFalse(TEXT("Cell behind obstacle requires more than budget"), Result.CostMap.Contains(FHexCoord(2, 0)));
    const int32 CallsAfterSearch = Calls;
    for (const TPair<FHexCoord, int32>& Pair : Result.CostMap)
    {
        TArray<FHexCoord> Path;
        TestTrue(TEXT("Every reachable cell has complete path"), Result.BuildPath(Pair.Key, Path));
        TestEqual(TEXT("Path steps equal cumulative cost"), Path.Num() - 1, Pair.Value);
        TestTrue(TEXT("Within budget"), Pair.Value <= 2);
    }
    TestEqual(TEXT("Multiple path previews do not search again"), Calls, CallsAfterSearch);
    TestEqual(TEXT("Search preserves moving unit"), Board->GetOccupant(FHexCoord()), 0);
    TestEqual(TEXT("Search preserves blocker"), Board->GetOccupant(Occupied), 9);
    return true;
}

#endif
