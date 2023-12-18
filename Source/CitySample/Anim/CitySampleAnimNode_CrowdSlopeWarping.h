// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNodeMessages.h"
#include "Kismet/KismetMathLibrary.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "CitySampleAnimNode_CrowdSlopeWarping.generated.h"

DECLARE_STATS_GROUP(TEXT("CitySampleAnim"), STATGROUP_CITYSAMPLEANIM, STATCAT_Advanced)

USTRUCT()
struct FCitySample_VectorSpringInterp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Strength = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float TargetVelocityAmount = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float CriticalDamping = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FVector Force = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FVector Current = FVector::ZeroVector;

	UPROPERTY(transient)
	FVectorSpringState SpringState = FVectorSpringState();

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInitializeFromTarget = true;

	FVector Update(FVector Target, float DeltaTime);
};

USTRUCT()
struct FCitySample_FloatSpringInterp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Strength = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float TargetVelocityAmount = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float CriticalDamping = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Force = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Velocity = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float Current = 0.0f;

	UPROPERTY(transient)
	FFloatSpringState SpringState = FFloatSpringState();

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bInitializeFromTarget = true;

	float Update(float Target, float DeltaTime);
};

USTRUCT()
struct FCitySample_CrowdSlopeWarpingFootData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FBoneReference IKFootBone;

	UPROPERTY(EditAnywhere, Category = Settings)
	FBoneReference FKFootBone;

	UPROPERTY(EditAnywhere, Category = Settings)
	FBoneReference IKFootBallBone;

	UPROPERTY(EditAnywhere, Category = Settings)
	FName FootPinningCurveName;
	SmartName::UID_Type FootPinningCurveUID;

	UPROPERTY(EditAnywhere, Category = Settings)
	FCitySample_VectorSpringInterp FootOffsetSpringInterp;

	UPROPERTY(EditAnywhere, Category = Settings)
	FCitySample_VectorSpringInterp FootNormalSpringInterp;

	FCompactPoseBoneIndex IKFootBoneIndex;
	FCompactPoseBoneIndex FKFootBoneIndex;
	FCompactPoseBoneIndex IKFootBallBoneIndex;

	FTransform BoneTransform = FTransform::Identity;
	FVector DesiredOffset = FVector::ZeroVector;
	FVector DesiredNormal = FVector::UpVector;

	FVector FootPinningInitialLocation = FVector::ZeroVector;

	float FootPinningAlpha = 0.0f;
	float CachedLegLength = 0.0f;
	bool bCachedHit = false;
	bool bHasPinTarget = false;

	void Initialize(const FBoneContainer& RequiredBones);

	void Reset()
	{
		FootOffsetSpringInterp.SpringState.Reset();
		FootNormalSpringInterp.SpringState.Reset();
		FootOffsetSpringInterp.Current = FVector::ZeroVector;
		FootNormalSpringInterp.Current = FVector::UpVector;
		DesiredNormal =  FVector::UpVector;
		DesiredOffset = FVector::ZeroVector;
		FootPinningAlpha = 0.0f;
		bCachedHit = false;
		bHasPinTarget = false;
	}

	FCitySample_CrowdSlopeWarpingFootData()
		: IKFootBoneIndex(INDEX_NONE)
		, FKFootBoneIndex(INDEX_NONE)
		, IKFootBallBoneIndex(INDEX_NONE)
	{}
};

struct FCrowdSlopeWarpingEvaluationContext
{
	FComponentSpacePoseContext& CSPContext;
	class UWorld* World = nullptr;
	class AActor* OwningActor = nullptr;
	class USkeletalMeshComponent* OwningComponent = nullptr;

	FCrowdSlopeWarpingEvaluationContext(FComponentSpacePoseContext& Output);
};

