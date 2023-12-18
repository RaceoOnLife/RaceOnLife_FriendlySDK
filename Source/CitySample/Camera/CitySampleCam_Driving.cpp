// Copyright Epic Games, Inc. All Rights Reserved.


#include "CitySampleCam_Driving.h"

#include "ChaosVehicleMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/CitySampleVehicleBase.h"
#include "VehicleUtility.h"

static TAutoConsoleVariable<bool> CVarDrawDrivingCamDebug(
	TEXT("Camera.DrawDrivingCamDebug"),
	false,
	TEXT("Toggle for displaying driving camera debug info"),
	ECVF_Default);

void UCitySampleCam_Driving::UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, struct FTViewTarget& OutVT)
{
	//Obtain all the data we need from the vehicle
	float TimeSpentSteering = 0.0f;
	if (const ACitySampleVehicleBase* Vehicle = Cast<ACitySampleVehicleBase>(ViewTarget))
	{
		DrivingState = Vehicle->GetDrivingState();
		TimeSpentSteering = Vehicle->GetTimeSpentSharpSteering();
	}

	// Update Interpolation Speeds
	const float CurrentSpeed = ViewTarget ? Chaos::CmSToMPH(ViewTarget->GetVelocity().Size()) : 0.0f;

	const float SharpLookAtSpeedAlpha = FMath::Clamp(FMath::GetRangePct(DrivingLookatWorldSpaceInterpScaleRange, CurrentSpeed), 0.0f, 1.0f);
	const float SharpLookAtPrimaryInterpSpeed = FMath::Lerp(DrivingLookatWorldSpaceInterpSpeed.X, DrivingLookatWorldSpaceInterpSpeed.X * DrivingLookatWorldSpaceInterpMaxSpeedScale, SharpLookAtSpeedAlpha);
	const float SharpLookAtIntermediateInterpSpeed = FMath::Lerp(DrivingLookatWorldSpaceInterpSpeed.Y, DrivingLookatWorldSpaceInterpSpeed.Y * DrivingLookatWorldSpaceInterpMaxSpeedScale, SharpLookAtSpeedAlpha);

	LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed = SharpLookAtPrimaryInterpSpeed;
	LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed = SharpLookAtIntermediateInterpSpeed;

	// Update LookatOffsetLocal with relevant lateral offset values
	if (SteeringToLookAtLateralOffset != nullptr && FMath::Abs(DrivingState.Steering) >= LookAtLateralOffsetSteeringThreshold)
	{
		const float Direction = DrivingState.Steering > 0 ? 1.f : -1.f;
		const float LateralLookAtOffsetTarget = SteeringToLookAtLateralOffset->GetFloatValue(TimeSpentSteering / SteeringToLookAtLateralOffsetScales.X) * SteeringToLookAtLateralOffsetScales.Y * Direction;

		LookatOffsetLocal.Y = FMath::FInterpConstantTo(LookatOffsetLocal.Y, LateralLookAtOffsetTarget, DeltaTime, LateralLookAtOffsetRiseAndFallSpeeds.X);
	}
	else
	{
		LookatOffsetLocal.Y = FMath::FInterpConstantTo(LookatOffsetLocal.Y, DefaultLateralLookAtOffset, DeltaTime, LateralLookAtOffsetRiseAndFallSpeeds.Y);
	}

	// Update Air-time logic
	if (DrivingState.bNoWheelsOnGround)
	{
		AerialCameraToPivotOffsetInterpolator.PrimaryInterpSpeed = AerialCameraToPivotOffsetAirTransitionSpeeds.X;
		AerialCameraToPivotOffsetInterpolator.IntermediateInterpSpeed = AerialCameraToPivotOffsetAirTransitionSpeeds.Y;

		const FVector GoalCameraToPivot = AdjustedCameraToPivot + AerialCameraToPivotOffset;
		CameraToPivot.SetLocation(AerialCameraToPivotOffsetInterpolator.Eval(GoalCameraToPivot, DeltaTime));
	}
	else
	{
		AerialCameraToPivotOffsetInterpolator.PrimaryInterpSpeed = AerialCameraToPivotOffsetGroundTransitionSpeeds.X;
		AerialCameraToPivotOffsetInterpolator.IntermediateInterpSpeed = AerialCameraToPivotOffsetGroundTransitionSpeeds.Y;

		CameraToPivot.SetLocation(AerialCameraToPivotOffsetInterpolator.Eval(AdjustedCameraToPivot, DeltaTime));
	}

#if ENABLE_DRAW_DEBUG
	if (CVarDrawDrivingCamDebug.GetValueOnGameThread())
	{
		const FString DisplayString1 = FString::Printf(TEXT("Time Spent past steering threshold: %f"), TimeSpentSteering);
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Blue, *DisplayString1);

		const FString DisplayString2 = FString::Printf(TEXT("Current Lateral Look At Offset: %f"), LookatOffsetLocal.Y);
		GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Blue, *DisplayString2);

		const FString DisplayString3 = FString::Printf(TEXT("Camera Laziness: %f"), LazyFollowLaziness);
		GEngine->AddOnScreenDebugMessage(3, 0.0f, FColor::Blue, *DisplayString3);
	}
