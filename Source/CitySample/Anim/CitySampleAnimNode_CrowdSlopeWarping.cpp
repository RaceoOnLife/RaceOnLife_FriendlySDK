// Copyright Epic Games, Inc. All Rights Reserved.
#include "Anim/CitySampleAnimNode_CrowdSlopeWarping.h"

#include "Animation/AnimInstanceProxy.h"
#include "ReferenceSkeleton.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimNode_CrowdSlopeWarping"

DECLARE_CYCLE_STAT(TEXT("CITYSAMPLE_CrowdSlopeWarping_Eval"), STAT_CrowdSlopeWarping_Eval, STATGROUP_CITYSAMPLEANIM);
DECLARE_DWORD_COUNTER_STAT(TEXT("CITYSAMPLE_CrowdSlopeWarping_DidTrace"), STAT_MassCrowdAnim_DidTrace, STATGROUP_CITYSAMPLEANIM);
DECLARE_DWORD_COUNTER_STAT(TEXT("CITYSAMPLE_CrowdSlopeWarping_TotalTraces"), STAT_MassCrowdAnim_TotalTraces, STATGROUP_CITYSAMPLEANIM);

TAutoConsoleVariable<bool> CVarCrowdSlopeWarpingEnable(TEXT("a.AnimNode.CrowdSlopeWarping.Enable"), true, TEXT("Toggle Crowd Slope Warping"));
#if ENABLE_ANIM_DEBUG
TAutoConsoleVariable<bool> CVarCrowdSlopeWarpingDebug(TEXT("a.AnimNode.CrowdSlopeWarping.Debug"), false, TEXT("Toggle Crowd Slope Warping debugging"));
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FCitySampleAnimNode_CrowdSlopeWarping

void FCitySampleAnimNode_CrowdSlopeWarping::ResetDynamics(ETeleportType InTeleportType)
{
	bTeleported |= InTeleportType == ETeleportType::TeleportPhysics;
}

void FCitySampleAnimNode_CrowdSlopeWarping::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread);

	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);

	check(Context.AnimInstanceProxy && Context.AnimInstanceProxy->GetSkelMeshComponent());

	ResetDynamicData();
	CachedComponentZ = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetComponentToWorld().GetTranslation().Z;

	// Cache the curve UIDs
	const USkeleton* Skeleton = Context.AnimInstanceProxy->GetSkeleton();
	check(Skeleton);
	LeftFootData.FootPinningCurveUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, LeftFootData.FootPinningCurveName);
	RightFootData.FootPinningCurveUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, RightFootData.FootPinningCurveName);

	// Use the leg's length based off ref pose.
	const FReferenceSkeleton& RefSkeleton = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset()->GetRefSkeleton();
	LeftFootData.CachedLegLength = FindLegLength(LeftFootData, RefSkeleton);
	RightFootData.CachedLegLength = FindLegLength(RightFootData, RefSkeleton);

	WalkableFloorZ = FMath::Cos(FMath::DegreesToRadians(WalkableFloorAngle));
}

void FCitySampleAnimNode_CrowdSlopeWarping::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);
}

