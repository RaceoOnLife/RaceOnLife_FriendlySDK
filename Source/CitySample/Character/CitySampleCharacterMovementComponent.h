// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CitySampleCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class CITYSAMPLE_API UCitySampleCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	//~ Begin UCharacterMovementComponent Interface
public:
	virtual void PhysicsRotation(float DeltaTime) override;

protected:
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const override;
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact) override;
	//~ End UCharacterMovementComponent Interface


public:
	UFUNCTION(BlueprintPure)
	bool WasSlideAlongSurfaceBlockedRecently(float Tolerance = 0.01f) const;

protected:
	// ConstraintInputAcceleration will interpolate the input size to go from this value to 1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinInputAccelerationSize = 0.0f;

	// Minimum angle for movement to slide against. This prevents sliding against very sharp angles that will result in very small movements. Only active if greater than 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHorizontalSurfaceSlideAngle = 0.0f;

	// Minimum angle for movement to slide against when colliding against a character
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHorizontalSurfaceSlideAngleCharacter = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly)
	bool bLastSurfaceWasCharacter = false;

private:
	UPROPERTY(EditDefaultsOnly)
	bool bUseAnimAuthoritativeRotation = true;

	UPROPERTY(EditDefaultsOnly)
	bool bUseMassEntityRotation = false;

	float TimeLastSlideAlongSurfaceBlock = -1000.0f;
};