#endif  //ENABLE_DRAW_DEBUG

	Super::UpdateCamera(ViewTarget, CineCamComp, DeltaTime, OutVT);
}

void UCitySampleCam_Driving::OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode)
{
	Super::OnBecomeActive(ViewTarget, PreviouslyActiveMode);

	if (bDefaultCameraModeValuesSaved == false)
	{
		DefaultCameraToPivot = CameraToPivot.GetLocation();
		DefaultLookatOffsetLocal = LookatOffsetLocal;

		DefaultLateralLookAtOffset = LookatOffsetLocal.Y;

		bDefaultCameraModeValuesSaved = true;
	}

	LazyFollowLaziness = DrivingLazyFollowLaziness;

	LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed = DrivingLookatWorldSpaceInterpSpeed.X;
	LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed = DrivingLookatWorldSpaceInterpSpeed.Y;

	PivotRotInterpolator.PrimaryInterpSpeed = DrivingPivotRotInterpSpeed.X;
	PivotRotInterpolator.IntermediateInterpSpeed = DrivingPivotRotInterpSpeed.Y;

	if (const ACitySampleVehicleBase* Vehicle = Cast<ACitySampleVehicleBase>(ViewTarget))
	{
		if (Vehicle->ShouldOverrideCameraToPivotOffset())
		{
			AdjustedCameraToPivot = DefaultCameraToPivot + Vehicle->GetCameraToPivotOffsetOverride();
			CameraToPivot.SetLocation(AdjustedCameraToPivot);
		}
		else
		{
			AdjustedCameraToPivot = DefaultCameraToPivot;
		}

		if (Vehicle->ShouldOverrideLookAtPointOffset())
		{
			FVector FinalLookAtPointOffset = DefaultLookatOffsetLocal + Vehicle->GetLookAtPointOffsetOverride();
			LookatOffsetLocal = FinalLookAtPointOffset;
		}
	}

	AerialCameraToPivotOffsetInterpolator.SetInitialValue(AdjustedCameraToPivot);
	AerialCameraToPivotOffsetInterpolator.Reset();
}

void UCitySampleCam_Driving::SkipNextInterpolation()
{
	Super::SkipNextInterpolation();

	LookatWorldSpaceDoubleInterpolator.Reset();
}

float UCitySampleCam_Driving::ComputeFinalFOV(const AActor* ViewTarget) const
{
	// Default finalFOV to member value that lives in parent class 'UCitySampleCameraMode'
	float finalFOV = FOV;

	if (bUseFOVSpeedAdjustmentCurve && FOV_SpeedAdjustmentCurve != nullptr)
	{
		// Currently converting Unreal's speed units of cm/s to MPH for ease of use
		const float CurrentSpeed = ViewTarget ? Chaos::CmSToMPH(ViewTarget->GetVelocity().Size()) : 0.0f;
		const float SpeedAlpha = FMath::Clamp(FMath::GetRangePct(FOV_SpeedAdjustment_SpeedRange, CurrentSpeed), 0.0f, 1.0f);
		finalFOV = FOV_SpeedAdjustmentCurve->GetFloatValue(SpeedAlpha);
	}

	return finalFOV;
}