void FCitySampleAnimNode_CrowdSlopeWarping::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread);
	SCOPE_CYCLE_COUNTER(STAT_CrowdSlopeWarping_Eval);

	check(Output.AnimInstanceProxy && Output.AnimInstanceProxy->GetSkelMeshComponent());

	if (bTeleported)
	{
		bTeleported = false;
		ResetDynamicData();
		CachedComponentZ = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetComponentToWorld().GetTranslation().Z;
	}

	FCrowdSlopeWarpingEvaluationContext CrowdSlopeWarpingContext(Output);

	const FBoneContainer& RequiredBones = CrowdSlopeWarpingContext.CSPContext.Pose.GetPose().GetBoneContainer();
	const FTransform ComponentToWorld = CrowdSlopeWarpingContext.OwningComponent->GetComponentToWorld();

	// Subtract the instantaneous component Z frame delta from our springs to properly conserve their Z position when the capsule moves upwards/downwards.
	const float PrevCompZ = CachedComponentZ;
	CachedComponentZ = ComponentToWorld.GetTranslation().Z;
	const float InstantDeltaZ = CachedComponentZ - PrevCompZ;
	LeftFootData.FootOffsetSpringInterp.Current.Z -= InstantDeltaZ;
	RightFootData.FootOffsetSpringInterp.Current.Z -= InstantDeltaZ;
	PelvisSpringInterp.Current.Z -= InstantDeltaZ;

	FVector TargetPelvisOffset = FVector::ZeroVector;

	FTransform IKRootBoneTransform = Output.Pose.GetComponentSpaceTransform(IKFootRootBoneIndex);

	LeftFootData.BoneTransform = Output.Pose.GetComponentSpaceTransform(LeftFootData.IKFootBoneIndex);

	RightFootData.BoneTransform = Output.Pose.GetComponentSpaceTransform(RightFootData.IKFootBoneIndex);

	if(bDoTraces)
	{
		INC_DWORD_STAT(STAT_MassCrowdAnim_DidTrace);
		UpdateFootTrace(CrowdSlopeWarpingContext, LeftFootData);
		UpdateFootTrace(CrowdSlopeWarpingContext, RightFootData);
	}
	else
	{
		CorrectFootOnSkippedTrace(CrowdSlopeWarpingContext, LeftFootData, InstantDeltaZ);
		CorrectFootOnSkippedTrace(CrowdSlopeWarpingContext, RightFootData, InstantDeltaZ);
	}

	// If we're in the middle of a step Y adjustment, choose the best foot to move forward for the adjustment
	if(FMath::IsNearlyEqual(LeftFootData.FootOffsetSpringInterp.Current.Y, 0.0f) && FMath::IsNearlyEqual(LeftFootData.FootOffsetSpringInterp.Current.Y, 0.0f))
	{
		const FVector RightFootInitial = RightFootData.BoneTransform.GetTranslation();
		const FVector LeftFootInitial = LeftFootData.BoneTransform.GetTranslation();
		if ((RightFootInitial.Y - LeftFootInitial.Y) > 10.0f)
		{
			bRightFootForward = true;
		}
		else if ((LeftFootInitial.Y - RightFootInitial.Y ) > 10.0f)
		{
			bRightFootForward = false;
		}
	}

	FTransform PelvisBoneTransform = Output.Pose.GetComponentSpaceTransform(PelvisBoneIndex);

	if (bDoTraces)
	{
		// Find forward offsets for the feet, and adjust the pelvis for half of that.
		UpdateFeetStepAdjustment(CrowdSlopeWarpingContext, LeftFootData, !bRightFootForward);
		UpdateFeetStepAdjustment(CrowdSlopeWarpingContext, RightFootData, bRightFootForward);
	}

	TargetPelvisOffset.Y = 0.5f * (LeftFootData.DesiredOffset.Y + RightFootData.DesiredOffset.Y);


	// Use the lowest foot delta as our pelvis/footIKroot offset
	TargetPelvisOffset.Z = FMath::Min(LeftFootData.DesiredOffset.Z, RightFootData.DesiredOffset.Z);
	PelvisSpringInterp.Update(TargetPelvisOffset, CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->GetDeltaSeconds());
	
	PelvisBoneTransform.AddToTranslation(PelvisSpringInterp.Current);
	IKRootBoneTransform.AddToTranslation(PelvisSpringInterp.Current);

	UpdateFootPinning(CrowdSlopeWarpingContext, LeftFootData);
	UpdateFootPinning(CrowdSlopeWarpingContext, RightFootData);

	UpdateFootOffset(CrowdSlopeWarpingContext, LeftFootData, IKRootBoneTransform);
	UpdateFootOffset(CrowdSlopeWarpingContext, RightFootData, IKRootBoneTransform);

	OutBoneTransforms.Add(FBoneTransform(IKFootRootBoneIndex, IKRootBoneTransform));
	OutBoneTransforms.Add(FBoneTransform(LeftFootData.IKFootBoneIndex, LeftFootData.BoneTransform));
	OutBoneTransforms.Add(FBoneTransform(RightFootData.IKFootBoneIndex, RightFootData.BoneTransform));
	OutBoneTransforms.Add(FBoneTransform(PelvisBoneIndex, PelvisBoneTransform));
	// Sort OutBoneTransforms so indices are in increasing order.
	OutBoneTransforms.Sort(FCompareBoneTransformIndex());

