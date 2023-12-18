// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CitySampleCameraMode.h"
#include "Util/CitySampleInterpolators.h"

#include "CitySampleCam_ThirdPerson.generated.h"

class UCurveVector;

/**
 * Auto follow types this camera mode supports
 */
UENUM(BlueprintType)
enum class ECameraAutoFollowMode : uint8
{
	None,
	LazyFollow,
	FullFollow
};

/**
 * Struct defining a feeler ray used for camera penetration avoidance.
 */
USTRUCT(BlueprintType)
struct FPenetrationAvoidanceRay
{
	GENERATED_BODY()

	/** FRotator describing deviance from main ray */
	UPROPERTY(EditAnywhere, Category=PenetrationAvoidanceRay)
	FRotator AdjustmentRot;

	/** how much this feeler affects the final position if it hits the world */
	UPROPERTY(EditAnywhere, Category= PenetrationAvoidanceRay)
	float WorldWeight = 0.f;

	/** extent to use for collision when tracing this feeler */
	UPROPERTY(EditAnywhere, Category= PenetrationAvoidanceRay)
	float Radius = 0.f;

	/** minimum frame interval between traces with this feeler if nothing was hit last frame */
	UPROPERTY(EditAnywhere, Category= PenetrationAvoidanceRay)
	int32 TraceInterval = 0;

	/** number of frames since this feeler was used */
	UPROPERTY(transient)
	int32 FramesUntilNextTrace = 0;

	UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceRay)
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = PenetrationAvoidanceRay)
	bool bPrimaryRay = false;

	FPenetrationAvoidanceRay()
		: AdjustmentRot(ForceInit)
	{
	}

	FPenetrationAvoidanceRay(FRotator InAdjustmentRot, 
							 float InWorldWeight, 
							 float InRadius, 
							 int32 InTraceInterval,
							 bool bInPrimaryRay)
		: AdjustmentRot(InAdjustmentRot)
		, WorldWeight(InWorldWeight)
		, Radius(InRadius)
		, TraceInterval(InTraceInterval)
		, bPrimaryRay(bInPrimaryRay)
	{}
};

/**
 * Third person camera implementation of the CitySampleCameraMode. 
 * Features camera smoothing, auto follow behaviors, and penetration avoidance.
 */
UCLASS(Blueprintable)
class CITYSAMPLE_API UCitySampleCam_ThirdPerson : public UCitySampleCameraMode
{
	GENERATED_BODY()
	
public:
	UCitySampleCam_ThirdPerson();

	//~ Begin UCitySampleCameraMode Interface
	virtual void UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, struct FTViewTarget& OutVT) override;
	virtual void OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode) override;
	virtual void SkipNextInterpolation() override;
	//~ End UCitySampleCameraModeInterface