float UCitySampleCam_Driving::ComputeYawModifier(const AActor* ViewTarget, float DeltaTime) 
{
	if (bUseSteeringYawModifiers && SpeedToYawMultiplierCurve != nullptr)
	{
		if (const ACitySampleVehicleBase* Vehicle = Cast<ACitySampleVehicleBase>(ViewTarget))
		{ 
			if (UChaosVehicleMovementComponent* VehicleMoveComp = Vehicle->GetVehicleMovementComponent())
			{
				// Currently converting Unreal's speed units of cm/s to MPH for ease of use
				const float CurrentSpeed = ViewTarget ? Chaos::CmSToMPH(ViewTarget->GetVelocity().Size()) : 0.0f;
				const float SpeedAlpha = FMath::Clamp(FMath::GetRangePct(SpeedToYawMultiplierCurve_SpeedRange, CurrentSpeed), 0.0f, 1.0f);
				float GoalYawModifier = (VehicleMoveComp->GetSteeringInput() * SpeedToYawMultiplierCurve->GetFloatValue(SpeedAlpha) * SpeedToYawMultiplierCurve_Scale);

				// Only apply a potential handbrake modifier if the vehicle is experiencing velocity
				if (VehicleMoveComp->GetHandbrakeInput() && !ViewTarget->GetVelocity().IsNearlyZero())
				{
					GoalYawModifier += HandbrakeYawModifier * VehicleMoveComp->GetSteeringInput();
				}

				return SteeringYawModifierInterpolator.Eval(GoalYawModifier, DeltaTime);
			}
		}
	}

	return 0.0f;
}

float UCitySampleCam_Driving::ComputeRollModifier(const AActor* ViewTarget, float DeltaTime)
{
	if (bUseSteeringRollModifiers && SpeedToRollMultiplierCurve != nullptr)
	{
		if (const ACitySampleVehicleBase* Vehicle = Cast<ACitySampleVehicleBase>(ViewTarget))
		{
			if (UChaosVehicleMovementComponent* VehicleMoveComp = Vehicle->GetVehicleMovementComponent())
			{
				// Determine whether or not the steering magnitude has increased or decreased, that we we can tweak our interp speeds accordingly
				const float CurrentSteeringInput = VehicleMoveComp->GetSteeringInput();
				const float CurrentSteeringMagnitude = FMath::Abs(CurrentSteeringInput);
				if (CurrentSteeringMagnitude > PreviousSteeringMagnitude)
				{
					SteeringRollModifierInterpolator.InterpSpeed = SteeringRollModifierRiseAndFallSpeeds.X;
				}
				else if (CurrentSteeringMagnitude < PreviousSteeringMagnitude)
				{
					SteeringRollModifierInterpolator.InterpSpeed = SteeringRollModifierRiseAndFallSpeeds.Y;
				}
				PreviousSteeringMagnitude = CurrentSteeringMagnitude;

				// Currently converting Unreal's speed units of cm/s to MPH for ease of use
				const float CurrentSpeed = ViewTarget ? Chaos::CmSToMPH(ViewTarget->GetVelocity().Size()) : 0.0f;
				const float SpeedAlpha = FMath::Clamp(FMath::GetRangePct(SpeedToRollMultiplierCurve_SpeedRange, CurrentSpeed), 0.0f, 1.0f);
				float GoalRollModifier = (CurrentSteeringInput * SpeedToRollMultiplierCurve->GetFloatValue(SpeedAlpha) * SpeedToRollMultiplierCurve_Scale);

				// Only apply a potential handbrake modifier if the vehicle is experiencing significant velocity (this can be further smoothed out with a curve or speed range)
				if (VehicleMoveComp->GetHandbrakeInput() && !ViewTarget->GetVelocity().IsNearlyZero(10.0f))
				{
					GoalRollModifier += HandbrakeRollModifier * CurrentSteeringInput;
				}

				return SteeringRollModifierInterpolator.Eval(GoalRollModifier, DeltaTime);
			}
		}
	}

	return 0.0f;
}