#if ENABLE_ANIM_DEBUG
	const bool bShowDebug = (CVarCrowdSlopeWarpingDebug.GetValueOnAnyThread() == true);
	if(bShowDebug)
	{
		FTransform LeftFootDebugTransform = LeftFootData.BoneTransform;
		LeftFootDebugTransform.SetRotation(FRotator(0.0f, 90.0f, 0.0f).Quaternion());
		LeftFootDebugTransform.AddToTranslation(FVector(0.0f, 0.0f, -FeetRadius));
		LeftFootDebugTransform *= ComponentToWorld;
		CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugPlane(LeftFootDebugTransform, FeetRadius, LeftFootData.bCachedHit ? FColor::Green : FColor::Red, false, -1.0f, 1.0f, SDPG_Foreground);
	
		FTransform RightFootDebugTransform = RightFootData.BoneTransform;
		RightFootDebugTransform.SetRotation(FRotator(0.0f, 90.0f, 0.0f).Quaternion());
		RightFootDebugTransform.AddToTranslation(FVector(0.0f, 0.0f, -FeetRadius));
		RightFootDebugTransform *= ComponentToWorld;
		CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugPlane(RightFootDebugTransform, FeetRadius, RightFootData.bCachedHit ? FColor::Green : FColor::Red, false, -1.0f, 1.0f, SDPG_Foreground);
	
		FTransform PelvisDebugTransform = PelvisBoneTransform;
		PelvisDebugTransform.SetRotation(FRotator(0.0f, 90.0f, 0.0f).Quaternion());
		PelvisDebugTransform *= ComponentToWorld;
		CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugPlane(PelvisDebugTransform, 20.0f, FColor::Blue, false, -1.0f, 1.0f, SDPG_Foreground);
	}
#endif
}


void FCitySampleAnimNode_CrowdSlopeWarping::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData);
	ComponentPose.GatherDebugData(DebugData);}

void FCitySampleAnimNode_CrowdSlopeWarping::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	PelvisBone.Initialize(RequiredBones);
	PelvisBoneIndex = PelvisBone.GetCompactPoseIndex(RequiredBones);

	LeftFootData.Initialize(RequiredBones);
	RightFootData.Initialize(RequiredBones);

	IKFootRootBone.Initialize(RequiredBones);
	IKFootRootBoneIndex = IKFootRootBone.GetCompactPoseIndex(RequiredBones);
}

void FCitySampleAnimNode_CrowdSlopeWarping::UpdateFootTrace(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_CrowdSlopeWarping_ProcessFootTrace);

	const FTransform ComponentToWorld = CrowdSlopeWarpingContext.OwningComponent->GetComponentToWorld();
	FVector Start = FootData.BoneTransform.GetLocation() + TraceStartOffset;
	Start.Z += TraceLength;
	FVector End = Start;
	End.Z -= 2.0f * TraceLength;


	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	// Ignore self and all attached components
	QueryParams.AddIgnoredActor(CrowdSlopeWarpingContext.OwningActor);

	// Do the trace in world space
	Start = ComponentToWorld.TransformPosition(Start);
	End = ComponentToWorld.TransformPosition(End);
	FVector HitLocation = FVector::ZeroVector;
	FVector HitNormal = FVector::UpVector;
	FHitResult HitResult;
	const bool bHit = DoSphereTrace(CrowdSlopeWarpingContext, Start, End, HitResult, QueryParams);

	// Move results back to component space
	if (bHit)
	{
		HitNormal = HitResult.ImpactNormal;
		HitLocation = HitResult.ImpactPoint;

		HitLocation = ComponentToWorld.InverseTransformPosition(HitLocation);
		HitNormal = ComponentToWorld.InverseTransformVector(HitNormal);

		if (HitNormal.Z < WalkableFloorZ)
		{
			HitNormal = FVector::UpVector;
		}
	}

	FootData.DesiredOffset.Z = HitLocation.Z + GroundOffset;
	FootData.DesiredNormal = HitNormal;
	FootData.bCachedHit = bHit;

#if ENABLE_ANIM_DEBUG
	const bool bShowDebug = (CVarCrowdSlopeWarpingDebug.GetValueOnAnyThread() == true);
	if (bShowDebug)
	{
		CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugLine(Start, bHit ? ComponentToWorld.TransformPosition(HitLocation) : End, bHit ? FColor::Green : FColor::Red, false, -1.0f, 1.0f, SDPG_Foreground);

		const FTransform TraceDebugTransform = FTransform(FRotationMatrix::MakeFromZ(HitNormal).ToQuat(), HitLocation);
		CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugPlane(TraceDebugTransform * ComponentToWorld, FeetRadius, bHit ? FColor::Cyan : FColor::Magenta, false, -1.0f, 0.5f, SDPG_Foreground);
	}
