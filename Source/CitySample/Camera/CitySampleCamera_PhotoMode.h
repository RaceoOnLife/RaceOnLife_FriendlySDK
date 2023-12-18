// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CitySampleCameraMode.h"
#include "CitySampleCamera_PhotoMode.generated.h"

/**
 * Camera mode that works in tandem with the PhotoModeComponent to apply user controlled adjustments to the cine cam component used for Photo Mode
 */
UCLASS(Blueprintable)
class CITYSAMPLE_API UCitySampleCamera_PhotoMode : public UCitySampleCameraMode
{
	GENERATED_BODY()

public:

	UCitySampleCamera_PhotoMode();

	//~ Begin UCitySampleCameraMode Interface
	virtual void UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, struct FTViewTarget& OutVT) override;
	virtual void OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode) override;
	//~ End UCitySampleCameraMode Interface

	UFUNCTION(BlueprintPure)
	float GetPhotoModeCustomFocusDistance() const { return CustomFocusDistance;}

protected:

	/** Distance used for auto focus traces and also a default max value for the focus distance*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PhotoModeAutoFocusTraceDistance = 5000.0f;
	 
	/** Radius for sphere used in auto focus trace. This gives a bit of leeway when trying to focus on an object in the world*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SphereTraceRadius = 12.0f;

private:
	
	/** Returns PlayerController's current Photomode Component*/
	class UPhotoModeComponent* GetPhotoModeComponent() const;
	
	/** Ensures that manual or auto exposure values are properly applied*/
	void UpdateExposureSettings(
		const struct FPhotoModeSettings& Settings,
		struct FTViewTarget& OutVT) const;
	
	/** Ensures that either manual or auto focus distances are properly applied*/
	void UpdateFocusSettings(
		UPhotoModeComponent* PhotoModeComponent,
		const struct FPhotoModeSettings& Settings,
		UCineCameraComponent* CineCamComp,
		const struct FTViewTarget& VT);

	/** Ensures that the rotation of this camera mode's view target matches the Photo Mode pawn's view rotation*/
	void UpdateFinalRotation(
		const AActor* ViewTarget,
		const struct FPhotoModeSettings& Settings,
		struct FTViewTarget& OutVT);

	/** Receives Trace data and determines AsyncFocusDistance to be used*/
	void HandleAsyncFocusTrace(const FTraceHandle& InTraceHandle, FTraceDatum& InTraceDatum);

	/** Stores focus distance determined by async trace*/
	float AsyncFocusDistance;

	/** Stores manual focus distance set by the user */
	float CustomFocusDistance;

	/** Stores base exposure bias value determined by current Post Process Volumes */
	float BaseExposureBias = 1;

	/** Trace delegate used for our Async traces when trying to determine auto focus distance */
	FTraceDelegate AsyncFocusTraceDelegate;
};
