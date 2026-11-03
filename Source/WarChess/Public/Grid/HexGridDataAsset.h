// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/HexCoord.h"
#include "HexGridDataAsset.generated.h"

/**
 *
 */
UCLASS()
class WARCHESS_API UHexGridDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FVector2D, FHexCellData> GridData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HexSize = 100.f; // 六边形的边长，默认值为100

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GridYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DecalYawOffset = 0.0f;
};