void UCitySampleCam_Driving::AdjustAutoFollowMode(const AActor* ViewTarget)
{
	const bool bBrakeActive = DrivingState.Brake > KINDA_SMALL_NUMBER;

	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		bool bIsDrifting = false;
		if (const ACitySampleVehicleBase* VehicleBase = Cast<ACitySampleVehicleBase>(ViewTarget))
		{
			bIsDrifting = VehicleBase->IsVehicleDrifting();
		}

		if (!DrivingState.bHandbrakeOn && bPrevHandbrakeState)
		{
			GameInstance->GetTimerManager().SetTimer(HandbrakeReleaseTimer, HandbrakeFullFollowPostReleaseTime, false);
		}

		// Determine if vehicle should be using Lazy Follow or manual camera follow
		AutoFollowMode = FMath::Abs(DrivingState.ForwardSpeed) > LazyFollowSpeedThreshold ? ECameraAutoFollowMode::LazyFollow : ECameraAutoFollowMode::None;

		// Enter Lazy follow with reverse laziness when we acknowledge that the vehicle is reversing
		if (DrivingState.ForwardSpeed <= ReverseVelocityThreshold && bBrakeActive)
		{
			LazyFollowLaziness = ReverseCamLazyFollowLaziness;
			bReverseFollowActive = true;
		}
		// Enter Full follow if the handbrake is active and the car is moving forward fast enough, or if the car is moving backwards but not past the reverse threshold (stops lazy follow from kicking in too early)
		else if ((DrivingState.bHandbrakeOn && DrivingState.ForwardSpeed > FowardVelocityThreshold) || (DrivingState.ForwardSpeed < KINDA_SMALL_NUMBER && !bReverseFollowActive))
		{
			// 0.0f laziness results in a Full follow behavior
			LazyFollowLaziness = 0.0f;
		}
		// Resume normal laziness if the car is not determined to be drifting + moving forward fast enough + release timer is over + handbrake is no longer active.
		else if (!bIsDrifting && !DrivingState.bHandbrakeOn && DrivingState.ForwardSpeed > FowardVelocityThreshold && !GameInstance->GetTimerManager().IsTimerActive(HandbrakeReleaseTimer))
		{
			LazyFollowLaziness = DrivingLazyFollowLaziness;
			bReverseFollowActive = false;
		}
	}

	bPrevHandbrakeState = DrivingState.bHandbrakeOn;
}

