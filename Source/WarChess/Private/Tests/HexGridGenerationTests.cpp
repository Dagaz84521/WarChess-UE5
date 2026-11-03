#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Board/Board.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Grid/HexGridGenerator.h"
#include "Grid/HexMathLibrary.h"
#include "Materials/Material.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHexGridGenerationTest, "WarChess.Grid.Generation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexGridGenerationTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false)
        .CreateFXSystem(false).ShouldSimulatePhysics(false).EnableTraceCollision(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Test world"), World))
    {
        return false;
    }

    // A wide flat ground with its top at Z=0, independent of project maps.
    AActor* Ground = World->SpawnActor<AActor>();
    UBoxComponent* GroundBox = NewObject<UBoxComponent>(Ground);
    Ground->SetRootComponent(GroundBox);
    GroundBox->SetBoxExtent(FVector(5000.0, 5000.0, 10.0));
    GroundBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GroundBox->SetCollisionResponseToAllChannels(ECR_Block);
    GroundBox->RegisterComponent();
    Ground->SetActorLocation(FVector(0.0, 0.0, -10.0));

    AHexGridGenerator* Generator = World->SpawnActor<AHexGridGenerator>();
    Generator->BoundsComponent->SetBoxExtent(FVector(300.0, 300.0, 100.0));
    Generator->SetActorLocation(FVector(150.0, -200.0, 0.0));
    Generator->SetActorRotation(FRotator(0.0, 37.0, 0.0));
    Generator->GenerateRuntimeGrid();
    const int32 CellCount = Generator->GridData.Num();
    TestTrue(TEXT("Cells generated without a decal material"), CellCount > 0);
    for (const TPair<FHexCoord, FHexCellData>& Pair : Generator->GridData)
    {
        TestTrue(TEXT("Flat ground is passable"), Pair.Value.bStandable);
        TestTrue(TEXT("Ground offset applied"), FMath::IsNearlyEqual(Pair.Value.WorldLocation.Z, 5.0, 0.01));
        FHexCoord Picked;
        TestTrue(TEXT("Rotated cell can be located"), Generator->GetCellAtWorldLocation(Pair.Value.WorldLocation, Picked));
        TestTrue(TEXT("Rotated coordinate round trip"), Picked == Pair.Key);
    }

    // An obstacle at standing height must mark the central cell as blocked.
    AActor* Obstacle = World->SpawnActor<AActor>();
    UBoxComponent* ObstacleBox = NewObject<UBoxComponent>(Obstacle);
    Obstacle->SetRootComponent(ObstacleBox);
    ObstacleBox->SetBoxExtent(FVector(15.0, 15.0, 15.0));
    ObstacleBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ObstacleBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    ObstacleBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    ObstacleBox->RegisterComponent();
    Obstacle->SetActorLocation(Generator->GetActorLocation() + FVector(0.0, 0.0, 95.0));
    Generator->GenerateRuntimeGrid();
    const FHexCellData* Center = Generator->GridData.Find(FHexCoord());
    if (TestNotNull(TEXT("Center cell"), Center))
    {
        TestFalse(TEXT("Standing obstacle blocks cell"), Center->bStandable);
    }

    Generator->TargetGridDataAsset = NewObject<UHexGridDataAsset>();
    Generator->GenerateGrid();
    TestEqual(TEXT("Export contains all cells"), Generator->TargetGridDataAsset->GridData.Num(), CellCount);
    TestTrue(TEXT("Export preserves grid yaw"), FMath::IsNearlyEqual(Generator->TargetGridDataAsset->GridYaw, 37.0f));
    const FHexCellData* ExportedCenter = Generator->TargetGridDataAsset->GridData.Find(FVector2D::ZeroVector);
    if (TestNotNull(TEXT("Exported center cell"), ExportedCenter))
    {
        TestFalse(TEXT("Export preserves blocked state"), ExportedCenter->bStandable);
    }

    UBoard* Board = NewObject<UBoard>();
    TestTrue(TEXT("Generated asset initializes runtime board"), Board->Initialize(Generator->TargetGridDataAsset));
    TestEqual(TEXT("Board retains every generated cell"), Board->GetCellCount(), CellCount);
    FHexCellData BoardCenter;
    if (TestTrue(TEXT("Board contains generated center"), Board->GetCellDefinition(FHexCoord(), BoardCenter)))
    {
        TestFalse(TEXT("Board preserves generated standability"), BoardCenter.bStandable);
    }

    UMaterial* GridMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/Grid/M_HexDecal.M_HexDecal"));
    TestNotNull(TEXT("Migrated decal material loads without the plugin"), GridMaterial);
    Generator->DecalMaterial = GridMaterial;
    Generator->GenerateRuntimeGrid();
    Generator->GenerateRuntimeGrid();
    TArray<UDecalComponent*> Decals;
    Generator->GetComponents(Decals);
    TestEqual(TEXT("Repeated generation does not duplicate decals"), Decals.Num(), CellCount);
    Generator->PreviewGridWires();
    Generator->PreviewGridWires();
    TArray<UTextRenderComponent*> Texts;
    Generator->GetComponents(Texts);
    TestEqual(TEXT("Repeated preview does not duplicate coordinate labels"), Texts.Num(), CellCount);
    Generator->ClearGrid();
    Generator->GetComponents(Texts);
    Generator->GetComponents(Decals);
    TestEqual(TEXT("Clear removes labels"), Texts.Num(), 0);
    TestEqual(TEXT("Clear removes decals"), Decals.Num(), 0);
    TestEqual(TEXT("Clear removes cells"), Generator->GridData.Num(), 0);

    GroundBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Generator->GenerateRuntimeGrid();
    for (const TPair<FHexCoord, FHexCellData>& Pair : Generator->GridData)
    {
        TestFalse(TEXT("Missing ground is impassable"), Pair.Value.bStandable);
    }

    Generator->HexSize = 0.0f;
    Generator->PreviewGridWires();
    TestEqual(TEXT("Zero hex size produces no cells"), Generator->GridData.Num(), 0);
    World->DestroyWorld(false);
    return true;
}

#endif