#endif
}

void FCitySampleAnimNode_CrowdSlopeWarping::UpdateFootOffset(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData, const FTransform& IKRootTransform) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_CrowdSlopeWarping_ProcessFootOffset);

	const FTransform ComponentToWorld = CrowdSlopeWarpingContext.OwningComponent->GetComponentToWorld();

	FootData.FootNormalSpringInterp.Update(FootData.DesiredNormal, CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->GetDeltaSeconds());

	FVector ActualOffset = FootData.DesiredOffset;
	if (!FootData.bCachedHit)
	{
		ActualOffset.Z += PelvisSpringInterp.Current.Z;
	}

	FootData.FootOffsetSpringInterp.Update(ActualOffset, CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->GetDeltaSeconds());

	FootData.BoneTransform.AddToTranslation(FootData.FootOffsetSpringInterp.Current);
	const FQuat TargetQuat = AimTo(IKRootTransform, FVector::UpVector, FootData.FootNormalSpringInterp.Current);
	FootData.BoneTransform.SetRotation(TargetQuat * FootData.BoneTransform.GetRotation());

	if (bCheckForLegOverextension)
	{
		const FVector HipLocation = CrowdSlopeWarpingContext.CSPContext.Pose.GetComponentSpaceTransform(PelvisBoneIndex).GetLocation();

		const FVector IKFootLocationPreAdjustment = FootData.BoneTransform.GetLocation();
		const FVector AdjustedHipLocation = HipLocation + PelvisSpringInterp.Current;
		const FVector IKFootToAdjustedHip = AdjustedHipLocation - IKFootLocationPreAdjustment;
		const float DistanceToIKTarget = IKFootToAdjustedHip.Size();

		if (DistanceToIKTarget > FootData.CachedLegLength)
		{
			const FVector HipToIKFoot = -IKFootToAdjustedHip.GetSafeNormal();
			const FVector ClampedTranslation = AdjustedHipLocation + HipToIKFoot * FootData.CachedLegLength;
			FootData.BoneTransform.SetTranslation(ClampedTranslation);
			FootData.FootOffsetSpringInterp.Current.Z += ClampedTranslation.Z - IKFootLocationPreAdjustment.Z;
		}
	}
}

void FCitySampleAnimNode_CrowdSlopeWarping::UpdateFootPinning(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_CrowdSlopeWarping_ProcessFootPinning);

	const FTransform ComponentToWorld = CrowdSlopeWarpingContext.OwningComponent->GetComponentToWorld();

#if ENABLE_ANIM_DEBUG
	const bool bShowDebug = (CVarCrowdSlopeWarpingDebug.GetValueOnAnyThread() == true);
#endif

	if (FootData.FootPinningCurveUID != SmartName::MaxUID)
	{
		const float FootLockCurveValue = CrowdSlopeWarpingContext.CSPContext.Curve.Get(FootData.FootPinningCurveUID);
		const float PrevFootLockAlpha = FootData.FootPinningAlpha;

		// Only grab the value if we're beyond the threhold or dropping.
		if (FootLockCurveValue > FootPinningCurveAlphaThreshold || FootLockCurveValue < FootData.FootPinningAlpha)
		{
			FootData.FootPinningAlpha = FootLockCurveValue;
		}

		// If we don't have a pin target and the curve we're above the threshold, then pin in world space.
		if (!FootData.bHasPinTarget && FootData.FootPinningAlpha > FootPinningCurveAlphaThreshold)
		{
			const FTransform FootBallTransform = CrowdSlopeWarpingContext.CSPContext.Pose.GetComponentSpaceTransform(FootData.IKFootBallBoneIndex);
			FootData.FootPinningInitialLocation = ComponentToWorld.TransformPosition(FootBallTransform.GetTranslation());
			FootData.bHasPinTarget = true;
		}

		// If our curve is not beyond the threshold, we can pin again
		if (FootData.FootPinningAlpha < FootPinningCurveAlphaThreshold)
		{
			FootData.bHasPinTarget = false;
		}

		if (FootData.FootPinningAlpha > 0.0f)
		{
			// Get a world space delta between foot ball positions
			const FTransform FootBallTransform = CrowdSlopeWarpingContext.CSPContext.Pose.GetComponentSpaceTransform(FootData.IKFootBallBoneIndex);
			FVector FootBallTranslationDeltaWorld = FootData.FootPinningInitialLocation - ComponentToWorld.TransformPosition(FootBallTransform.GetTranslation());
			FootBallTranslationDeltaWorld.Z = 0.0f;

			// Move it back to component space
			const FVector FootOffset = ComponentToWorld.InverseTransformVector(FootBallTranslationDeltaWorld);

#if ENABLE_ANIM_DEBUG
			if (bShowDebug)
			{
				FTransform FootDebugTransform = FootData.BoneTransform;
				FootDebugTransform.SetRotation(FRotator(0.0f, 90.0f, 0.0f).Quaternion());
				FootDebugTransform *= ComponentToWorld;
				CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugPlane(FootDebugTransform, FeetRadius, FColor::Orange, false, -1.0f, 0.5f, SDPG_Foreground);
			}
#endif
			
			// Alpha it by our foot pinning curve value. This requires the curve to have a smooth "blend out" if we don't want to pop here.
			FootData.BoneTransform.AddToTranslation(FootOffset * FootData.FootPinningAlpha);
		}
	}
}