void UCitySampleCam_Driving::ComputePredictiveLookAtPoint(FVector& LookAtPointOutput, const AActor* ViewTarget, float DeltaTime)
{
	float VectorMultiplier = 0.0f;

	/*If the LazyFollowDelay is active it means the player has activated manual camera inputs of some kind. We 0 out our interp speeds to keep the player vehicle focused no matter what speed they're going at.
	  The zeroed-out speeds will be overwritten next update in UCitySampleCam_Driving::UpdateCamera, where we constantly scale up our interp speeds to adjust to player speeds.*/
	if (LazyFollowDelay_TimeRemaining > 0.0f)
	{
		TimeSpentInAutomaticCameraControl = 0.0f;
		TimeSpentInManualCameraControl += DeltaTime;

		// If we're past our transition time, simply force our interps to be instant. Otherwise we lerp towards a scaled up interp speed in hopes of hiding the transition to a instant speed.
		if (TimeSpentInManualCameraControl > LookAtManualFollowInterpSpeedTransitionTime)
		{
			LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed = 0.0f;
			LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed = 0.0f;
		}
		else
		{
			//Quick note, since the LookatWorldSpaceDoubleInterpolator speeds scales to the vehicle's velocity, we scale our interp goal so that the smoothness factor carries over to our range of look-at interp speeds.
			const float LookAtWorldSpaceInterpAlpha = FMath::Clamp(FMath::GetRangePct(FVector2D(0.0f, LookAtManualFollowInterpSpeedTransitionTime), TimeSpentInManualCameraControl), 0.0f, 1.0f);
			LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed = FMath::Lerp(LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed, LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed * LookAtManualInterpSpeedTransitionScalar, LookAtWorldSpaceInterpAlpha);
			LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed = FMath::Lerp(LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed, LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed * LookAtManualInterpSpeedTransitionScalar, LookAtWorldSpaceInterpAlpha);
		}

		VectorMultiplier = MinLookAtPointDistance;
	}
	/*If the auto follow camera is in control then most of the time we just need to calculate our forward vector multiplier based on the vehicle's velocity.
	  However, if the camera is currently in the reverse follow behavior or in mid-transition between forward and reverse follow we need to adjust the vector multiplier accordingly to ensure
	  that the look at point is in proper locations during these behaviors.*/
	else
	{
		TimeSpentInManualCameraControl = 0.0f;
		TimeSpentInAutomaticCameraControl += DeltaTime;

		// Transition from near instant manual camera interp speeds back to whatever is being computed in UCitySampleCam_Driving::UpdateCamera to smooth out the transition to scaled interp speeds
		const bool bInTransitionFromManualCamera = TimeSpentInAutomaticCameraControl < LookAtLazyFollowInterpSpeedTransitionTime;
		if (bInTransitionFromManualCamera)
		{
			const float LookAtWorldSpaceInterpAlpha = FMath::Clamp(FMath::GetRangePct(FVector2D(0.0f, LookAtLazyFollowInterpSpeedTransitionTime), TimeSpentInAutomaticCameraControl), 0.0f, 1.0f);
			LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed = FMath::Lerp(LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed * LookAtManualInterpSpeedTransitionScalar, LookatWorldSpaceDoubleInterpolator.PrimaryInterpSpeed, LookAtWorldSpaceInterpAlpha);
			LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed = FMath::Lerp(LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed * LookAtManualInterpSpeedTransitionScalar, LookatWorldSpaceDoubleInterpolator.IntermediateInterpSpeed, LookAtWorldSpaceInterpAlpha);
		}

		VectorMultiplier = FMath::Max(ViewTarget->GetVelocity().Size(), MinLookAtPointDistance);

		if (bReverseFollowActive)
		{
			/* If we're trying to go forward while the reverse follow behavior is active,
			we want to keep the look at point close to the min look at point distance to reduce friction during this camera transition, this multiplier helps with that.*/
			const float ThrottleMultiplier = DrivingState.Throttle > 0 ? 0.0f : 1.0f;
			if (bTransitionBetweenForwardAndReverseActive || bInTransitionFromManualCamera)
			{
				const float ReverseOffset = (1 - FMath::Abs(DrivingState.Steering)) * LookAtReverseOffset;
				VectorMultiplier = MinLookAtPointDistance - (ReverseOffset * ThrottleMultiplier);
			}
			else
			{
				FVector ActorReverseVector = ViewTarget->GetActorForwardVector();
				FVector CameraForwardVector = LastPivotToWorld.GetRotation().GetForwardVector();
				const float AngleCosine = ActorReverseVector.CosineAngle2D(CameraForwardVector);

				/* We take the least extreme of these 2 reverse offsets. This accounts for when the camera is facing the side of the vehicle,
				we need the keep the look at point close to the vehicle to keep it centered on screen in these situations.*/
				const float SteeringReverseOffset = (1 - FMath::Abs(DrivingState.Steering)) * LookAtReverseOffset;
				const float CameraAngleReverseOffset = (FMath::Abs(AngleCosine)) * LookAtReverseOffset;

				VectorMultiplier = MinLookAtPointDistance - (FMath::Min(SteeringReverseOffset, CameraAngleReverseOffset) * ThrottleMultiplier);
			}
		}
		else if (bTransitionBetweenForwardAndReverseActive)
		{
			VectorMultiplier = MinLookAtPointDistance + LookAtForwardTransitionOffset;
		}
	}

	const float FinalVectorMultiplier = LookatVectorMultiplierInterpolator.Eval(VectorMultiplier, DeltaTime);

	LookAtPointOutput = LookAtPointOutput + ViewTarget->GetActorForwardVector() * FinalVectorMultiplier * PredictiveLookatTime;
}

