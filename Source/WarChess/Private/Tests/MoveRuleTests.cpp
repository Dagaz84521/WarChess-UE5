#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Board/Board.h"
#include "Grid/HexGridDataAsset.h"
#include "Rules/MoveRule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoveRuleMovementTest, "WarChess.MoveRule.Movement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoveRuleMovementTest::RunTest(const FString& Parameters)
{
    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    const FHexCoord Ring[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    const auto AddCell = [Asset](const FHexCoord& Cell, bool bStandable, double Height)
    {
        FHexCellData Definition;
        Definition.Coordinate = Cell;
        Definition.WorldLocation = FVector(0.0, 0.0, Height);
        Definition.bStandable = bStandable;
        Asset->GridData.Add(FVector2D(Cell.Q, Cell.R), Definition);
    };
    AddCell(FHexCoord(), true, 0.0);
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Ring); ++Index)
    {
        AddCell(Ring[Index], Index != 0, (Index + 1) * 1000.0);
    }
    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Initialize board"), Board->Initialize(Asset));
    TestTrue(TEXT("Place moving unit"), Board->PlaceUnit(7, FHexCoord()));
    TestTrue(TEXT("Place blocking unit"), Board->PlaceUnit(8, Ring[2]));

    const FMoveRule Rule;
    const TArray<FMoveNeighbor> Neighbors = Rule.GetNeighbors(*Board, 7, FHexCoord());
    const FHexCoord Expected[] = {Ring[1], Ring[3], Ring[4], Ring[5]};
    TestEqual(TEXT("Reject unstandable and occupied neighbors"), Neighbors.Num(), 4);
    for (int32 Index = 0; Index < FMath::Min(Neighbors.Num(), 4); ++Index)
    {
        TestTrue(TEXT("Preserve geometric order despite large height differences"), Neighbors[Index].To == Expected[Index]);
        TestEqual(TEXT("Every step costs one"), Neighbors[Index].Cost, 1);
    }

    // 搜索扩展时单位仍站在真实起点，查询不得要求 From 是其当前占位。
    const TArray<FMoveNeighbor> Next = Rule.GetNeighbors(*Board, 7, Ring[1]);
    TestEqual(TEXT("Expand hypothetical position and allow own starting cell"), Next.Num(), 1);
    if (Next.Num() == 1)
    {
        TestTrue(TEXT("Own occupancy does not block"), Next[0].To == FHexCoord());
    }
    TestEqual(TEXT("Invalid unit ID returns no edges"), Rule.GetNeighbors(*Board, INDEX_NONE, FHexCoord()).Num(), 0);
    TestEqual(TEXT("Missing source returns no edges"), Rule.GetNeighbors(*Board, 7, FHexCoord(99, 99)).Num(), 0);
    TestEqual(TEXT("Query preserves moving unit occupancy"), Board->GetOccupant(FHexCoord()), 7);
    TestEqual(TEXT("Query preserves blocker occupancy"), Board->GetOccupant(Ring[2]), 8);
    TestEqual(TEXT("Hypothetical position remains empty"), Board->GetOccupant(Ring[1]), INDEX_NONE);
    FHexCellData Definition;
    Board->GetCellDefinition(Ring[0], Definition);
    TestFalse(TEXT("Query preserves map definition"), Definition.bStandable);

    TestTrue(TEXT("Remove blocker"), Board->RemoveUnit(8));
    TestEqual(TEXT("Later query reads current board without stale cache"), Rule.GetNeighbors(*Board, 7, FHexCoord()).Num(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoveRuleOverrideTest, "WarChess.MoveRule.Override",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoveRuleOverrideTest::RunTest(const FString& Parameters)
{
    // 验证未来玩法可以通过基类引用替换费用，而无需修改 Board。
    class FDoubleCostRule final : public FMoveRule
    {
    public:
        virtual TArray<FMoveNeighbor> GetNeighbors(
            const UBoard& Board, int32 MovingUnitId, const FHexCoord& From) const override
        {
            TArray<FMoveNeighbor> Neighbors = FMoveRule::GetNeighbors(Board, MovingUnitId, From);
            for (FMoveNeighbor& Neighbor : Neighbors)
            {
                Neighbor.Cost = 2;
            }
            return Neighbors;
        }
    };

    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    for (int32 Q = 0; Q < 2; ++Q)
    {
        FHexCellData Definition;
        Definition.Coordinate = FHexCoord(Q, 0);
        Definition.bStandable = true;
        Asset->GridData.Add(FVector2D(Q, 0), Definition);
    }
    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Initialize board"), Board->Initialize(Asset));
    TestTrue(TEXT("Place unit zero"), Board->PlaceUnit(0, FHexCoord()));
    const FDoubleCostRule DerivedRule;
    const FMoveRule& Rule = DerivedRule;
    const TArray<FMoveNeighbor> Neighbors = Rule.GetNeighbors(*Board, 0, FHexCoord());
    TestEqual(TEXT("Derived rule retains default legal neighbor"), Neighbors.Num(), 1);
    if (Neighbors.Num() == 1)
    {
        TestTrue(TEXT("Expected destination"), Neighbors[0].To == FHexCoord(1, 0));
        TestEqual(TEXT("Virtual dispatch uses derived movement cost"), Neighbors[0].Cost, 2);
    }
    return true;
}

#endif