protected:

	/** Transform for the Pivot, in the ViewTarget's space. This is the point the camera rotates around. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FTransform PivotToViewTarget;

	/** Interpolator for smooth changes to the camera pivot's location in world space. Note: For very fast moving objects you may want to set this to 0,0 for instant pivot updates */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FDoubleIIRInterpolatorVector PivotLocInterpolator = FDoubleIIRInterpolatorVector(4.f, 12.f);

	/** Interpolator for smooth changes to the camera pivot's rotation in world space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FDoubleIIRInterpolatorRotator PivotRotInterpolator = FDoubleIIRInterpolatorRotator(4.f, 7.f);

	/** Min and Max pitch thresholds for the camera pivot, in degrees  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FVector2D PivotPitchLimits;

	/** Min and Max yaw thresholds for the camera pivot in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FVector2D PivotYawLimits;

	/** Default transform for the camera in pivot space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FTransform CameraToPivot;

	/** Interpolator for smooth changes to the camera's translation in pivot space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FDoubleIIRInterpolatorVector CameraToPivotTranslationInterpolator = FDoubleIIRInterpolatorVector(4.f, 12.f);

	/** 0 on this curve corresponds to PitchMin (PitchLimits.X, looking down). 1 corresponds to max pitch (PitchLimits.Y, looking up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	UCurveVector* CameraToPivot_PitchAdjustmentCurve;

	/** Scalar applied to pitch adjustment scale curve output, for cases where you want to make a 0..1 curve shape and set magnitude here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	float CameraToPivot_PitchAdjustmentCurveScale = 1.f;

	/** 0 on this curve corresponds to CameraToPivot_SpeedAdjustment_SpeedRange.X (range min). 1 corresponds to CameraToPivot_SpeedAdjustment_SpeedRange.Y (range max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	UCurveVector* CameraToPivot_SpeedAdjustmentCurve;

	/** Scalar applied to speed adjustment scale curve output, for cases where you want to make a 0..1 curve shape and set magnitude here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	float CameraToPivot_SpeedAdjustmentCurveScale = 1.f;

	/** Speeds between which the camera to pivot translation is interpolated between base CameraToPivot translation and the translation obtained from the CameraToPivot_SpeedAdjustmentCurve curve.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSettings")
	FVector2D CameraToPivot_SpeedAdjustment_SpeedRange;


	/** Auto follow behavior to apply to the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoFollow")
	ECameraAutoFollowMode AutoFollowMode;

	/** Pitch limits to apply when the LazyFollow CameraAutoFollowMode is enabled and AllowLazyAutoFollowPitchControl is false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoFollow")
	FVector2D LazyAutoFollowPitchLimits;

	/** When enabled, manual camera pitch set by player will be used when in LazyFollow mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoFollow")
	bool bAllowLazyAutoFollowPitchControl = false;

	/** Higher numbers == slower following behavior, lower == faster. 0 == perfectly tight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoFollow")
	float LazyFollowLaziness = 200.f;

	/** Delay in seconds before lazy follow behavior kicks in after manual control of the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoFollow")
	float LazyFollowDelayAfterUserControl = 0.7f;
	
	/** Current time remaining after manual control of the camera before lazy follow behavior kicks in again */
	float LazyFollowDelay_TimeRemaining = 0.f;


	/** Offset of the camera's look at point relative to the ViewTarget's transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	FVector LookatOffsetLocal;

	/** Interpolator for smooth updates to the final world position of the camera look at point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	FIIRInterpolatorVector LookatWorldSpaceInterpolator = FIIRInterpolatorVector(8.f);

	/** Cache of previously calculated camera look at point in world space */
	FVector LastLookatWorldSpace;

	/** When enabled the camera will automatically rotate to focus the look at point. If false, the camera will just look straight ahead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	uint32 bUseLookatPoint : 1;

	/** When enabled, the view target's velocity will be taken into account when calculating our goal look at point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	uint32 bDoPredictiveLookat : 1;

	/** The length of time in seconds in the future the camera will predict the look at point to be when DoPredictiveLookat is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookAt")
	float PredictiveLookatTime = 1.f;

	/** Rays used when detecting obstacles that the camera needs to avoid */
	UPROPERTY(EditAnywhere, Category = "PenetrationAvoidance")
	TArray<FPenetrationAvoidanceRay> CameraPenetrationAvoidanceRays;

	/** Rays used when ensuring that the camera's safe location remains unobstructed */
	UPROPERTY(EditAnywhere, Category = "PenetrationAvoidance")
	TArray<FPenetrationAvoidanceRay> SafeLocPenetrationAvoidanceRays;

	/** Offset relative to view target to the "safe" place for the camera.  This is where penetration ray traces originate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	FVector SafeLocationOffset;

	/** Interpolator for smooth updates to the camera's safe location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	FIIRInterpolatorVector SafeLocationInterpolator = FIIRInterpolatorVector(0.f);

	/** If true, does an extra ray trace to make sure the safe loc is actually safe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	uint32 bValidateSafeLoc:1;

	/** If true, the camera will perform ray traces to detect potential obstacles and adjust accordingly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	uint32 bPreventCameraPenetration : 1;

	/** If true, try to detect nearby walls and move the camera in anticipation.  Helps prevent popping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	uint32 bDoPredictiveAvoidance:1;

	/** Blend time when having to bring the camera closer to the safe loc to avoid penetration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	float PenetrationBlendInTime = 0.15f;

	/** Blend time when bringing the camera away from the safe loc when space is available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PenetrationAvoidance")
	float PenetrationBlendOutTime = 0.25f;

	/** Cache of camera safe location calculated on the previous update */
	UPROPERTY(transient)
	FVector LastSafeLocationLocal;

	/** Cache of the percentage of the distance blocked between the camera location and safe location calculated on the previous update */
	UPROPERTY(transient)
	float LastPenetrationBlockedPct = 1.f;

	/** When true, any blending of the block percentage when calculating penetration avoidance is skipped */
	bool bSkipNextPredictivePenetrationAvoidanceBlend = false;

	/**
	* Handles traces to make sure camera does not penetrate geometry and tries to find the best location for the camera.
	* Also handles interpolating back smoothly to ideal/desired position.
	*
	* @param	SafeLoc		Worst location (Start Trace)
	* @param	DesiredLocation		Desired / Ideal position for camera (End Trace)
	* @param	DeltaTime			Time passed since last frame.
	* @param	DistBlockedPct		Percentage of distance blocked last frame, between SafeLoc and CameraLoc. To interpolate out smoothly.
	* @param	bSingleRayOnly		Only fire a single ray.  Do not send out extra predictive feelers.
	*/
	virtual void PreventCameraPenetration(AActor* Target, TArray<FPenetrationAvoidanceRay>& Rays, const FVector& SafeLoc, const FVector& IdealCameraLoc, float DeltaTime, FVector& OutCameraLoc, float& DistBlockedPct, bool bPrimaryRayOnly = false);

	/** Returns the camera pivot's transform in world space */
	FTransform GetLastPivotToWorld() const
	{
		return LastPivotToWorld;
	};

	/**  Returns the pivot's transform in view target space */
	virtual FTransform GetPivotToViewTarget(const AActor* ViewTarget) const
	{
		return PivotToViewTarget;
	};

	/** Returns camera-to-pivot, before any smoothing. */
	virtual FTransform GetBaseCameraToPivot(const AActor* ViewTarget) const
	{
		return CameraToPivot;
	};

	/** Returns smoothed camera-to-pivot. */
	virtual FTransform GetCameraToPivot(const AActor* ViewTarget) const;

	/** Attempts to return view target's transform in world space */
	FTransform GetViewTargetToWorld(const AActor* ViewTarget) const;

	/** Attempts to return the view target's mesh height offset determined by the Pelvis bone's location on a character actor */
	float GetViewTargetMeshHeightOffset(const AActor* ViewTarget) const;

	/** Attempts to return the rotation of the actor that auto follow is following */
	FQuat GetAutoFollowPivotToWorldRotation(const AActor* FollowActor) const;

	/** Computes the camera pivot's goal transform in world space */
	virtual FTransform ComputePivotToWorld(const AActor* ViewTarget) const;

	/** Computes the camera's goal transform in world space */
	FTransform ComputeCameraToWorld(const AActor* ViewTarget, FTransform const& PivotToWorld) const;

	/** Computes the final FOV value during camera updates. Override as needed.*/
	virtual float ComputeFinalFOV(const AActor* ViewTarget) const
	{
		return FOV;
	}

	/** Computes the final Yaw Modifier applied to the camera's rotation. Override as needed. */
	virtual float ComputeYawModifier(const AActor* ViewTarget, float DeltaTime)
	{
		return 0.0f;
	}

	/** Computes the final Roll Modifier applied to the camera's rotation. Override as needed. */
	virtual float ComputeRollModifier(const AActor* ViewTarget, float DeltaTime)
	{
		return 0.0f;
	}

	/** Updates auto follow mode settings based on gameplay logic. Override as needed. */
	virtual void AdjustAutoFollowMode(const AActor* ViewTarget) {};

	/** Computes the camera look at point's goal position based on gameplay logic. Override as needed. */
	virtual void ComputePredictiveLookAtPoint(FVector& LookAtPointOutput, const AActor* ViewTarget, float DeltaTime);

	/** Computes the final look at point world position based on gameplay logic. Override as needed. */
	virtual FVector ComputeWorldLookAtPosition(const FVector IdealWorldLookAt, float DeltaTime);

	/** Computes the Smoothed Pivot to World location based on gameplay logic. Override as needed. */
	virtual FRotator ComputeSmoothPivotRotation(const FRotator IdealPivotToWorldRot, float DeltaTime);


	/** Cache of previous pivot to world transform post-smoothing */
	FTransform LastPivotToWorld;

	/** Cache of previous pivot to world transform pre-smoothing */
	FTransform LastUnsmoothedPivotToWorld;

	/** Cache of previous owner PlayerController's control rotation */
	FRotator LastControlRotation;	


	/** When enabled, debug visuals will be rendered for the camera pivot point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugPivot = false;

	/** When enabled, debug visuals will be rendered for the camera look at point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugLookat = false;

	/** When enabled, debug visuals will be rendered for the camera safe location, where penetration avoidance ray traces originate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugSafeLoc = false;

	/** When enabled, debug visuals will be rendered for the penetration avoidance ray traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugPenetrationAvoidance = false;

};