FVector UCitySampleCam_Driving::ComputeWorldLookAtPosition(const FVector IdealWorldLookAt, float DeltaTime)
{
	return LookatWorldSpaceDoubleInterpolator.Eval(IdealWorldLookAt, DeltaTime);
}

FRotator UCitySampleCam_Driving::ComputeSmoothPivotRotation(const FRotator IdealPivotToWorldRot, float DeltaTime)
{
	if (bSkipNextInterpolation)
	{
		PivotRotInterpolator.Reset();
	}

	UpdatePivotRotInterpSpeeds(IdealPivotToWorldRot, DeltaTime);

	const FRotator CurrentRot = PivotRotInterpolator.Eval(IdealPivotToWorldRot, DeltaTime);

#if ENABLE_DRAW_DEBUG
	if (CVarDrawDrivingCamDebug.GetValueOnGameThread())
	{
		const float RemainingRot = FMath::Abs((IdealPivotToWorldRot - CurrentRot).Yaw);

		const FString DisplayString4 = FString::Printf(TEXT("Rot Diff: %f"), RemainingRot);
		GEngine->AddOnScreenDebugMessage(4, 0.0f, FColor::Blue, *DisplayString4);

		const FString DisplayString5 = FString::Printf(TEXT("Primary Rot Interp Speed: %f,  Intermediate Rot Interp Speed: %f"), PivotRotInterpolator.PrimaryInterpSpeed, PivotRotInterpolator.IntermediateInterpSpeed);
		GEngine->AddOnScreenDebugMessage(5, 0.0f, FColor::Blue, *DisplayString5);
	}
#endif  //ENABLE_DRAW_DEBUG

	return CurrentRot;
}

void UCitySampleCam_Driving::UpdatePivotRotInterpSpeeds(const FRotator& GoalRotation, float DeltaTime)
{
	// Determine if a manual to auto camera transition is still in progress
	if (bTransitionBetweenManualToAutoCameraActive)
	{
		const float RemainingYaw = FMath::Abs((PivotRotInterpolator.GetCurrentValue() - GoalRotation).Yaw);

		if (RemainingYaw <= ManualToAutoCameraTransitionalYawThreshold)
		{
			bTransitionBetweenManualToAutoCameraActive = false;

			//Prep these members so we can smoothly lerp back to normal interp speeds
			bExitingTransitionalPivotRotInterpSpeeds = true;
			PivotRotSpeedChangePassedTransitionTime = 0.0f;
		}
	}

	// Determine if a forward/reverse camera transition is still in progress
	if (bTransitionBetweenForwardAndReverseActive)
	{
		const float RemainingYaw = FMath::Abs((PivotRotInterpolator.GetCurrentValue() - GoalRotation).Yaw);

		if (!bTransitionRotationStarted && RemainingYaw > PivotRotForwardReverseTransitionalYawThreshold)
		{
			bTransitionRotationStarted = true;
		}

		if (RemainingYaw <= PivotRotForwardReverseTransitionalYawThreshold && bTransitionRotationStarted)
		{
			bTransitionBetweenForwardAndReverseActive = false;
			bTransitionRotationStarted = false;

			//Prep these members so we can smoothly lerp back to normal interp speeds
			bExitingTransitionalPivotRotInterpSpeeds = true;
			PivotRotSpeedChangePassedTransitionTime = 0.0f;
		}
	}

	FVector2D GoalInterpSpeeds = FVector2D(PivotRotInterpolator.PrimaryInterpSpeed, PivotRotInterpolator.IntermediateInterpSpeed);

	// This function will check driving & camera parameters to determine if a transition should take place, and handles setting the goal speeds appropriately.
	CheckForRotationalTransitionTriggers(GoalInterpSpeeds);

	// If no transition is currently active, then use the normal Full follow and normal lazy follow speeds as needed
	if (!bTransitionBetweenForwardAndReverseActive && !bTransitionBetweenManualToAutoCameraActive)
	{
		if (LazyFollowLaziness <= 0.0f)
		{
			GoalInterpSpeeds = FVector2D(DrivingFullFollowPivotRotInterpSpeed.X, DrivingFullFollowPivotRotInterpSpeed.Y);
		}
		else
		{
			GoalInterpSpeeds = FVector2D(DrivingPivotRotInterpSpeed.X, DrivingPivotRotInterpSpeed.Y);
		}
	}

	// If were exiting transitional pivot rot speeds due to meeting the yaw threshold, we smoothly lerp back to our default speeds to avoid a jarring change in rotational camera speed
	if (bExitingTransitionalPivotRotInterpSpeeds)
	{
		PivotRotSpeedChangePassedTransitionTime += DeltaTime;
		const float PivotRotInterpSpeedAlpha = FMath::Clamp(FMath::GetRangePct(FVector2D(0, PivotRotSpeedChangeTimeToTransition), PivotRotSpeedChangePassedTransitionTime), 0.0f, 1.0f);
		PivotRotInterpolator.PrimaryInterpSpeed = FMath::Lerp(ActiveTransitionalPivotRotInterpSpeeds.X, GoalInterpSpeeds.X, PivotRotInterpSpeedAlpha);
		PivotRotInterpolator.IntermediateInterpSpeed = FMath::Lerp(ActiveTransitionalPivotRotInterpSpeeds.Y, GoalInterpSpeeds.Y, PivotRotInterpSpeedAlpha);

		if (PivotRotSpeedChangePassedTransitionTime >= PivotRotSpeedChangeTimeToTransition)
		{
			// Transition is over, no need to lerp, we can apply pivot rot speed changes instantly until another valid transitional period is detected
			bExitingTransitionalPivotRotInterpSpeeds = false;
		}
	}
	else
	{
		PivotRotInterpolator.PrimaryInterpSpeed = GoalInterpSpeeds.X;
		PivotRotInterpolator.IntermediateInterpSpeed = GoalInterpSpeeds.Y;
	}
}

