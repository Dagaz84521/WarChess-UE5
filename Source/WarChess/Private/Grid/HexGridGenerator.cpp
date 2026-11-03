#include "Grid/HexGridGenerator.h"
#include "Engine/World.h"
#include "Grid/HexMathLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AHexGridGenerator::AHexGridGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    // 初始化包围盒，默认给个 1000x1000 的范围
    BoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    RootComponent = BoundsComponent;
    BoundsComponent->SetBoxExtent(FVector(1000.f, 1000.f, 100.f));
    BoundsComponent->SetLineThickness(5.0f);
    BoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHexGridGenerator::PreviewGridWires()
{
    ClearGrid();
    if (!GetWorld() || !BoundsComponent || !FMath::IsFinite(HexSize) || HexSize <= KINDA_SMALL_NUMBER)
    {
        return;
    }
    FlushPersistentDebugLines(GetWorld());

    // 获取包围盒的 Extent (半长/半宽)
    FVector Extent = BoundsComponent->GetScaledBoxExtent();

    // 【核心修改 1】：获取包含位置、旋转和缩放的完整 Transform
    FTransform ActorTransform = BoundsComponent->GetComponentTransform();

    int32 MaxRadius = FMath::CeilToInt((Extent.X + Extent.Y) / HexSize);

    for (int32 q = -MaxRadius; q <= MaxRadius; ++q)
    {
        int32 r1 = FMath::Max(-MaxRadius, -q - MaxRadius);
        int32 r2 = FMath::Min(MaxRadius, -q + MaxRadius);

        for (int32 r = r1; r <= r2; ++r)
        {
            FHexCoord CurrentCoord(q, r);
            FVector2D LocalPos2D = UHexMathLibrary::HexToLocalSpace(CurrentCoord, HexSize);

            // 在未旋转的局部空间中进行完美裁剪
            if (FMath::Abs(LocalPos2D.X) > Extent.X || FMath::Abs(LocalPos2D.Y) > Extent.Y)
            {
                continue;
            }

            // 构建 3D 局部中心点坐标 (Z=0)
            FVector LocalCenter(LocalPos2D.X, LocalPos2D.Y, 0.0f);

            // 【核心修改 2】：将局部坐标转换到世界坐标，使其跟随 Actor 旋转！
            // 使用 TransformPositionNoScale 确保六边形不会因为你缩放了包围盒而被拉伸变形
            FVector WorldPos = ActorTransform.TransformPositionNoScale(LocalCenter);

            FHexCellData CellData;
            CellData.Coordinate = CurrentCoord;
            CellData.WorldLocation = WorldPos;
            CellData.bStandable = true;

            GridData.Add(CurrentCoord, CellData);

            // 计算顶点并绘制
            FVector Corners[6];
            for (int32 i = 0; i < 6; ++i)
            {
                float AngleDeg = 60.0f * i + 30.0f;
                float AngleRad = FMath::DegreesToRadians(AngleDeg);

                // 先算出顶点相对于网格原点的局部坐标
                FVector LocalCornerPos = LocalCenter + FVector(HexSize * FMath::Cos(AngleRad), HexSize * FMath::Sin(AngleRad), 0.0f);

                // 【核心修改 3】：将顶点也转换到世界坐标
                Corners[i] = ActorTransform.TransformPositionNoScale(LocalCornerPos);
            }

            for (int32 i = 0; i < 6; ++i)
            {
                int32 NextIndex = (i + 1) % 6;
                DrawDebugLine(GetWorld(), Corners[i], Corners[NextIndex], FColor::Green, true, -1.0f, 0, 2.0f);
            }

            DrawDebugPoint(GetWorld(), WorldPos, 5.0f, FColor::Red, true);
            // 【修改为使用 3D 文本组件】
            FString CoordText = FString::Printf(TEXT("%d, %d"), CurrentCoord.Q, CurrentCoord.R);

            UTextRenderComponent* TextRender = NewObject<UTextRenderComponent>(this);
            TextRender->SetText(FText::FromString(CoordText));
            TextRender->SetWorldSize(30.0f); // 控制字的大小
            TextRender->SetTextRenderColor(FColor::White);
            TextRender->SetHorizontalAlignment(EHTA_Center);
            TextRender->SetVerticalAlignment(EVRTA_TextCenter);

            TextRender->SetupAttachment(RootComponent);

            TextRender->SetAbsolute(false, false, true);

            TextRender->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));

            TextRender->RegisterComponent();

            TextRender->SetWorldLocation(WorldPos + FVector(0.0f, 0.0f, 10.0f));
            SpawnedTexts.Add(TextRender);
        }
    }
}

