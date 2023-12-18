// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_CitySampleSimpleWarp.generated.h"

/** 
 * Custom version of the built-in Simple Warp with a few new options
 * Eventually we will update the built-in but we need to find a better way to expose these options
*/
UCLASS(meta = (DisplayName = "CitySample Simple Warp"))
class URootMotionModifier_CitySampleSimpleWarp : public URootMotionModifier_Warp
{
	GENERATED_BODY()

public:

	/** When true, translation on the horizontal plane will remain intact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (DisplayAfter = "bWarpTranslation", EditCondition = "bWarpTranslation"))
	bool bIgnoreXYAxes = false;

	/** When total translation on the Z axis is below this value we will linearly interpolate it to reach the target. Only relevant when bIgnoreZAxis is false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (DisplayAfter = "WarpRotationTimeMultiplier", EditCondition = "bAllowAdditiveTranslationZAxis"))
	float WarpZTolerance = 1.f;

	/** When true, translation will we linearly interpolated instead of warped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bLerpTranslation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (DisplayAfter = "bWarpRotation", EditCondition = "bWarpRotation"))
	bool bIgnoreYaw = false;

	URootMotionModifier_CitySampleSimpleWarp(const FObjectInitializer& ObjectInitializer);

	virtual FTransform ProcessRootMotion(const FTransform& InRootMotion, float DeltaSeconds) override;
	virtual void OnTargetTransformChanged() override;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	void PrintLog(const FTransform& OriginalRootMotion, const FTransform& WarpedRootMotion) const;
#endif

protected:

	UPROPERTY()
	FTransform CachedRootMotion;

public:

	/** Wrapper function to add warping targets to reduce friction during merges while using the old motion warping API */
	UFUNCTION(BlueprintCallable, Category = "Motion Warping", meta = (DisplayName = "Add Or Update Warp Target"))
	static void BP_AddOrUpdateWarpTarget(class UMotionWarpingComponent* MotionWarpingComp, FName Name, FVector Location, FRotator Rotation);
};