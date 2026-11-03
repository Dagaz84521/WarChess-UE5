#include "Grid/HexMathLibrary.h"

FVector2D UHexMathLibrary::HexToLocalSpace(const FHexCoord& Hex, float HexSize)
{
    const float SQRT3 = FMath::Sqrt(3.0f);
    float x = HexSize * SQRT3 * (Hex.Q + Hex.R / 2.0f);
    float y = HexSize * 1.5f * Hex.R;
    return FVector2D(x, y);
}

FHexCoord UHexMathLibrary::WorldToHex(const FVector2D& Point, float HexSize)
{
    if (!FMath::IsFinite(HexSize) || HexSize <= KINDA_SMALL_NUMBER)
    {
        return FHexCoord();
    }

    const float SQRT3 = FMath::Sqrt(3.0f);
    float q = (SQRT3 / 3.0f * Point.X - 1.0f / 3.0f * Point.Y) / HexSize;
    float r = (2.0f / 3.0f * Point.Y) / HexSize;

    float s = -q - r;

    int32 rx = FMath::RoundToInt(q);
    int32 ry = FMath::RoundToInt(r);
    int32 rz = FMath::RoundToInt(s);

    float xDiff = FMath::Abs(rx - q);
    float yDiff = FMath::Abs(ry - r);
    float zDiff = FMath::Abs(rz - s);

    if (xDiff > yDiff && xDiff > zDiff)
    {
        rx = -ry - rz;
    }
    else if (yDiff > zDiff)
    {
        ry = -rx - rz;
    }

    return FHexCoord(rx, ry);
}
