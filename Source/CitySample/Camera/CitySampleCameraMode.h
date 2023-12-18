// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "CineCameraComponent.h"
#include "CoreMinimal.h"
#include "Util/CitySampleInterpolators.h"

#include "CitySampleCameraMode.generated.h"

class ACitySamplePlayerCameraManager;
class ACitySamplePlayerController;
class UCameraShakeBase;

/**
 * Base class for Camera Mode implementations. Has functions and settings for camera activation, updating, and whether or not to use cine cam properties
 */
UCLASS(Blueprintable)
class CITYSAMPLE_API UCitySampleCameraMode : public UObject
{
	GENERATED_BODY()

public:
	UCitySampleCameraMode();

	/** Skips interpolation steps for the current camera mode to cleanly handle specific situations depending on the camera mode */
	virtual void SkipNextInterpolation();

	/** Notify camera that it "active", i.e. the primary camera. */
	virtual void OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode);

	/** Notify camera that it is no longer the primary camera.  Might still be processing as it blends out. */
	virtual void OnBecomeInactive(AActor* ViewTarget, UCitySampleCameraMode* NewActiveMode);

	/** Called by the CitySampleCameraManager in UpdateViewTarget, camera mode implementations can use this to tailor their update logic as needed*/
	virtual void UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, struct FTViewTarget& OutVT);

	/** Called when this mode is fully blended out and removed from the stack, as in it has no more influence */
	virtual void OnRemovedFromStack() {};

	/** Returns camera transition time for this camera mode */
	float GetTransitionTime() const;

	/** Camera actor this mode is bound to */
	UPROPERTY(transient)
	ACitySamplePlayerCameraManager* PlayerCamera;

	/** Amount of time camera transition should take when entering this camera mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (DisplayName = "Transition In Time"))
	float TransitionInTime;

	/** Transition parameters for this camera mode type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	FViewTargetTransitionParams TransitionParams;

	/** Desired FOV.  Full angle, in degrees (e.g. 90.f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings", meta = (EditCondition = "!bUseCineCamSettings"))
	float FOV = 75.f;

	/** If true, this camera mode will use a cine cam component, allowing access to cinematic camera settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam")
	bool bUseCineCam;

	/** When true, custom cine cam settings will be applied when UseCinecam is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam")
	bool bUseCineCamSettings;

	/** When true, default cinecam filmback settings will be overriden by what is set in CineCam_FilmbackOverride */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam", meta = (EditCondition = bUseCineCamSettings))
	bool bOverrideFilmback = false;

	/** Custom filmback settings to use when OverrideFilmback is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam", meta = (EditCondition = bOverrideFilmback))
	FCameraFilmbackSettings CineCam_FilmbackOverride;
	
	/** Camera focal length to use when UseCinecamSettings is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam", meta = (EditCondition = bUseCineCamSettings))
	float CineCam_CurrentFocalLength = 30.f;

	/** Camera aperture to use when UseCinecamSettings is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam", meta = (EditCondition = bUseCineCamSettings))
	float CineCam_CurrentAperture = 11.f;

	/** Custom focus distance adjustment to use when UseCinecamSettings is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam", meta = (EditCondition = bUseCineCamSettings))
	float CineCam_FocusDistanceAdjustment = 0.f;

	/** FOV to use when UseCineCam is enabled */
	UPROPERTY(VisibleAnywhere, Category = "CineCam", meta = (EditCondition = bUseCineCamSettings))
	float CineCam_DisplayOnly_FOV = 70.f;

	/** List for tracking actors that block the camera */
	UPROPERTY(Transient)
	TArray<AActor*> BlockingActors;

	/** Blueprint native event for camera modes to reset to default settings as needed */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ResetToDefaultSettings();

	/** 
	 * returns true if this mode should stop simulating while transitioning out, and blend hold its last POV while active 
	 * returns false to keep simulating while transitioning out
	 */
	virtual bool ShouldLockOutgoingPOV() const;

protected:

	/** Flag to keep track of whether the current camera mode is active */
	bool bIsActive = false;

	/** Camera shake class to create when this camera mode becomes active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	/** Camera Shake object instance created when this camera mode is activated and a valid CameraShakeClass is set */
	UPROPERTY(transient)
	UCameraShakeBase* CameraShakeInstance = nullptr;

	/** When true, camera shake will be scaled using the ShakeScaling_SpeedRange and the ShakeScaling_ScaleRange */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	bool bScaleShakeWithViewTargetVelocity = false;

	/** Ranges of view target velocity that will be mapped to a camera shake scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (EditCondition = bScaleShakeWithViewTargetVelocity))
	FVector2D ShakeScaling_SpeedRange { 0.f, 300.f };

	/** Maps to ShakeScaling_SpeedRange, such that at SpeedRange.X, shake will scale to ScaleRange.X */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (EditCondition = bScaleShakeWithViewTargetVelocity))
	FVector2D ShakeScaling_ScaleRange { 1.f, 3.f };

	/** Interpolator for smooth changes in camera shake intensities */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (EditCondition = bScaleShakeWithViewTargetVelocity))
	FIIRInterpolatorFloat ShakeScaleInterpolator = FIIRInterpolatorFloat(5.f);

	/** When true, the camera mode will attempt to display debug information related to camera shake*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	bool bDrawDebugShake = false;

	/** When UseCustomFocusDistance is true, this event should be implemented to determine a focus distance appropriate for the camera mode */
	UFUNCTION(BlueprintImplementableEvent)
	float GetCustomFocusDistance(AActor* ViewTarget, const FTransform& ViewToWorld) const;

	/** When true, a custom focus distance will be provided to the cine cam via an implementation of the GetCustomFocusDistance Blueprint Implementable Event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CineCam", meta = (EditCondition = bUseCineCamSettings))
	bool bUseCustomFocusDistance;

	/** When true, custom view pitch limits will be used for this camera mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Manager Overrides")
	bool bOverrideViewPitchMinAndMax = false;

	/** Pitch minimum limit to use when OverrideViewPitchMinAndMax is true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Manager Overrides", meta = (EditCondition = bOverrideViewPitchMinAndMax))
	float ViewPitchMinOverride = 0.f;

	/** Pitch maximum limit to use when OverrideViewPitchMinAndMax is true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Manager Overrides", meta = (EditCondition = bOverrideViewPitchMinAndMax))
	float ViewPitchMaxOverride = 0.f;

	/** Attempts to cast and return the camera modes's owning CitySample Player Controller */
	ACitySamplePlayerController* GetOwningCitySamplePC() const;

	/** Applies cine cam settings to the current camera mode if relevant */
	void ApplyCineCamSettings(FTViewTarget& OutVT, UCineCameraComponent* CineCamComp, float DeltaTime);

	/** Attempts to determine the camera mode's desired focus distance, whether it be calculated normally or if a custom focus distance method is used  */
	virtual float GetDesiredFocusDistance(AActor* ViewTarget, const FTransform& ViewToWorld) const;

	/** When true, camera modes will reset certain interpolators. Useful for hard cuts or unique camera situations */
	bool bSkipNextInterpolation = false;

	/** Camera to world transform that is cached and can be used when something wants to easily access the camera's transform */
	FTransform LastCameraToWorld;
};
