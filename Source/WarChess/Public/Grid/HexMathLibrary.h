#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Grid/HexCoord.h"
#include "HexMathLibrary.generated.h"

// Pointy-top axial coordinate conversions used by grid generation.
UCLASS()
class WARCHESS_API UHexMathLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Hex|Math")
    static FVector2D HexToLocalSpace(const FHexCoord& Hex, float HexSize);

    // Point is in the grid's unscaled local XY space.
    UFUNCTION(BlueprintPure, Category = "Hex|Math")
    static FHexCoord WorldToHex(const FVector2D& Point, float HexSize);
};
