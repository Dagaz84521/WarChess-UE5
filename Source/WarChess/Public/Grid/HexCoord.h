#pragma once
#include "CoreMinimal.h"
#include "HexCoord.generated.h"

USTRUCT(BlueprintType)
struct WARCHESS_API FHexCoord
{
	GENERATED_BODY()

	// 轴向坐标的两个核心维度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex")
	int32 Q;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex")
	int32 R;

	// 构造函数
	FHexCoord() : Q(0), R(0) {}
	FHexCoord(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

	// 获取隐式的第三个坐标轴 S
	int32 GetS() const { return -Q - R; }

	// 重载判等运算符，方便放入 TSet 或 TMap 作为哈希键值
	bool operator==(const FHexCoord& Other) const
	{
		return Q == Other.Q && R == Other.R;
	}

	FHexCoord operator+(const FHexCoord& Other) const
	{
		return FHexCoord(Q + Other.Q, R + Other.R);
	}

	FHexCoord operator-(const FHexCoord& Other) const
	{
		return FHexCoord(Q - Other.Q, R - Other.R);
	}

};

USTRUCT(BlueprintType)
struct WARCHESS_API FHexCellData
{
	GENERATED_BODY()

	// 逻辑坐标
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex")
	FHexCoord Coordinate;

	// 真实世界的三维坐标（中心点）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex")
	FVector WorldLocation;

	// 基础地图条件，不包含单位占位或单位专属的移动规则。
	// 生成器提供初值，策划可在地图资产中调整；战斗期间由 Board 只读持有。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex")
	bool bStandable = false;

	FHexCellData() : WorldLocation(FVector::ZeroVector) {}
};

// 为 FHexCoord 实现全局哈希函数，这是在 UE 中使用 TMap/TSet 必须的一步
FORCEINLINE uint32 GetTypeHash(const FHexCoord& Hex)
{
	return HashCombine(GetTypeHash(Hex.Q), GetTypeHash(Hex.R));
}
