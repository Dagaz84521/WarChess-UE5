#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Grid/HexCoord.h"
#include "Grid/HexGridDataAsset.h"
#include "HexGridGenerator.generated.h"

class UBoxComponent;
class UDecalComponent;
class UMaterialInterface;
class UTextRenderComponent;

UCLASS()
class WARCHESS_API AHexGridGenerator : public AActor
{
	GENERATED_BODY()

public:
	AHexGridGenerator();


	// 编辑器操作
	UFUNCTION(CallInEditor, Category = "Hex")
	void PreviewGridWires();

	UFUNCTION(CallInEditor, Category = "Hex")
	void GenerateRuntimeGrid();

	UFUNCTION(CallInEditor, Category = "Grid Generation")
	void GenerateGrid();

	UFUNCTION(CallInEditor, Category = "Hex")
	void ClearGrid();

	// 颜色设置
	UFUNCTION(BlueprintCallable, Category = "Hex")
	bool SetCellDecalColor(const FHexCoord& Coord, const FLinearColor& Color);
	UFUNCTION(BlueprintCallable, Category = "Hex")
	bool SetCellDecalEnableCenterCircle(const FHexCoord& Coord, bool bEnable);
	// 重置颜色
	UFUNCTION(BlueprintCallable, Category = "Hex")
	bool ResetCellDecalColor(const FHexCoord& Coord) { return SetCellDecalColor(Coord, DefaultDecalColor); }
	UFUNCTION(BlueprintCallable, Category = "Hex")
	bool GetCellAtWorldLocation(const FVector& WorldLocation, FHexCoord& OutCoord) const;

	// 组件
	// 划定生成范围的包围盒。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex")
	UBoxComponent* BoundsComponent;

	// 网格设置
	// 六边形大小，表示中心到顶点的距离。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex", meta = (ClampMin = "1.0"))
	float HexSize = 100.0f;

	// 当前区域内生成的有效格子。
	UPROPERTY(VisibleAnywhere, Category = "Hex")
	TMap<FHexCoord, FHexCellData> GridData;

	// 地面检测设置
	// 射线检测起始高度，相对于格子中心上下偏移。
	UPROPERTY(EditAnywhere, Category = "Hex|Trace")
	float TraceHeight = 1000.0f;

	// 最大允许通行坡度，超过该角度视为不可通行。
	UPROPERTY(EditAnywhere, Category = "Hex|Trace")
	float MaxWalkableSlopeAngle = 45.0f;

	// 用于检测地形的碰撞通道。
	UPROPERTY(EditAnywhere, Category = "Hex|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 顶点与中心点高度差在此范围内视为相似。
	UPROPERTY(EditAnywhere, Category = "Hex|Trace")
	float HeightTolerance = 20.0f;

	// 至少需要多少个六边形顶点与中心点高度相似。
	UPROPERTY(EditAnywhere, Category = "Hex|Trace")
	int32 MinRequiredSimilarCorners = 3;

	// 碰撞设置
	// 生成格子时，在地面上方做一次球形占位检测；检测到阻挡时该格子会被标记为不可通行。
	UPROPERTY(EditAnywhere, Category = "Hex|Collision")
	bool bCheckUnitPlacementCollision = true;

	// 球形占位检测的半径，通常对应单位胶囊体半径或略小一点的站位半径。
	UPROPERTY(EditAnywhere, Category = "Hex|Collision", meta = (ClampMin = "0.0"))
	float UnitPlacementCheckRadius = 40.0f;

	// 球心相对格子地面中心的高度。默认使用单位半高附近，避免球体碰到地面自身。
	UPROPERTY(EditAnywhere, Category = "Hex|Collision")
	float UnitPlacementCheckHeight = 90.0f;

	// 用于站位检测的碰撞通道；场景阻挡物需要对该通道返回 Block/Overlap 才会被判定为占用。
	UPROPERTY(EditAnywhere, Category = "Hex|Collision")
	TEnumAsByte<ECollisionChannel> UnitPlacementTraceChannel = ECC_Pawn;

	// 稍微抬高生成位置，减少贴近地面导致的误判或摩擦。
	UPROPERTY(EditAnywhere, Category = "Hex|Collision")
	float GroundOffset = 5.0f;

	// 可视化设置
	UPROPERTY(EditAnywhere, Category = "Hex|Visual")
	UMaterialInterface* DecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Hex|Visual")
	FVector DecalSize = FVector(100.0f, 100.0f, 100.0f);

	UPROPERTY(EditAnywhere, Category = "Hex|Visual")
	float DecalYawOffset = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Grid Generation")
	UHexGridDataAsset* TargetGridDataAsset;
private:
	bool BuildGridData(TMap<FHexCoord, FHexCellData>& OutGridData) const;

	UPROPERTY(Transient)
	TArray<UTextRenderComponent*> SpawnedTexts;

	UPROPERTY()
	TArray<UDecalComponent*> SpawnedDecals;

	UPROPERTY()
	TMap<FHexCoord, UDecalComponent*> DecalsMap; // 方便后续管理贴花

	UPROPERTY()
	FLinearColor DefaultDecalColor = FLinearColor::Black;

	bool IsUnitPlacementBlocked(const FVector& GroundLocation, const FCollisionQueryParams& QueryParams) const;
};