bool FCitySampleAnimNode_CrowdSlopeWarping::DoSphereTrace(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, const FVector& Start, const FVector& End, FHitResult& HitResult, const FCollisionQueryParams& QueryParams) const
{
	INC_DWORD_STAT(STAT_MassCrowdAnim_TotalTraces);

	bool bHit = false;

	if (CrowdSlopeWarpingContext.World == nullptr)
	{
		return false;
	}

	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FeetRadius);

	// similar to KismetTraceUtils.h -> ConfigureCollisionObjectParams
	TArray<ECollisionChannel> CollisionChannels;
	CollisionChannels.AddUninitialized(ObjectTypes.Num());

	for (int Index = 0; Index < ObjectTypes.Num(); Index++)
	{
		CollisionChannels[Index] = UEngineTypes::ConvertToCollisionChannel(ObjectTypes[Index]);
	}

	FCollisionObjectQueryParams ObjectParams;
	for (const ECollisionChannel& Channel : CollisionChannels)
	{
		if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
	}

	bHit = CrowdSlopeWarpingContext.World->SweepSingleByObjectType(HitResult, Start, End, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);

	return bHit && !HitResult.bStartPenetrating;
}

FQuat FCitySampleAnimNode_CrowdSlopeWarping::AimTo(const FTransform& InputTransform, FVector Axis, FVector Target)
{
	if (!Target.IsNearlyZero() && !Axis.IsNearlyZero())
	{
		Target = Target.GetUnsafeNormal();
		Axis = InputTransform.TransformVectorNoScale(Axis).GetSafeNormal();
		const FQuat Rotation = FQuat::FindBetweenNormals(Axis, Target);

		return (Rotation * InputTransform.GetRotation()).GetNormalized();
	}

	return InputTransform.GetRotation();
}

float FCitySampleAnimNode_CrowdSlopeWarping::FindLegLength(const FCitySample_CrowdSlopeWarpingFootData& FootData, const FReferenceSkeleton& RefSkeleton) const
{
	const int32 PelvisIndex = RefSkeleton.FindBoneIndex(PelvisBone.BoneName);
	const int32 LegIndex = RefSkeleton.FindBoneIndex(FootData.FKFootBone.BoneName);

	if((PelvisIndex != INDEX_NONE) && (LegIndex != INDEX_NONE))
	{
		return DesiredMaxLegLengthPercentageFromRefPose * 
		(RefSkeleton.GetRefBonePose()[PelvisIndex].GetTranslation() - RefSkeleton.GetRefBonePose()[LegIndex].GetTranslation()).Size();
	}

	return 0.0f;
}

void FCitySampleAnimNode_CrowdSlopeWarping::ResetDynamicData()
{
	PelvisSpringInterp.SpringState.Reset();
	LeftFootData.Reset();
	RightFootData.Reset();

	bRightFootForward = FMath::RandBool();
}