USTRUCT()
struct CITYSAMPLE_API FCitySampleAnimNode_CrowdSlopeWarping : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = Pelvis)
	FBoneReference PelvisBone;
	FCompactPoseBoneIndex PelvisBoneIndex;

	UPROPERTY(EditAnywhere, Category = Pelvis)
	FCitySample_VectorSpringInterp PelvisSpringInterp;

	UPROPERTY(EditAnywhere, Category = Feet)
	FCitySample_CrowdSlopeWarpingFootData LeftFootData;

	UPROPERTY(EditAnywhere, Category = Feet)
	FCitySample_CrowdSlopeWarpingFootData RightFootData;

	UPROPERTY(EditAnywhere, Category = Feet)
	FBoneReference IKFootRootBone;
	FCompactPoseBoneIndex IKFootRootBoneIndex;

	UPROPERTY(EditAnywhere, Category = Trace)
	float FeetRadius = 8.0f;

	UPROPERTY(EditAnywhere, Category = Trace)
	FVector TraceStartOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Trace)
	float GroundOffset = 1.0f;

	UPROPERTY(EditAnywhere, Category = Trace)
	float TraceLength = 50.0f;

	UPROPERTY(EditAnywhere, Category = FootPinning)
	float FootPinningCurveAlphaThreshold = 0.99f;

	// Legs may over-extend a bit during foot pinning. This is sometimes desirable in CitySample. (@Caleb.Longmire)
	UPROPERTY(EditAnywhere, Category = Feet, meta=(EditCondition=bCheckForLegOverextension, ClampMin = 0.1f))
	float DesiredMaxLegLengthPercentageFromRefPose = 1.1f;

	UPROPERTY(EditAnywhere, Category = Feet, meta = (ClampMin = 0.0f, ClampMax = 90.0f))
	float MinStepAdjustHeight = 10.0f;

	UPROPERTY(EditAnywhere, Category = Feet, meta=(ClampMin = 0.0f, ClampMax = 90.0f))
	float WalkableFloorAngle = 50.0f;

	UPROPERTY(EditAnywhere, Category = Feet)
	bool bCheckForLegOverextension = true;

	UPROPERTY(EditAnywhere, Category = Feet, meta = (PinShownByDefault))
	bool bDoForwardStepAdjustment = true;

	UPROPERTY(EditAnywhere, Category = Trace, meta = (PinShownByDefault))
	bool bDoTraces = true;

	UPROPERTY(EditAnywhere, Category = Trace)
	/** The types of objects that this trace can hit */
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	virtual void ResetDynamics(ETeleportType InTeleportType) override;
	virtual bool NeedsDynamicReset() const override { return true; };

public: // FAnimNode_Base
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
private:
	
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	void UpdateFootTrace(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingEvaluationContext, FCitySample_CrowdSlopeWarpingFootData& FootData) const;
	void UpdateFootOffset(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingEvaluationContext, FCitySample_CrowdSlopeWarpingFootData& FootData, const FTransform& IKRootTransform) const;
	void UpdateFootPinning(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData) const;
	bool DoSphereTrace(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingEvaluationContext, const FVector& Start, const FVector& End, FHitResult& HitResult, const FCollisionQueryParams& QueryParams) const;
	static FQuat AimTo(const FTransform& InputTransform, FVector Axis, FVector Target);
	float FindLegLength(const FCitySample_CrowdSlopeWarpingFootData& FootData, const FReferenceSkeleton& RefSkeleton) const;
	void ResetDynamicData();
	void UpdateFeetStepAdjustment(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData, const bool bMoveForward);
	void CorrectFootOnSkippedTrace(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData, const float ComponentZDelta);

	float WalkableFloorZ = 0.0f;
	float CachedComponentZ = 0.0f;
	bool bTeleported = false;
	bool bRightFootForward = false;
protected:
	bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

public:
	FCitySampleAnimNode_CrowdSlopeWarping()
		: PelvisBoneIndex(INDEX_NONE)
		, IKFootRootBoneIndex(INDEX_NONE)
	{
	}
};

