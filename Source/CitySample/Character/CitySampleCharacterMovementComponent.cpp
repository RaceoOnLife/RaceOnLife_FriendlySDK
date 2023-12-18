// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Kismet/GameplayStatics.h"

#include "Animation/MassCrowdAnimInstance.h"

#include "Character/CitySampleCharacter.h"


namespace CitySampleCharacterMovementStatics
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static int32 VisualizeSlideAlongSurface = 0;
	FAutoConsoleVariableRef CVarVisualizeSlideAlongSurface(
		TEXT("CitySample.VisualizeSlideAlongSurface"),
		VisualizeSlideAlongSurface,
		TEXT("Whether to draw in-world debug information for character movement's slide along surface.\n")
		TEXT("0: Disable, 1: Enable"),
		ECVF_Cheat);
#endif
}

void UCitySampleCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	const ACitySampleCharacter* const Char = Cast<ACitySampleCharacter>(CharacterOwner);

	// When available, use the rotation provided by Mass system directly. 
	if (bUseMassEntityRotation && Char && Char->GetMesh())
	{
		if (UMassCrowdAnimInstance* AnimInstance = Cast<UMassCrowdAnimInstance>(Char->GetMesh()->GetAnimInstance()))
		{
			// If we swapped from an ISM this frame, skip this. Mass should already set our actor to a good transform
			if(!AnimInstance->MassSwappedThisFrame())
			{
				FQuat DesiredRotation = AnimInstance->GetMassEntityTransform().GetRotation();
				DesiredRotation.DiagnosticCheckNaN(TEXT("CitySampleCharacterMovementComponent::PhysicsRotation(): DesiredRotation"));
				MoveUpdatedComponent(FVector::ZeroVector, DesiredRotation, /*bSweep*/ false);
				return;
			}
		}
	}

	Super::PhysicsRotation(DeltaTime);
}

FVector UCitySampleCharacterMovementComponent::ConstrainInputAcceleration(const FVector& InputAcceleration) const
{
	FVector Result = InputAcceleration;
	const float InputAccelerationSize = InputAcceleration.Size();
	if (InputAccelerationSize > SMALL_NUMBER)
	{
		const float ScaledInputAccelerationSize = FMath::Lerp(MinInputAccelerationSize, 1.0f, InputAccelerationSize);
		Result = InputAcceleration * ScaledInputAccelerationSize / InputAccelerationSize;
	}

	Result = Super::ConstrainInputAcceleration(Result);
	return Result;
}

float UCitySampleCharacterMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult& Hit, bool bHandleImpact)
{
	FVector Normal(InNormal);

	const bool bHitCharacter = Hit.GetHitObjectHandle().DoesRepresentClass(ACharacter::StaticClass());
	bLastSurfaceWasCharacter = bHitCharacter;
	const float EffectiveMinHorizontalSurfaceSlideAngle = bHitCharacter ? MinHorizontalSurfaceSlideAngleCharacter : MinHorizontalSurfaceSlideAngle;
	if (EffectiveMinHorizontalSurfaceSlideAngle > 0.0f)
	{
		// Use input acceleration as our movement direction because our "movement delta" may already be affected by the surface slide.
		const FVector MovementDirection = Acceleration.GetSafeNormal2D();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		FColor HitNormalColor = FColor::Green;
#endif

		const float MovementDotNormal = MovementDirection | Normal;
		if (FMath::Abs(MovementDotNormal) > FMath::Cos(FMath::DegreesToRadians(EffectiveMinHorizontalSurfaceSlideAngle)))
		{
			// If the angle is too sharp, consider it opposite to our movement
			Normal = -MovementDirection;
			TimeLastSlideAlongSurfaceBlock = GetWorld()->GetTimeSeconds();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			HitNormalColor = FColor::Red;
#endif
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CitySampleCharacterMovementStatics::VisualizeSlideAlongSurface > 0)
		{
			const float DebugAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(MovementDotNormal)));
			DrawDebugLine(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetActorLocation() + Normal * 30.0f, HitNormalColor, false, -1.0f, SDPG_Foreground, 3.0f);
			DrawDebugLine(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetActorLocation() + MovementDirection * 30.0f, FColor::Blue, false, -1.0f, SDPG_Foreground, 3.0f);
			DrawDebugString(GetWorld(), CharacterOwner->GetActorLocation(), FString::SanitizeFloat(DebugAngle), nullptr, FColor::White, 0.0f, false, 3.0f);
		}
#endif
	}

	return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}

bool UCitySampleCharacterMovementComponent::WasSlideAlongSurfaceBlockedRecently(float Tolerance /*= 0.01f*/) const
{
	if (const UWorld* const World = GetWorld())
	{
		const float TimeThreshold = FMath::Max(Tolerance, World->DeltaTimeSeconds + KINDA_SMALL_NUMBER);
		return World->TimeSince(TimeLastSlideAlongSurfaceBlock) <= TimeThreshold;
	}
	return false;
}