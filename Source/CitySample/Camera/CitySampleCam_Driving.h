// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CitySampleCam_ThirdPerson.h"
#include "Vehicles/CitySampleDrivingState.h"

#include "CitySampleCam_Driving.generated.h"

/**
 * An extension of the CitySampleCam_ThirdPerson class with extra features meant for driving cameras
 */
UCLASS()
class CITYSAMPLE_API UCitySampleCam_Driving : public UCitySampleCam_ThirdPerson
{
	GENERATED_BODY()

public:

	//~ Begin CitySampleCameraMode Interface
	virtual void UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, struct FTViewTarget& OutVT) override;
	virtual void OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode) override;
	virtual void SkipNextInterpolation() override;
	//~ End CitySampleCameraModeInterface

protected:

	//~ Begin CitySampleCam_ThirdPerson Interface
	virtual float ComputeFinalFOV(const AActor* ViewTarget) const override final;
	virtual float ComputeYawModifier(const AActor* ViewTarget, float DeltaTime) override final;
	virtual float ComputeRollModifier(const AActor* ViewTarget, float DeltaTime) override final;
	virtual void AdjustAutoFollowMode(const AActor* ViewTarget) override final;
	virtual void ComputePredictiveLookAtPoint(FVector& LookAtPointOutput, const AActor* ViewTarget, float DeltaTime) override final;
	virtual FVector ComputeWorldLookAtPosition(const FVector IdealWorldLookAt, float DeltaTime) override final;
	virtual FRotator ComputeSmoothPivotRotation(const FRotator IdealPivotToWorldRot, float DeltaTime) override final;
	//~ End CitySampleCam_ThirdPerson Interface

	/** Updates the pivot rot interpolation speeds based on a variety of vehicle states */
	void UpdatePivotRotInterpSpeeds(const FRotator& GoalRotation, float DeltaTime);

	/** Checks camera parameters to detect and possibly trigger driving camera behavioral transitions */
	void CheckForRotationalTransitionTriggers(FVector2D& GoalPivotRotInterpSpeeds);

	/** Changes rotational interpolation speeds set for smooth changes between forward and reverse camera behavior */
	void TriggerTransitionBetweenForwardAndReverseBehavior(FVector2D& GoalPivotRotInterpSpeeds);

	/** Changes rotational interpolation speeds set for smooth changes from manual camera control to automatic camera control */
	void TriggerTransitionBetweenManualAndAutoBehavior(FVector2D& GoalPivotRotInterpSpeeds);


	//Lazy Follow Members

	/**Lazy follow laziness to use when driving normally. Higher values mean a lazier auto follow camera. Camera's follow mode must be set to "LazyFollow" for this to work. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Auto Follow")
	float DrivingLazyFollowLaziness = 0.0f;

	/**Lazy follow laziness to use when reversing. Higher values mean a lazier auto follow camera. Camera's follow mode must be set to "LazyFollow" for this to work. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Auto Follow")
	float ReverseCamLazyFollowLaziness = 12.0f;

	/**Velocity threshold used to determine whether the car is properly moving forward or not. We adjust the camera's follow behavior based on this. You want to keep this value positive.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Auto Follow")
	float FowardVelocityThreshold = 100.f;

	/**Velocity threshold used in conjunction with the car's current reverse input state to determine whether the vehicle is currently reversing or not. We adjust the camera's follow behavior based on this. You want to keep this value negative.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Auto Follow")
	float ReverseVelocityThreshold = -100.f;

	/** Speed Threshold that must be surpassed before lazy follow camera behavior kicks in*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Auto Follow")
	float LazyFollowSpeedThreshold = 100.0f;

	/**Minimum amount of time that the "Full Follow" behavior may be considered active after releasing the handbrake. Used to enhance the feel of sharp handbrake turns. Too low of a number can result in hard to follow handbrake turns, while too high of a number may result in overzealous full follow behavior.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Auto Follow")
	float HandbrakeFullFollowPostReleaseTime = 1.0f;


	// Look At Point Members

	/**Minimum Look At Point Distance from vehicle. A larger distance will make the camera look farther ahead of the vehicle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float MinLookAtPointDistance = 250.0f;

	/**Alternative LookAtWorldSpace interpolator for the driving cam - helps smooth out the sudden changes in camera behavior */
	FDoubleIIRInterpolatorVector LookatWorldSpaceDoubleInterpolator = FDoubleIIRInterpolatorVector(8.f, 8.f);

	/**Speeds that the LookatWorldSpaceDoubleInterpolator will use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	FVector2D DrivingLookatWorldSpaceInterpSpeed = FVector2D(12.f, 20.f);

	/**Max scalar to multiply DrivingLookatWorldInterpSpeeds depending on vehicle speeds. This is helpful if at high speeds the default interp speeds are struggling to catch up. Leave at 1.0f if you'd like interp speeds to be unaffected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At|DrivingLookAtWorldSpaceInterpolator Speeds Scales")
	float DrivingLookatWorldSpaceInterpMaxSpeedScale = 1.0f;

	/**The speeds in MPH at which the speed scale will start to apply. If you have a range of 80-120 mph and DrivingLookatWorldSpaceInterpMaxSpeedScale is at 2.0, then interp speeds will be scaled like so: 80mph - x1 | 100 mph - x1.5 | 120 mph - x2 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At|DrivingLookAtWorldSpaceInterpolator Speeds Scales")
	FVector2D DrivingLookatWorldSpaceInterpScaleRange = FVector2D(40.f, 80.f);

	/**How far back to push the Camera's look at point when the vehicle is reversing. Also used for Forward->Reverse camera transitions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float LookAtReverseOffset = 0.0f;

	/**How far forward to push the Camera's look at point when the vehicle is transitioning from reverse behavior -> forward behavior*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float LookAtForwardTransitionOffset = 0.0f;

	/**Interpolator that manages the vector multiplier applied to the vehicle's forward vector that determines the look at point. This helps smooth out sudden LookAt changes.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	FDoubleIIRInterpolatorFloat LookatVectorMultiplierInterpolator = FDoubleIIRInterpolatorFloat(3.f, 6.f);

	/**Time it takes to transition from normal LookAtInterpSpeeds to instant interp speeds when manual camera control is active. Used with LookAtManualInterpSpeedTransitionScalar to provide a smooth transition to the instant interp speed.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float LookAtManualFollowInterpSpeedTransitionTime = 0.5f;

	/**Time it takes to transition from instant LookAtInterpSpeeds to normal interp speeds when lazy follow camera control is active. Used with LookAtManualInterpSpeedTransitionScalar to provide a smooth transition back to normal interp speeds.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float LookAtLazyFollowInterpSpeedTransitionTime = 1.0f;

	/** Scalar applied to the vehicle's current LookAt interp speeds to determine the Look At Interp speed goal when transitioning to the "instant" interp speed in manual camera mode. Used with LookAtManualFollowInterpSpeedTransitionTime to provide a smooth transition to the instant interp speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float LookAtManualInterpSpeedTransitionScalar = 5.0f;

	/** Curve that applies a lateral offset to the Camera's look at point depending on how long the steering threshold has been met. X = Time Steering Threshold Passed, Y = Lateral offset (depends on steering direction) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	UCurveFloat* SteeringToLookAtLateralOffset;

	/** Scales the values in the SteeringToLookAtLateralOffset curve. X = Scales Time Axis, Y = Scales Lateral Offset Axis*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	FVector2D SteeringToLookAtLateralOffsetScales = FVector2D(1.f, 1.f);

	/**Steering threshold to meet so that the SteeringToLookAtLateralOffset curve comes into effect.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	float LookAtLateralOffsetSteeringThreshold = 0.75f;

	/**Rise and Fall Interpolation speeds for the LateralLookAtOffset Target . X = Rise Speed (Steering is triggering lateral offset), Y = Fall Speed. (Weak or zero steering)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Look At")
	FVector2D LateralLookAtOffsetRiseAndFallSpeeds = FVector2D(200.0f, 40.0f);


	// Pivot Rotation Members

	/**Speeds that the PivotRotDoubleInterpolator will use when driving behavior*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation")
	FVector2D DrivingPivotRotInterpSpeed = FVector2D(12.f, 20.f);

	/**Speeds that the PivotRotDoubleInterpolator will use when the camera enters full follow mode (happens during handbrake turns)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation")
	FVector2D DrivingFullFollowPivotRotInterpSpeed = FVector2D(12.f, 20.f);

	/**Speeds that the PivotRotDoubleInterpolator will use when transitioning from manual cam to auto cam*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation|Manual To Auto Camera Transition")
	FVector2D ManualToAutoCameraPivotRotInterpSpeed = FVector2D(2.f, 4.f);

	/**Yaw threshold that must be met before during a manual to auto camera transition before normal pivot rot interp speeds kick in again*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation|Manual To Auto Camera Transition")
	float ManualToAutoCameraTransitionalYawThreshold = 10.0f;

	/**Speeds that the PivotRotDoubleInterpolator will use when the camera is switching from the forward to reverse camera following behavior*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation|Forward Reverse Transition")
	FVector2D ForwardToReversePivotRotInterpSpeed = FVector2D(12.f, 20.f);

	/**Speeds that the PivotRotDoubleInterpolator will use when the camera is switching from the reverse to forward camera following behavior*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation|Forward Reverse Transition")
	FVector2D ReverseToForwardPivotRotInterpSpeed = FVector2D(12.f, 20.f);

	/**Yaw threshold that must be met before during a forward-reverse camera transition before normal pivot rot interp speeds kick in again*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation|Forward Reverse Transition")
	float PivotRotForwardReverseTransitionalYawThreshold = 10.0f;

	/**Time for pivot rot speed to change back to normal speeds upon completing a transition. This helps reduce camera snappiness when speeds readjust.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Pivot Rotation|Forward Reverse Transition")
	float PivotRotSpeedChangeTimeToTransition = 1.0f;


	//FOV related members

	/** False: Use normal FOV at all times, True: Use FOV_SpeedAdjustmentCurve to determine FOV at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|FOV")
	bool bUseFOVSpeedAdjustmentCurve = false;

	/** Curve that applies a dynamic FOV depending on the player's current velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|FOV", meta = (EditCondition = bUseFOVSpeedAdjustmentCurve))
	UCurveFloat* FOV_SpeedAdjustmentCurve;

	/** Speeds (in MPH) between which the FOV will adjust to at runtime when using the FOV_SpeedAdjustmentCurve*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|FOV", meta = (EditCondition = bUseFOVSpeedAdjustmentCurve))
	FVector2D FOV_SpeedAdjustment_SpeedRange;


	// Yaw related members

	/** False: Don't apply any Yaw modification based on steering state, True: Use SpeedToYawMultiplierCurve to apply extra Yaw to the camera when turning. This is applied on top of any 'Look At' Camera settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Yaw")
	bool bUseSteeringYawModifiers = false;

	/** Curve that applies a Yaw multiplier based on the vehicles current speed. X = Speed, Y = Yaw Multiplier. The multiplier is applied to the current steering rate which has a range from -1 (leftmost), to 1 (rightmost) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Yaw", meta = (EditCondition = bUseSteeringYawModifiers))
	UCurveFloat* SpeedToYawMultiplierCurve;

	/** Interpolator for smoothing yaw modifications. Too high of a speed may result in very sudden yaw changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Yaw", meta = (EditCondition = bUseSteeringYawModifiers))
	FIIRInterpolatorFloat SteeringYawModifierInterpolator = FIIRInterpolatorFloat(3.0f);

	/** Scales Final Multiplier value of SpeedToYawMultiplierCurve*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Yaw", meta = (EditCondition = bUseSteeringYawModifiers))
	float SpeedToYawMultiplierCurve_Scale = 1.0f;

	/** Speeds (in MPH) between which the Yaw multiplier will adjust to at runtime when using the SpeedToYawMultiplierCurve*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Yaw", meta = (EditCondition = bUseSteeringYawModifiers))
	FVector2D SpeedToYawMultiplierCurve_SpeedRange;

	/** Yaw Modifier that is added on top of all current Yaw modifiers when using the handbrake. Scales with current steering value and is only applicable when the Vehicle is actually moving. Can be used to apply more extreme camera rotations when drifting.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Yaw", meta = (EditCondition = bUseSteeringYawModifiers))
	float HandbrakeYawModifier = 0.0f;


	// Roll related members

	/** False: Don't apply any Roll modification based on steering state, True: Use SpeedToRollMultiplierCurve to apply extra Roll to the camera when turning. This is applied on top of any 'Look At' Camera settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Roll")
	bool bUseSteeringRollModifiers = false;

	/** Curve that applies a Roll multiplier based on the vehicles current speed. X = Speed, Y = Roll Multiplier. The multiplier is applied to the current steering rate which has a range from -1 (leftmost), to 1 (rightmost) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Roll", meta = (EditCondition = bUseSteeringRollModifiers))
	UCurveFloat* SpeedToRollMultiplierCurve;

	/** Interpolator for smoothing Roll modifications.*/
	FIIRInterpolatorFloat SteeringRollModifierInterpolator = FIIRInterpolatorFloat(3.0f);

	/** Rise and fall speeds for the SteeringRollModifier interpolator. Rise = Steering Magnitude > Last frame Magnitude, Fall = Steering Magnitude < Last Frame Magnitude */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Roll", meta = (EditCondition = bUseSteeringRollModifiers))
	FVector2D SteeringRollModifierRiseAndFallSpeeds = FVector2D(0.5f, 2.0f);

	/** Scales Final Multiplier value of SpeedToRollMultiplierCurve*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Roll", meta = (EditCondition = bUseSteeringRollModifiers))
	float SpeedToRollMultiplierCurve_Scale = 1.0f;

	/** Speeds (in MPH) between which the Roll multiplier will adjust to at runtime when using the SpeedToRollMultiplierCurve*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Roll", meta = (EditCondition = bUseSteeringRollModifiers))
	FVector2D SpeedToRollMultiplierCurve_SpeedRange;

	/** Roll Modifier that is added on top of all current Roll modifiers when using the handbrake. Scales with current steering value and is only applicable when the Vehicle is actually moving. Can be used to apply more extreme camera rotations when drifting.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Roll", meta = (EditCondition = bUseSteeringRollModifiers))
	float HandbrakeRollModifier = 0.0f;

	// Air Time members
	/** Additive offset to apply to the Camera To Pivot Offset when the vehicle is in the air */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Air Time")
	FVector AerialCameraToPivotOffset;

	/** Double Interp Speeds to use when transitioning TO the air.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Air Time")
	FVector2D AerialCameraToPivotOffsetAirTransitionSpeeds = FVector2D(4.0f, 8.0f);

	/** Double Interp Speeds to use when transitioning TO the ground.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving Camera Adjustments|Air Time")
	FVector2D AerialCameraToPivotOffsetGroundTransitionSpeeds = FVector2D(4.0f, 8.0f);

	/** Interpolator for going in and out of the aerial camera to pivot offset*/
	FDoubleIIRInterpolatorVector AerialCameraToPivotOffsetInterpolator = FDoubleIIRInterpolatorVector(4.0f, 8.0f);


	private:

	/** Cache previous steering values for comparison */
	float PreviousSteeringMagnitude = 0.0f;


	/** Original CameraToPivot value when camera mode instance was first created. Cached since camera mode instances can be reused, so knowing the original value is necessary to properly apply offsets */
	FVector DefaultCameraToPivot;

	/** Original LookatOffsetLocal value when camera mode instance was first created. Cached since camera mode instances can be reused,  so knowing the original value is necessary to properly apply offsets */
	FVector DefaultLookatOffsetLocal;

	/** Original LateralLookatOffset value when camera mode instance was first created. Cached so we have a reference point to return to when managing look at point offsets */
	float DefaultLateralLookAtOffset = 0.0f;

	/** Since CameraMode instances can be reused OnBecomeActive can be called multiple time in this object's lifespan. This flag is kept around to only cache default values on the first call of OnBecomeActive */
	bool bDefaultCameraModeValuesSaved = false;


	/** Handle for timer that stops the camera from returning to the normal follow laziness for as long as it is active */
	FTimerHandle HandbrakeReleaseTimer;

	/** Cache of previous handbrake state to tell if the handbrake was freshly activated */
	bool bPrevHandbrakeState = false;


	/** If true, the camera's reverse follow behavior is currently active */
	bool bReverseFollowActive = false;

	/** Caches state of previous frames reverse follow flag to help trigger transitions */
	bool bPrevReverseFollowActive = false;

	
	/** If true, a camera transition between forward and reverse behaviors is currently active */
	bool bTransitionBetweenForwardAndReverseActive = false;

	/** If true, the rotational portion of the forward/reverse transition has started */
	bool bTransitionRotationStarted = false;

	/** If true, Pivot Rotation interpolator speeds are smoothly updated to avoid a jarring camera rotational speed change */
	bool bExitingTransitionalPivotRotInterpSpeeds = false;

	/** Time spent transitioning the Pivot Rotation Interpolation speeds */
	float PivotRotSpeedChangePassedTransitionTime = 0.0f;

	/** Currently active Pivot Rotation Interpolation Speeds */
	FVector2D ActiveTransitionalPivotRotInterpSpeeds;

	
	/** If true, manual camera control mode is active */
	bool bUsingManualCam = false;

	/** If true, a transition between manual and auto follow camera behaviors is currently active */
	bool bTransitionBetweenManualToAutoCameraActive = false;

	/** Amount of time spent using the manual camera behavior. Useful for knowing when to transition to auto follow camera behaviors */
	float TimeSpentInManualCameraControl = 0.0f;

	/** Amount of time spent in the auto follow camera behavior. Useful for handling transition upon leaving manual mode */
	float TimeSpentInAutomaticCameraControl = 0.0f;


	/** Copy of the driving state of the vehicle that his camera mode instance is tied to. Driving State information is used for a variety of camera behaviors */
	FCitySampleDrivingState DrivingState;

	/** Default CameraToPivot translation after the optional vehicle - specific offset is applied */
	FVector AdjustedCameraToPivot;

};