void AHexGridGenerator::ClearGrid()
{
    GridData.Empty();
    if (GetWorld() && SpawnedTexts.Num() > 0)
    {
        FlushPersistentDebugLines(GetWorld());
    }
    for (UTextRenderComponent* Text : SpawnedTexts)
    {
        if (IsValid(Text))
        {
            Text->DestroyComponent();
        }
    }
    SpawnedTexts.Empty();
    for (UDecalComponent* Decal : SpawnedDecals)
    {
        if (IsValid(Decal))
        {
            Decal->DestroyComponent();
        }
    }
    SpawnedDecals.Empty();
    DecalsMap.Empty();
}

bool AHexGridGenerator::IsUnitPlacementBlocked(const FVector& GroundLocation, const FCollisionQueryParams& QueryParams) const
{
    if (!bCheckUnitPlacementCollision || UnitPlacementCheckRadius <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 在格子地面中心上方检测单位站位空间；如果球形范围碰到阻挡物，则单位站不下。
    const FVector CheckLocation = GroundLocation + FVector(0.0f, 0.0f, UnitPlacementCheckHeight);
    const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(UnitPlacementCheckRadius);

    return World->OverlapAnyTestByChannel(
        CheckLocation,
        FQuat::Identity,
        UnitPlacementTraceChannel,
        CollisionShape,
        QueryParams
    );
}

bool AHexGridGenerator::SetCellDecalColor(const FHexCoord& Coord, const FLinearColor& Color)
{
    UDecalComponent** Decal = DecalsMap.Find(Coord);
    if (!Decal || !IsValid(*Decal))
    {
        return false;
    }

    UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>((*Decal)->GetDecalMaterial());
    if (!DynamicMaterial)
    {
        DynamicMaterial = (*Decal)->CreateDynamicMaterialInstance();
    }

    if (!DynamicMaterial)
    {
        return false;
    }

    DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
    return true;
}

bool AHexGridGenerator::SetCellDecalEnableCenterCircle(const FHexCoord& Coord, bool bEnable)
{
    UDecalComponent** Decal = DecalsMap.Find(Coord);
    if (!Decal || !IsValid(*Decal))
    {
        return false;
    }

    UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>((*Decal)->GetDecalMaterial());
    if (!DynamicMaterial)
    {
        DynamicMaterial = (*Decal)->CreateDynamicMaterialInstance();
    }

    if (!DynamicMaterial)
    {
        return false;
    }

    DynamicMaterial->SetScalarParameterValue(TEXT("EnableCenterCircle"), bEnable ? 1.0f : 0.0f);
    return true;
}

bool AHexGridGenerator::GetCellAtWorldLocation(const FVector& WorldLocation, FHexCoord& OutCoord) const
{
    if (!BoundsComponent || HexSize <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FTransform BoundsTransform = BoundsComponent->GetComponentTransform();

    // 生成格子时用了 TransformPositionNoScale，这里要用对应的逆变换。
    const FVector LocalLocation = BoundsTransform.InverseTransformPositionNoScale(WorldLocation);

    const FHexCoord Coord = UHexMathLibrary::WorldToHex(
        FVector2D(LocalLocation.X, LocalLocation.Y),
        HexSize
    );

    if (!GridData.Contains(Coord))
    {
        return false;
    }

    OutCoord = Coord;
    return true;
}


bool AHexGridGenerator::BuildGridData(TMap<FHexCoord, FHexCellData>& OutGridData) const
{
    OutGridData.Empty();
    if (!GetWorld() || !BoundsComponent || !FMath::IsFinite(HexSize) || HexSize <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid generation requires a world, bounds and a positive finite HexSize."));
        return false;
    }

    const FVector Extent = BoundsComponent->GetScaledBoxExtent();
    const FTransform BoundsTransform = BoundsComponent->GetComponentTransform();
    const int32 MaxRadius = FMath::CeilToInt((Extent.X + Extent.Y) / HexSize);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    for (int32 q = -MaxRadius; q <= MaxRadius; ++q)
    {
        const int32 r1 = FMath::Max(-MaxRadius, -q - MaxRadius);
        const int32 r2 = FMath::Min(MaxRadius, -q + MaxRadius);

        for (int32 r = r1; r <= r2; ++r)
        {
            const FHexCoord CurrentCoord(q, r);
            const FVector2D LocalPos2D = UHexMathLibrary::HexToLocalSpace(CurrentCoord, HexSize);

            if (FMath::Abs(LocalPos2D.X) > Extent.X || FMath::Abs(LocalPos2D.Y) > Extent.Y)
            {
                continue;
            }

            const FVector LocalCenter(LocalPos2D.X, LocalPos2D.Y, 0.0f);
            const FVector WorldCenterBase = BoundsTransform.TransformPositionNoScale(LocalCenter);

            FHitResult CenterHit;
            const bool bHitCenter = GetWorld()->LineTraceSingleByChannel(
                CenterHit,
                WorldCenterBase + FVector(0.0f, 0.0f, TraceHeight),
                WorldCenterBase - FVector(0.0f, 0.0f, TraceHeight),
                TraceChannel,
                QueryParams
            );

            bool bTerrainPassable = bHitCenter;
            float HeightSum = bHitCenter ? CenterHit.Location.Z : WorldCenterBase.Z;
            int32 SimilarPointsCount = bHitCenter ? 1 : 0;

            if (bHitCenter)
            {
                for (int32 i = 0; i < 6; ++i)
                {
                    const float AngleRad = FMath::DegreesToRadians(60.0f * i + 30.0f);
                    const FVector CornerLocal = LocalCenter + FVector(HexSize * FMath::Cos(AngleRad), HexSize * FMath::Sin(AngleRad), 0.0f);
                    const FVector CornerWorldBase = BoundsTransform.TransformPositionNoScale(CornerLocal);

                    FHitResult CornerHit;
                    if (GetWorld()->LineTraceSingleByChannel(
                        CornerHit,
                        CornerWorldBase + FVector(0.0f, 0.0f, TraceHeight),
                        CornerWorldBase - FVector(0.0f, 0.0f, TraceHeight),
                        TraceChannel,
                        QueryParams))
                    {
                        const float HeightDiff = FMath::Abs(CornerHit.Location.Z - CenterHit.Location.Z);
                        if (HeightDiff <= HeightTolerance)
                        {
                            ++SimilarPointsCount;
                            HeightSum += CornerHit.Location.Z;
                        }
                    }
                }
            }

            if (!bHitCenter || SimilarPointsCount < (MinRequiredSimilarCorners + 1))
            {
                bTerrainPassable = false;
            }

            if (bHitCenter)
            {
                const float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(CenterHit.ImpactNormal, FVector::UpVector), -1.0, 1.0)));
                if (SlopeAngle > MaxWalkableSlopeAngle)
                {
                    bTerrainPassable = false;
                }
            }

            FHexCellData CellData;
            CellData.Coordinate = CurrentCoord;
            const float AverageZ = SimilarPointsCount > 0 ? HeightSum / static_cast<float>(SimilarPointsCount) : WorldCenterBase.Z;
            CellData.WorldLocation = bHitCenter
                ? FVector(CenterHit.Location.X, CenterHit.Location.Y, AverageZ + GroundOffset)
                : WorldCenterBase + FVector(0.0f, 0.0f, GroundOffset);
            const bool bPlacementBlocked = bTerrainPassable && IsUnitPlacementBlocked(CellData.WorldLocation, QueryParams);
            CellData.bStandable = bTerrainPassable && !bPlacementBlocked;

            OutGridData.Add(CurrentCoord, CellData);
        }
    }

    return true;
}

