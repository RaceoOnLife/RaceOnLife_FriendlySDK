// Copyright Epic Games, Inc. All Rights Reserved.

#include "RootMotionModifier_CitySampleSimpleWarp.h"
#include "GameFramework/Character.h"
#include "MotionWarpingComponent.h"

URootMotionModifier_CitySampleSimpleWarp::URootMotionModifier_CitySampleSimpleWarp(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URootMotionModifier_CitySampleSimpleWarp::OnTargetTransformChanged()
{
	if (const ACharacter* CharacterOwner = GetCharacterOwner())
	{
		ActualStartTime = PreviousPosition;
		StartTransform = CharacterOwner->GetActorTransform();
		CachedRootMotion = UMotionWarpingUtilities::ExtractRootMotionFromAnimation(Animation.Get(), StartTime, EndTime);
	}
}

FTransform URootMotionModifier_CitySampleSimpleWarp::ProcessRootMotion(const FTransform& InRootMotion, float DeltaSeconds)
{
	const ACharacter* CharacterOwner = GetCharacterOwner();
	if (CharacterOwner == nullptr)
	{
		return InRootMotion;
	}

	const FTransform& CharacterTransform = CharacterOwner->GetActorTransform();

	FTransform FinalRootMotion = InRootMotion;

	const FTransform RootMotionTotal = UMotionWarpingUtilities::ExtractRootMotionFromAnimation(Animation.Get(), PreviousPosition, EndTime);

	if(bLerpTranslation)
	{
		const FVector StartLocation = StartTransform.GetLocation();
		const FVector CurrentLocation = CharacterTransform.GetLocation();
		const FVector TargetLocation = GetTargetLocation();

		const float Alpha = (FMath::Min(CurrentPosition, EndTime) - StartTime) / (EndTime - StartTime);
		const FVector TargetLocThisFrame = FMath::Lerp<FVector, float>(StartLocation, TargetLocation, Alpha);

		FinalRootMotion.SetTranslation((TargetLocThisFrame - CurrentLocation));
	}
	else if (bWarpTranslation)
	{
		FVector DeltaTranslation = InRootMotion.GetTranslation();

		const FTransform RootMotionDelta = UMotionWarpingUtilities::ExtractRootMotionFromAnimation(Animation.Get(), PreviousPosition, FMath::Min(CurrentPosition, EndTime));

		const float HorizontalDelta = RootMotionDelta.GetTranslation().Size2D();
		const float HorizontalTarget = FVector::Dist2D(CharacterTransform.GetLocation(), GetTargetLocation());
		const float HorizontalOriginal = RootMotionTotal.GetTranslation().Size2D();
		const float HorizontalTranslationWarped = HorizontalOriginal != 0.f ? ((HorizontalDelta * HorizontalTarget) / HorizontalOriginal) : 0.f;

		if(!bIgnoreXYAxes)
		{
			const FTransform MeshRelativeTransform = FTransform(CharacterOwner->GetBaseRotationOffset(), CharacterOwner->GetBaseTranslationOffset());
			const FTransform MeshTransform = MeshRelativeTransform * CharacterOwner->GetActorTransform();
			DeltaTranslation = MeshTransform.InverseTransformPositionNoScale(GetTargetLocation()).GetSafeNormal2D() * HorizontalTranslationWarped;
		}

		if (!bIgnoreZAxis)
		{
			if ((WarpZTolerance < 0.f || !FMath::IsNearlyZero(static_cast<float>(CachedRootMotion.GetTranslation().Z), static_cast<float>(WarpZTolerance))))
			{
				const float VerticalDelta = RootMotionDelta.GetTranslation().Z;
				const float VerticalTarget = GetTargetLocation().Z - CharacterOwner->GetActorLocation().Z;
				const float VerticalOriginal = RootMotionTotal.GetTranslation().Z;
				const float VerticalTranslationWarped = VerticalOriginal != 0.f ? ((VerticalDelta * VerticalTarget) / VerticalOriginal) : 0.f;

				DeltaTranslation.Z = VerticalTranslationWarped;
			}
			else
			{
				const FVector StartLocation = StartTransform.GetLocation();
				const FVector CurrentLocation = CharacterTransform.GetLocation();
				const FVector TargetLocation = GetTargetLocation();

				const float Alpha = (FMath::Min(CurrentPosition, EndTime) - ActualStartTime) / (EndTime - ActualStartTime);
				float TargetZThisFrame = FMath::Lerp<float>(StartLocation.Z, TargetLocation.Z, Alpha);

				DeltaTranslation.Z = (TargetZThisFrame - CurrentLocation.Z);
			}
		}
		else
		{
			DeltaTranslation.Z = InRootMotion.GetTranslation().Z;
		}

		FinalRootMotion.SetTranslation(DeltaTranslation);
	}

	if (bWarpRotation)
	{
		FQuat WarpedRotation = WarpRotation(InRootMotion, RootMotionTotal, DeltaSeconds);

		if(bIgnoreYaw)
		{
			const float Angle = WarpedRotation.Inverse().GetTwistAngle(FVector::UpVector);
			WarpedRotation = WarpedRotation * FQuat(FVector::UpVector, Angle);
		}

		FinalRootMotion.SetRotation(WarpedRotation);
	}

	// Debug
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const int32 DebugLevel = FMotionWarpingCVars::CVarMotionWarpingDebug.GetValueOnGameThread();
	if (DebugLevel > 0)
	{
		PrintLog(InRootMotion, FinalRootMotion);

		if (DebugLevel >= 2)
		{
			const float DrawDebugDuration = FMotionWarpingCVars::CVarMotionWarpingDrawDebugDuration.GetValueOnGameThread();
			DrawDebugCoordinateSystem(CharacterOwner->GetWorld(), GetTargetLocation(), GetTargetRotator(), 50.f, false, DrawDebugDuration, 0, 1.f);
		}
	}
#endif

	return FinalRootMotion;
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
void URootMotionModifier_CitySampleSimpleWarp::PrintLog(const FTransform& OriginalRootMotion, const FTransform& WarpedRootMotion) const
{
	if (const ACharacter* CharacterOwner = GetCharacterOwner())
	{
		const FVector CurrentLocation = (CharacterOwner->GetActorLocation());// - CharacterOwner->GetActorUpVector() * CharacterOwner->GetSimpleCollisionHalfHeight());
		const FVector CurrentToTarget = (GetTargetLocation() - CurrentLocation).GetSafeNormal2D();
		const FVector FutureLocation = CurrentLocation + (CharacterOwner->GetMesh()->ConvertLocalRootMotionToWorld(WarpedRootMotion)).GetTranslation();
		const FRotator CurrentRotation = CharacterOwner->GetActorRotation();
		const FRotator FutureRotation = (WarpedRootMotion.GetRotation() * CharacterOwner->GetActorQuat()).Rotator();
		const float Dot = FVector::DotProduct(CharacterOwner->GetActorForwardVector(), CurrentToTarget);
		const float CurrentDist2D = FVector::Dist2D(GetTargetLocation(), CurrentLocation);
		const float FutureDist2D = FVector::Dist2D(GetTargetLocation(), FutureLocation);
		const float DeltaSeconds = CharacterOwner->GetWorld()->GetDeltaSeconds();
		const float Speed = WarpedRootMotion.GetTranslation().Size() / DeltaSeconds;
		const float EndTimeOffset = CurrentPosition - EndTime;

		UE_LOG(LogTemp, Log, TEXT("CitySampleSimpleWarp. NetMode: %d Char: %s Anim: %s Win: [%f %f][%f %f] DT: %f WT: %f ETOffset: %f Dist2D: %f Z: %f FutureDist2D: %f Z: %f Dot: %f Delta: %s (%f) FDelta: %s (%f) Speed: %f Loc: %s FLoc: %s Rot: %s FRot: %s"),
			(int32)CharacterOwner->GetWorld()->GetNetMode(), *GetNameSafe(CharacterOwner), *GetNameSafe(Animation.Get()), StartTime, EndTime, PreviousPosition, CurrentPosition, DeltaSeconds, CharacterOwner->GetWorld()->GetTimeSeconds(), EndTimeOffset, 
			CurrentDist2D, (GetTargetLocation().Z - CurrentLocation.Z), FutureDist2D, (GetTargetLocation().Z - FutureLocation.Z), Dot,
			*OriginalRootMotion.GetTranslation().ToString(), OriginalRootMotion.GetTranslation().Size(), *WarpedRootMotion.GetTranslation().ToString(), WarpedRootMotion.GetTranslation().Size(), Speed,
			*CurrentLocation.ToString(), *FutureLocation.ToString(), *CurrentRotation.ToCompactString(), *FutureRotation.ToCompactString());
	}
}
#endif

void URootMotionModifier_CitySampleSimpleWarp::BP_AddOrUpdateWarpTarget(UMotionWarpingComponent* MotionWarpingComp, FName Name, FVector Location, FRotator Rotation)
{
	if(ensureAlways(MotionWarpingComp))
	{
		MotionWarpingComp->AddOrUpdateWarpTarget(FMotionWarpingTarget(Name, FTransform(Rotation, Location)));
	}
}