void FCitySampleAnimNode_CrowdSlopeWarping::UpdateFeetStepAdjustment(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData, const bool bMoveForward)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_CrowdSlopeWarping_AdjustHangingFeet);

	FootData.DesiredOffset.Y = 0.0f;

	if (!bDoForwardStepAdjustment)
	{
		return;
	}

	const FTransform ComponentToWorld = CrowdSlopeWarpingContext.OwningComponent->GetComponentToWorld();
	const FVector BallLocation = CrowdSlopeWarpingContext.CSPContext.Pose.GetComponentSpaceTransform(FootData.IKFootBallBoneIndex).GetLocation();
	const FVector FootLocation = FootData.BoneTransform.GetTranslation();

	FVector Start = BallLocation + TraceStartOffset;
	Start.Z += TraceLength;
	Start.Y += FeetRadius;
	FVector End = Start;
	End.Z -= 2.0f * TraceLength;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	// Ignore self and all attached components
	QueryParams.AddIgnoredActor(CrowdSlopeWarpingContext.OwningActor);

	// Do the trace in world space
	Start = ComponentToWorld.TransformPosition(Start);
	End = ComponentToWorld.TransformPosition(End);
	FVector HitLocation = FVector::ZeroVector;
	FVector HitNormal = FVector::UpVector;

	FHitResult HitResult;
	bool bHit = DoSphereTrace(CrowdSlopeWarpingContext, Start, End, HitResult, QueryParams);

#if ENABLE_ANIM_DEBUG
	const bool bShowDebug = (CVarCrowdSlopeWarpingDebug.GetValueOnAnyThread() == true);
	if (bShowDebug)
	{
		CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugSphere(HitResult.Location, FeetRadius, 8, FColor::Yellow, false, -1.0f, -0.5f, SDPG_Foreground);
	}
#endif

	if (bHit)
	{
		HitNormal = HitResult.ImpactNormal;
		HitLocation = HitResult.ImpactPoint;

		// Move results back to component space
		HitLocation = ComponentToWorld.InverseTransformPosition(HitLocation);
		HitNormal = ComponentToWorld.InverseTransformVector(HitNormal);

		// Only adjust the feet if we'd actually step up or down
		if (FMath::Abs((FootData.DesiredOffset.Z - (HitLocation.Z + GroundOffset))) > MinStepAdjustHeight)
		{
			const bool bStepUp = (HitLocation.Z + GroundOffset) > FootData.DesiredOffset.Z;
			if (bStepUp)
			{
				End = HitLocation;
				Start = FootLocation;
				Start.Z = FootData.DesiredOffset.Z + FeetRadius;

				End.Z -= FeetRadius;
			}
			else
			{
				End = FootLocation;
				Start = BallLocation;
				Start.Z = HitLocation.Z / 2.0f;

				End.Y -= FeetRadius;
				End.Z -= FeetRadius;
			}

			Start = ComponentToWorld.TransformPosition(Start);
			End = ComponentToWorld.TransformPosition(End);

			if (bMoveForward)
			{
				FootData.DesiredOffset.Z = HitLocation.Z + GroundOffset;
				FootData.DesiredNormal = HitNormal;
			}

#if ENABLE_ANIM_DEBUG
			if (bShowDebug)
			{
				CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugLine(Start, End, FColor::Green, false, -1.0f, 0.2f, SDPG_Foreground);
			}
#endif
			bHit = DoSphereTrace(CrowdSlopeWarpingContext, Start, End, HitResult, QueryParams);

			if(bHit)
			{
				HitNormal = HitResult.ImpactNormal;
				HitLocation = HitResult.ImpactPoint;

#if ENABLE_ANIM_DEBUG
				if (bShowDebug)
				{
					CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugSphere(HitResult.Location, FeetRadius, 8, FColor::Orange, false, -1.0f, 0.2f, SDPG_Foreground);
					CrowdSlopeWarpingContext.CSPContext.AnimInstanceProxy->AnimDrawDebugLine(Start, HitLocation, FColor::Orange, false, -1.0f, 0.5f, SDPG_Foreground);
				}
#endif

				HitLocation = ComponentToWorld.InverseTransformPosition(HitLocation);
				HitNormal = ComponentToWorld.InverseTransformVector(HitNormal);

				if (bStepUp)
				{
					const float TraceDelta = HitLocation.Y - FootLocation.Y;
					const float OutOffset = TraceDelta - (bMoveForward ? FeetRadius : (BallLocation.Y - FootLocation.Y + 2.0f * FeetRadius));
					FootData.DesiredOffset.Y = OutOffset;
				}
				else
				{
					const float OutOffset = (HitLocation.Y - FootLocation.Y + 2.0f * FeetRadius * (bMoveForward ? 1.0f : -1.0f));
					FootData.DesiredOffset.Y = OutOffset;
				}
			}
		}
	}
}

