#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Board/Board.h"
#include "Dom/JsonObject.h"
#include "Grid/HexGridDataAsset.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PathFinding/PathFinding.h"
#include "Rules/MoveRule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace MoveJsonTests
{
    TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonValue>& Value)
    {
        return Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
    }

    TSharedPtr<FJsonObject> ObjectField(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field)
    {
        const TSharedPtr<FJsonObject>* Value = nullptr;
        return Parent.IsValid() && Parent->TryGetObjectField(Field, Value) ? *Value : nullptr;
    }

    bool IntField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& Out)
    {
        double Number;
        if (!Object.IsValid() || !Object->TryGetNumberField(Field, Number)
            || !FMath::IsFinite(Number) || Number < MIN_int32 || Number > MAX_int32)
        {
            return false;
        }
        Out = static_cast<int32>(Number);
        return Number == static_cast<double>(Out);
    }

    bool Coord(const TSharedPtr<FJsonObject>& Object, FHexCoord& Out)
    {
        return IntField(Object, TEXT("q"), Out.Q) && IntField(Object, TEXT("r"), Out.R);
    }

    TSharedRef<FJsonObject> CoordObject(const FHexCoord& Cell)
    {
        const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
        Object->SetNumberField(TEXT("q"), Cell.Q);
        Object->SetNumberField(TEXT("r"), Cell.R);
        return Object;
    }

    TSharedRef<FJsonValue> CoordValue(const FHexCoord& Cell)
    {
        return MakeShared<FJsonValueObject>(CoordObject(Cell));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathFindingJsonFixtureTest, "WarChess.PathFinding.JsonFixture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathFindingJsonFixtureTest::RunTest(const FString& Parameters)
{
    using namespace MoveJsonTests;
    const FString RelativeFixture = TEXT("Tests/Fixtures/Movement/hex_board.json");
    const FString OutputPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests/Movement/move_range_result.json"));
    // A failed rerun must not leave a previous success looking like fresh output.
    if (IFileManager::Get().FileExists(*OutputPath)
        && !IFileManager::Get().Delete(*OutputPath))
    {
        AddError(TEXT("Cannot remove stale movement JSON output"));
        return false;
    }

    FString Input;
    TSharedPtr<FJsonObject> Scenario;
    if (!TestTrue(TEXT("Load JSON fixture"), FFileHelper::LoadFileToString(Input, *FPaths::Combine(FPaths::ProjectDir(), RelativeFixture)))
        || !TestTrue(TEXT("Parse JSON fixture"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Input), Scenario)))
    {
        return false;
    }
    int32 SchemaVersion = 0;
    if (!TestTrue(TEXT("Supported fixture schema"), IntField(Scenario, TEXT("schemaVersion"), SchemaVersion) && SchemaVersion == 1))
    {
        return false;
    }
    const TSharedPtr<FJsonObject> BoardJson = ObjectField(Scenario, TEXT("board"));
    const TSharedPtr<FJsonObject> Query = ObjectField(Scenario, TEXT("query"));
    const TSharedPtr<FJsonObject> Expected = ObjectField(Scenario, TEXT("expected"));
    const TArray<TSharedPtr<FJsonValue>>* Cells = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
    if (!TestTrue(TEXT("Board cells and units arrays"), BoardJson.IsValid()
        && BoardJson->TryGetArrayField(TEXT("cells"), Cells) && BoardJson->TryGetArrayField(TEXT("units"), Units)))
    {
        return false;
    }

    UHexGridDataAsset* Asset = NewObject<UHexGridDataAsset>();
    for (const TSharedPtr<FJsonValue>& Value : *Cells)
    {
        const TSharedPtr<FJsonObject> Cell = Object(Value);
        FHexCellData Definition;
        const TArray<TSharedPtr<FJsonValue>>* WorldPosition = nullptr;
        if (!TestTrue(TEXT("Valid cell definition"), Coord(Cell, Definition.Coordinate)
            && Cell->TryGetBoolField(TEXT("bStandable"), Definition.bStandable)
            && Cell->TryGetArrayField(TEXT("worldPosition"), WorldPosition) && WorldPosition->Num() == 3))
        {
            return false;
        }
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            double Component;
            if (!TestTrue(TEXT("Finite world position component"), (*WorldPosition)[Axis]->TryGetNumber(Component) && FMath::IsFinite(Component)))
            {
                return false;
            }
            Definition.WorldLocation[Axis] = Component;
        }
        const FVector2D Key(Definition.Coordinate.Q, Definition.Coordinate.R);
        if (!TestFalse(TEXT("No duplicate fixture coordinates"), Asset->GridData.Contains(Key)))
        {
            return false;
        }
        Asset->GridData.Add(Key, Definition);
    }

    UBoard* Board = NewObject<UBoard>();
    if (!TestTrue(TEXT("Initialize real board from fixture"), Board->Initialize(Asset)))
    {
        return false;
    }
    TMap<int32, FHexCoord> InitialPositions;
    for (const TSharedPtr<FJsonValue>& Value : *Units)
    {
        const TSharedPtr<FJsonObject> Unit = Object(Value);
        int32 UnitId = INDEX_NONE;
        FHexCoord Cell;
        FHexCellData Definition;
        if (!TestTrue(TEXT("Valid unit placement"), IntField(Unit, TEXT("unitId"), UnitId)
            && Coord(Unit, Cell) && Board->GetCellDefinition(Cell, Definition) && Definition.bStandable
            && Board->PlaceUnit(UnitId, Cell)))
        {
            return false;
        }
        InitialPositions.Add(UnitId, Cell);
    }

    int32 MovingUnitId = INDEX_NONE, Budget = -1;
    FHexCoord Start, Destination;
    if (!TestTrue(TEXT("Valid movement query"), IntField(Query, TEXT("movingUnitId"), MovingUnitId)
        && IntField(Query, TEXT("moveBudget"), Budget) && Budget >= 0
        && Board->GetUnitCell(MovingUnitId, Start)
        && Coord(ObjectField(Query, TEXT("destination")), Destination) && Board->HasCell(Destination)))
    {
        return false;
    }
    const FMoveRule Rule;
    FMoveRangeResult Result;
    if (!TestTrue(TEXT("Real C++ bucket Dijkstra succeeds"), FPathFinding::FindReachable(Start, Budget,
        [&](const FHexCoord& From) { return Rule.GetNeighbors(*Board, MovingUnitId, From); }, Result)
        == EMoveRangeSearchStatus::Success))
    {
        return false;
    }

    int32 ExpectedCount = 0;
    const TArray<TSharedPtr<FJsonValue>>* ExpectedCosts = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Unreachable = nullptr;
    if (!TestTrue(TEXT("Fixture has numeric expectations"), IntField(Expected, TEXT("reachableCount"), ExpectedCount)
        && Expected->TryGetArrayField(TEXT("costs"), ExpectedCosts)
        && Expected->TryGetArrayField(TEXT("unreachable"), Unreachable)))
    {
        return false;
    }
    TestEqual(TEXT("Expected reachable cell count"), Result.CostMap.Num(), ExpectedCount);
    for (const TSharedPtr<FJsonValue>& Value : *ExpectedCosts)
    {
        FHexCoord Cell;
        int32 ExpectedCost = -1, ActualCost = -1;
        if (!TestTrue(TEXT("Valid cost expectation"), Coord(Object(Value), Cell) && IntField(Object(Value), TEXT("cost"), ExpectedCost)))
        {
            return false;
        }
        TestTrue(TEXT("Expected cell is reachable"), Result.GetCost(Cell, ActualCost));
        TestEqual(TEXT("Expected lowest cumulative cost"), ActualCost, ExpectedCost);
    }
    for (const TSharedPtr<FJsonValue>& Value : *Unreachable)
    {
        FHexCoord Cell;
        if (!TestTrue(TEXT("Valid unreachable expectation"), Coord(Object(Value), Cell)))
        {
            return false;
        }
        TestFalse(TEXT("Expected cell is unreachable"), Result.CostMap.Contains(Cell));
    }

    TestEqual(TEXT("Tree has one parent per non-root node"), Result.PreviousMap.Num(), Result.CostMap.Num() - 1);
    TArray<TSharedPtr<FJsonValue>> ReachableJson;
    for (const FHexCoord& Cell : Result.GetMoveRange())
    {
        const int32 Cost = Result.CostMap.FindChecked(Cell);
        TestTrue(TEXT("Cost lies within budget"), Cost >= 0 && Cost <= Budget);
        TArray<FHexCoord> Path;
        TestTrue(TEXT("Every node has complete path"), Result.BuildPath(Cell, Path));
        TestEqual(TEXT("Default rule path length equals action cost"), Path.Num() - 1, Cost);
        for (int32 Index = 1; Index < Path.Num(); ++Index)
        {
            TestTrue(TEXT("Every path edge is legal under real rule"), Rule.GetNeighbors(*Board, MovingUnitId, Path[Index - 1])
                .ContainsByPredicate([&](const FMoveNeighbor& Edge) { return Edge.To == Path[Index] && Edge.Cost == 1; }));
        }
        // Closure + relaxation checks validate completeness and minimum costs without a second solver.
        for (const FMoveNeighbor& Edge : Rule.GetNeighbors(*Board, MovingUnitId, Cell))
        {
            if (static_cast<int64>(Cost) + Edge.Cost <= Budget)
            {
                const int32* NextCost = Result.CostMap.Find(Edge.To);
                TestTrue(TEXT("All budget-feasible neighbors included at minimum cost"), NextCost && *NextCost <= Cost + Edge.Cost);
            }
        }
        const TSharedRef<FJsonObject> Node = CoordObject(Cell);
        Node->SetNumberField(TEXT("cost"), Cost);
        const FHexCoord* Previous = Result.PreviousMap.Find(Cell);
        if (Previous)
        {
            Node->SetObjectField(TEXT("previous"), CoordObject(*Previous));
        }
        else
        {
            Node->SetField(TEXT("previous"), MakeShared<FJsonValueNull>());
        }
        ReachableJson.Add(MakeShared<FJsonValueObject>(Node));
    }
    for (const TPair<int32, FHexCoord>& Pair : InitialPositions)
    {
        FHexCoord ActualCell;
        TestTrue(TEXT("Search leaves unit position unchanged"), Board->GetUnitCell(Pair.Key, ActualCell) && ActualCell == Pair.Value);
    }
    TArray<FHexCoord> SelectedPath;
    if (!TestTrue(TEXT("Selected target has complete path"), Result.BuildPath(Destination, SelectedPath)) || HasAnyErrors())
    {
        return false;
    }

    TArray<TSharedPtr<FJsonValue>> PathJson;
    for (const FHexCoord& Cell : SelectedPath)
    {
        PathJson.Add(CoordValue(Cell));
    }
    const TSharedRef<FJsonObject> Selected = MakeShared<FJsonObject>();
    Selected->SetObjectField(TEXT("destination"), CoordObject(Destination));
    Selected->SetNumberField(TEXT("totalCost"), Result.CostMap.FindChecked(Destination));
    Selected->SetArrayField(TEXT("cells"), PathJson);
    const TSharedRef<FJsonObject> MoveResult = MakeShared<FJsonObject>();
    MoveResult->SetObjectField(TEXT("start"), CoordObject(Start));
    MoveResult->SetArrayField(TEXT("reachable"), ReachableJson);
    MoveResult->SetObjectField(TEXT("selectedPath"), Selected);
    const TSharedRef<FJsonObject> Output = MakeShared<FJsonObject>();
    Output->SetNumberField(TEXT("schemaVersion"), 1);
    Output->SetStringField(TEXT("searchAlgorithm"), TEXT("Bucket Dijkstra"));
    Output->SetStringField(TEXT("sourceFixture"), RelativeFixture);
    FString ScenarioName;
    Scenario->TryGetStringField(TEXT("name"), ScenarioName);
    Output->SetStringField(TEXT("name"), ScenarioName);
    Output->SetObjectField(TEXT("board"), BoardJson);
    Output->SetObjectField(TEXT("query"), Query);
    Output->SetObjectField(TEXT("moveResult"), MoveResult);
    FString Serialized;
    if (!TestTrue(TEXT("Serialize actual search result"), FJsonSerializer::Serialize(Output, TJsonWriterFactory<>::Create(&Serialized)))
        || !TestTrue(TEXT("Create export directory"), IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true))
        || !TestTrue(TEXT("Write result JSON"), FFileHelper::SaveStringToFile(Serialized, *OutputPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)))
    {
        return false;
    }
    AddInfo(FString::Printf(TEXT("Movement JSON exported: %s (%d reachable cells, target cost %d)"),
        *FPaths::ConvertRelativePathToFull(OutputPath), Result.CostMap.Num(), Result.CostMap.FindChecked(Destination)));
    return true;
}

#endif