void AHexGridGenerator::GenerateRuntimeGrid()
{
    ClearGrid();
    if (!BuildGridData(GridData) || !DecalMaterial)
    {
        return;
    }

    const float GridYaw = BoundsComponent->GetComponentRotation().Yaw;
    for (const TPair<FHexCoord, FHexCellData>& CellPair : GridData)
    {
        UDecalComponent* Decal = NewObject<UDecalComponent>(this);
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(DecalMaterial, this);
        Material->SetVectorParameterValue(TEXT("Color"), DefaultDecalColor);
        Decal->SetDecalMaterial(Material);
        Decal->DecalSize = DecalSize;
        Decal->SetupAttachment(RootComponent);
        Decal->SetAbsolute(false, false, true);
        Decal->RegisterComponent();
        Decal->SetWorldLocation(CellPair.Value.WorldLocation);
        Decal->SetWorldRotation(FRotator(-90.0f, GridYaw + DecalYawOffset, 0.0f));
        SpawnedDecals.Add(Decal);
        DecalsMap.Add(CellPair.Key, Decal);
    }
}

void AHexGridGenerator::GenerateGrid()
{
    if (!TargetGridDataAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("GenerateGrid failed: TargetGridDataAsset is not set."));
        return;
    }

    TMap<FHexCoord, FHexCellData> GeneratedGridData;
    if (!BuildGridData(GeneratedGridData))
    {
        return;
    }
    ClearGrid();
    const FTransform BoundsTransform = BoundsComponent->GetComponentTransform();
    GridData = GeneratedGridData;

    TargetGridDataAsset->Modify();
    TargetGridDataAsset->GridData.Empty();
    TargetGridDataAsset->HexSize = HexSize;
    TargetGridDataAsset->GridYaw = BoundsTransform.GetRotation().Rotator().Yaw;
    TargetGridDataAsset->DecalYawOffset = DecalYawOffset;

    for (const TPair<FHexCoord, FHexCellData>& CellPair : GeneratedGridData)
    {
        const FHexCoord& Coord = CellPair.Key;
        TargetGridDataAsset->GridData.Add(FVector2D(Coord.Q, Coord.R), CellPair.Value);
    }

    TargetGridDataAsset->MarkPackageDirty();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GenerateGrid saved %d cells to %s."),
        TargetGridDataAsset->GridData.Num(),
        *TargetGridDataAsset->GetName()
    );
}