void FCitySampleAnimNode_CrowdSlopeWarping::CorrectFootOnSkippedTrace(FCrowdSlopeWarpingEvaluationContext& CrowdSlopeWarpingContext, FCitySample_CrowdSlopeWarpingFootData& FootData, const float ComponentZDelta)
{
	// If we are not tracing this frame, assume that if the capsule is going up or down, we want don't want to exceed this new maximum/minimum
	// This won't work when the leg is already behind the capsule's movement 
	if (ComponentZDelta > 0.0f)
	{
		FootData.DesiredOffset.Z = FMath::Max(FootData.DesiredOffset.Z - ComponentZDelta, 0.0f);
	}
	else if (ComponentZDelta < 0.0f)
	{
		FootData.DesiredOffset.Z = FMath::Min(FootData.DesiredOffset.Z - ComponentZDelta, 0.0f);
	}
}

bool FCitySampleAnimNode_CrowdSlopeWarping::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return 
		CVarCrowdSlopeWarpingEnable.GetValueOnAnyThread() &&
		PelvisBone.GetCompactPoseIndex(RequiredBones) != INDEX_NONE &&
		LeftFootData.IKFootBone.GetCompactPoseIndex(RequiredBones) != INDEX_NONE &&
		RightFootData.IKFootBone.GetCompactPoseIndex(RequiredBones) != INDEX_NONE &&
		IKFootRootBone.GetCompactPoseIndex(RequiredBones) != INDEX_NONE;
}

float FCitySample_FloatSpringInterp::Update(float Target, float DeltaTime)
{
	// Treat the input as a frequency in Hz
	const float AngularFrequency = Strength * 2.0f * PI;
	const float Stiffness = AngularFrequency * AngularFrequency;
	float AdjustedTarget = Target;
	if (!FMath::IsNearlyZero(Stiffness))
	{
		AdjustedTarget += Force / (Stiffness);
	}
	else
	{
		SpringState.Velocity += Force * (DeltaTime);
	}
	Current = UKismetMathLibrary::FloatSpringInterp(
		Current, AdjustedTarget, SpringState, Stiffness, CriticalDamping,
		DeltaTime, 1.0f, TargetVelocityAmount,
		false, 0.0f, 0.0f, bInitializeFromTarget);
	Velocity = SpringState.Velocity;
	return Current;
}

FVector FCitySample_VectorSpringInterp::Update(FVector Target, float DeltaTime)
{
	// Treat the input as a frequency in Hz
	const float AngularFrequency = Strength * 2.0f * PI;
	const float Stiffness = AngularFrequency * AngularFrequency;
	FVector AdjustedTarget = Target;
	if (!FMath::IsNearlyZero(Stiffness))
	{
		AdjustedTarget += Force / (Stiffness);
	}
	else
	{
		SpringState.Velocity += Force * (DeltaTime);
	}
	Current = UKismetMathLibrary::VectorSpringInterp(
		Current, AdjustedTarget, SpringState, Stiffness, CriticalDamping,
		DeltaTime, 1.0f, TargetVelocityAmount,
		false, FVector(), FVector(), bInitializeFromTarget);
	Velocity = SpringState.Velocity;
	return Current;
}

FCrowdSlopeWarpingEvaluationContext::FCrowdSlopeWarpingEvaluationContext(FComponentSpacePoseContext& CSPContextIn)
	: CSPContext(CSPContextIn)
{
	OwningComponent = CSPContextIn.AnimInstanceProxy->GetSkelMeshComponent();
	World = OwningComponent->GetWorld();
	OwningActor = OwningComponent->GetOwner();
}

#undef LOCTEXT_NAMESPACE

void FCitySample_CrowdSlopeWarpingFootData::Initialize(const FBoneContainer& RequiredBones)
{
	IKFootBone.Initialize(RequiredBones);
	FKFootBone.Initialize(RequiredBones);
	IKFootBallBone.Initialize(RequiredBones);
	IKFootBoneIndex = IKFootBone.GetCompactPoseIndex(RequiredBones);
	FKFootBoneIndex = FKFootBone.GetCompactPoseIndex(RequiredBones);
	IKFootBallBoneIndex = IKFootBallBone.GetCompactPoseIndex(RequiredBones);
}
