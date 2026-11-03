#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Board/Board.h"
#include "Grid/HexGridDataAsset.h"
#include "UObject/CoreRedirects.h"

namespace BoardTests
{
    void AddCell(UHexGridDataAsset* Asset, const FHexCoord& Coord, bool bStandable = true, double Z = 0.0)
    {
        FHexCellData Definition;
        Definition.Coordinate = Coord;
        Definition.WorldLocation = FVector(0.0, 0.0, Z);
        Definition.bStandable = bStandable;
        Asset->GridData.Add(FVector2D(Coord.Q, Coord.R), Definition);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoardInitializationTest, "WarChess.Board.Initialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBoardInitializationTest::RunTest(const FString& Parameters)
{
    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    BoardTests::AddCell(Asset, FHexCoord(0, 0));
    BoardTests::AddCell(Asset, FHexCoord(1, 0), false, 120.0);
    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Initialize from asset"), Board->Initialize(Asset));
    TestEqual(TEXT("Blocked definitions retained"), Board->GetCellCount(), 2);

    FHexCellData Definition;
    TestTrue(TEXT("Read blocked cell"), Board->GetCellDefinition(FHexCoord(1, 0), Definition));
    TestFalse(TEXT("Standability preserved"), Definition.bStandable);
    TestEqual(TEXT("Height preserved"), Definition.WorldLocation.Z, 120.0);
    Definition.bStandable = true;
    Board->GetCellDefinition(FHexCoord(1, 0), Definition);
    TestFalse(TEXT("Query copy cannot mutate board"), Definition.bStandable);
    Asset->GridData.Empty();
    TestEqual(TEXT("Board owns a snapshot independent of asset"), Board->GetCellCount(), 2);

    FBoardCellState State;
    TestTrue(TEXT("Unrecorded existing cell has default state"), Board->GetCellState(FHexCoord(0, 0), State));
    TestFalse(TEXT("Default state is empty"), State.HasOccupant());
    TestTrue(TEXT("Place unit before invalid initialization"), Board->PlaceUnit(0, FHexCoord(0, 0)));
    TestFalse(TEXT("Null asset rejected"), Board->Initialize(nullptr));
    BoardTests::AddCell(Asset, FHexCoord(1, 0));
    Asset->GridData.FindChecked(FVector2D(1.0, 0.0)).Coordinate = FHexCoord(2, 0);
    TestFalse(TEXT("Mismatched asset key rejected"), Board->Initialize(Asset));
    TestEqual(TEXT("Failed initialization preserves occupancy"), Board->GetOccupant(FHexCoord()), 0);
    TestEqual(TEXT("Failed initialization preserves definitions"), Board->GetCellCount(), 2);

    Asset->GridData.Empty();
    BoardTests::AddCell(Asset, FHexCoord(8, -3));
    TestTrue(TEXT("Reinitialize"), Board->Initialize(Asset));
    FHexCoord Position;
    TestFalse(TEXT("Reinitialize clears unit positions"), Board->GetUnitCell(0, Position));
    TestFalse(TEXT("Old definitions removed"), Board->HasCell(FHexCoord()));
    TestTrue(TEXT("New definitions loaded"), Board->HasCell(FHexCoord(8, -3)));
    Asset->GridData.Empty();
    TestTrue(TEXT("Empty asset is valid"), Board->Initialize(Asset));
    TestEqual(TEXT("Empty board"), Board->GetCellCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoardAdjacencyTest, "WarChess.Board.Adjacency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBoardAdjacencyTest::RunTest(const FString& Parameters)
{
    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    const FHexCoord Ring[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    BoardTests::AddCell(Asset, FHexCoord());
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Ring); ++Index)
    {
        BoardTests::AddCell(Asset, Ring[Index], Index != 0, Index * 100.0);
    }
    BoardTests::AddCell(Asset, FHexCoord(3, 0));
    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Initialize"), Board->Initialize(Asset));
    TestTrue(TEXT("Occupy one neighbor"), Board->PlaceUnit(1, Ring[1]));
    const TArray<FHexCoord> Neighbors = Board->GetAdjacentCells(FHexCoord());
    TestEqual(TEXT("Six neighbors including blocked, occupied and elevated cells"), Neighbors.Num(), 6);
    for (int32 Index = 0; Index < FMath::Min(Neighbors.Num(), 6); ++Index)
    {
        TestTrue(TEXT("Stable axial direction order"), Neighbors[Index] == Ring[Index]);
    }
    TestEqual(TEXT("Missing source has no neighbors"), Board->GetAdjacentCells(FHexCoord(2, 0)).Num(), 0);
    const TArray<FHexCoord> EdgeNeighbors = Board->GetAdjacentCells(Ring[0]);
    TestEqual(TEXT("Boundary only returns existing cells"), EdgeNeighbors.Num(), 3);

    // 有效 int32 边界坐标也不能产生环绕到另一侧的假邻居。
    Asset->GridData.Empty();
    BoardTests::AddCell(Asset, FHexCoord(MAX_int32, 0));
    BoardTests::AddCell(Asset, FHexCoord(MAX_int32 - 1, 0));
    BoardTests::AddCell(Asset, FHexCoord(MIN_int32, 0));
    TestTrue(TEXT("Initialize coordinate boundary"), Board->Initialize(Asset));
    const TArray<FHexCoord> LimitNeighbors = Board->GetAdjacentCells(FHexCoord(MAX_int32, 0));
    TestEqual(TEXT("No overflowing neighbor"), LimitNeighbors.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoardOccupancyTest, "WarChess.Board.Occupancy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBoardOccupancyTest::RunTest(const FString& Parameters)
{
    const FHexCoord Start(0, 0), Destination(1, 0), Blocked(2, 0), Missing(99, 99);
    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    BoardTests::AddCell(Asset, Start);
    BoardTests::AddCell(Asset, Destination);
    BoardTests::AddCell(Asset, Blocked, false);
    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Initialize"), Board->Initialize(Asset));
    TestFalse(TEXT("Negative ID rejected"), Board->PlaceUnit(-2, Start));
    TestFalse(TEXT("None ID rejected"), Board->PlaceUnit(INDEX_NONE, Start));
    TestFalse(TEXT("Missing placement rejected"), Board->PlaceUnit(0, Missing));
    TestTrue(TEXT("Zero is a valid unit ID"), Board->PlaceUnit(0, Start));
    TestFalse(TEXT("Unit cannot occupy two cells"), Board->PlaceUnit(0, Destination));
    TestFalse(TEXT("Cell cannot contain two units"), Board->PlaceUnit(1, Start));
    TestTrue(TEXT("Place second unit"), Board->PlaceUnit(1, Destination));
    TestFalse(TEXT("Cannot move onto another unit"), Board->MoveUnit(0, Destination));
    TestFalse(TEXT("Cannot move to missing cell"), Board->MoveUnit(0, Missing));
    TestFalse(TEXT("Same-cell move rejected"), Board->MoveUnit(0, Start));
    TestFalse(TEXT("Absent unit cannot move"), Board->MoveUnit(2, Blocked));
    TestEqual(TEXT("Failed moves preserve source"), Board->GetOccupant(Start), 0);
    TestEqual(TEXT("Failed moves preserve destination"), Board->GetOccupant(Destination), 1);
    TestTrue(TEXT("Remove second unit"), Board->RemoveUnit(1));
    TestFalse(TEXT("Removing absent unit fails"), Board->RemoveUnit(1));
    TestTrue(TEXT("Move into freed cell"), Board->MoveUnit(0, Destination));
    TestEqual(TEXT("Source freed"), Board->GetOccupant(Start), INDEX_NONE);
    TestEqual(TEXT("Destination occupied"), Board->GetOccupant(Destination), 0);
    FHexCoord Position;
    TestTrue(TEXT("Position lookup"), Board->GetUnitCell(0, Position));
    TestTrue(TEXT("Position agrees with occupancy"), Position == Destination);

    // Board 是状态提交层，玩法资格交给 MoveRule；未来飞行也可沿用这些接口。
    TestTrue(TEXT("Board does not hardcode standability rules"), Board->PlaceUnit(2, Blocked));
    FBoardCellState State;
    TestTrue(TEXT("Released cell has empty state"), Board->GetCellState(Start, State));
    TestFalse(TEXT("Released state is unoccupied"), State.HasOccupant());
    State.OccupantUnitId = 123;
    TestFalse(TEXT("Missing cell differs from empty cell"), Board->GetCellState(Missing, State));
    TestFalse(TEXT("Failed query clears output"), State.HasOccupant());
    Board->Reset();
    TestEqual(TEXT("Reset clears definitions"), Board->GetCellCount(), 0);
    TestEqual(TEXT("Reset clears occupancy"), Board->GetOccupant(Destination), INDEX_NONE);
    TestFalse(TEXT("Reset clears derived position"), Board->GetUnitCell(0, Position));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoardStandabilityRedirectTest, "WarChess.Board.StandabilityRedirect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBoardStandabilityRedirectTest::RunTest(const FString& Parameters)
{
    const FCoreRedirectObjectName OldName(FString(TEXT("/Script/WarChess.HexCellData.bIsPassable")));
    const FCoreRedirectObjectName NewName(FString(TEXT("/Script/WarChess.HexCellData.bStandable")));
    TestTrue(TEXT("Legacy serialized property name resolves to standability"),
        FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Property, OldName) == NewName);
    return true;
}

#endif