void UCitySampleCam_Driving::CheckForRotationalTransitionTriggers(FVector2D& GoalPivotRotInterpSpeeds)
{
	//bReverseFollowActive is updated in UCitySampleCam_Driving::AdjustAutoFollowMode
	if (bPrevReverseFollowActive != bReverseFollowActive)
	{
		TriggerTransitionBetweenForwardAndReverseBehavior(GoalPivotRotInterpSpeeds);
	}

	bPrevReverseFollowActive = bReverseFollowActive;

	// Determine if a manual to auto camera mode transition is about to start
	// Check the lazy follow delay (manual if > 0) and only trigger when going from Manual to Auto
	const bool bWasUsingManualCam = bUsingManualCam;
	if (LazyFollowDelay_TimeRemaining > 0.0f)
	{
		bUsingManualCam = true;
	}
	else
	{
		bUsingManualCam = false;

		if (bWasUsingManualCam)
		{
			TriggerTransitionBetweenManualAndAutoBehavior(GoalPivotRotInterpSpeeds);
		}
	}
}

void UCitySampleCam_Driving::TriggerTransitionBetweenForwardAndReverseBehavior(FVector2D& GoalPivotRotInterpSpeeds)
{
	// Prioritize Manual To Auto Camera transition speeds, if another transition is already active we'll trust it to handle the situation
	if (!bTransitionBetweenManualToAutoCameraActive)
	{
		if (bReverseFollowActive)
		{
			GoalPivotRotInterpSpeeds = ForwardToReversePivotRotInterpSpeed;
		}
		else
		{
			GoalPivotRotInterpSpeeds = ReverseToForwardPivotRotInterpSpeed;
		}

		ActiveTransitionalPivotRotInterpSpeeds = GoalPivotRotInterpSpeeds;

		bTransitionBetweenForwardAndReverseActive = true;
	}
}

void UCitySampleCam_Driving::TriggerTransitionBetweenManualAndAutoBehavior(FVector2D& GoalPivotRotInterpSpeeds)
{
	GoalPivotRotInterpSpeeds = ManualToAutoCameraPivotRotInterpSpeed;
	ActiveTransitionalPivotRotInterpSpeeds = GoalPivotRotInterpSpeeds;

	// Manual to Auto transition takes precedence and effectively overwrites a forward/reverse one
	bTransitionBetweenForwardAndReverseActive = false;

	bTransitionBetweenManualToAutoCameraActive = true;
}
