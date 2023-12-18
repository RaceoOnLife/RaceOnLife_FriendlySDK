// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CitySampleCameraMode.h"
#include "Util/CitySampleInterpolators.h"
#include "CitySampleCamera_Drone.generated.h"

/**
 * Camera mode for the drone
 */
UCLASS(Blueprintable)
class CITYSAMPLE_API UCitySampleCamera_Drone : public UCitySampleCameraMode
{
	GENERATED_BODY()
	
public:
	UCitySampleCamera_Drone();

	virtual void UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, struct FTViewTarget& OutVT) override;

	// UCitySampleCameraMode overrides
	virtual void OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode) override;

	UPROPERTY(EditAnywhere)
	FIIRInterpolatorRotator DroneTiltInterpolator = FIIRInterpolatorRotator(8.f);

	/** How quickly/aggressively to interp into the tilted position. */
	UPROPERTY(EditDefaultsOnly)
	float DroneTiltInterpSpeed_Accel;

	/** How quickly/aggressively to interp back to neutral when decelerating */
	UPROPERTY(EditDefaultsOnly)
	float DroneTiltInterpSpeed_Decel;

	/** For interpolating the tilt. */
	FRotator LastTiltedDroneRot;

	/** The drone's up vector during neutral hovering. The magnitude determines resistance to tilt when moving. */
	UPROPERTY(EditDefaultsOnly)
	FVector TiltUpVector;

	UPROPERTY(EditDefaultsOnly)
	bool bEnableTiltLimits = false;
	
	UPROPERTY(EditDefaultsOnly)
	FRotator TiltLimits;
};

