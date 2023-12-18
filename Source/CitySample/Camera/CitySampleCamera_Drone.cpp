// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCamera_Drone.h"
#include "Camera/CitySamplePlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Game/CitySamplePlayerController.h"
#include "CineCameraComponent.h"
#include "../Private/KismetTraceUtils.h"
#include "Stats/Stats2.h"
#include "Curves/CurveVector.h"
#include "HoverDronePawnBase.h"
#include "HoverDroneMovementComponent.h"

UCitySampleCamera_Drone::UCitySampleCamera_Drone()
{
	FOV = 75.f;

	DroneTiltInterpSpeed_Accel = 10.f;
	DroneTiltInterpSpeed_Decel = 10.f;
	TiltUpVector = FVector(0, 0, 10000.f);
}

void UCitySampleCamera_Drone::UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, FTViewTarget& OutVT)
{
	if (!PlayerCamera || !PlayerCamera->PCOwner)
	{
		return;
	}
	
	UWorld const* const World = ViewTarget ? ViewTarget->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	auto GetPawnAndVelocity = [this, ViewTarget]()
	{
		TOptional<float> OverrideDeltaTime;
		TOptional<FVector> OutVelocity;
		APawn* OutPawn = nullptr;
		
		if (AHoverDronePawnBase* DronePawn = Cast<AHoverDronePawnBase>(ViewTarget))
		{
			if (UHoverDroneMovementComponent const* const HoverMoveComponent = Cast<UHoverDroneMovementComponent>(DronePawn->GetMovementComponent()))
			{
				OutVelocity = HoverMoveComponent->MeasuredVelocity;
			}

			OutPawn = DronePawn;
		}
		
		return TTuple<APawn*, TOptional<FVector>, TOptional<float>>(OutPawn, OutVelocity, OverrideDeltaTime);
	};
	
	const TTuple<APawn*, TOptional<FVector>, TOptional<float>> DronePawnAndVelocity = GetPawnAndVelocity();
	APawn* const DronePawn = DronePawnAndVelocity.Get<0>();
	const TOptional<FVector> MeasuredVelocity = DronePawnAndVelocity.Get<1>();
	const TOptional<float> OverrideDeltaTime = DronePawnAndVelocity.Get<2>();

	if (OverrideDeltaTime.IsSet() && OverrideDeltaTime.GetValue() > KINDA_SMALL_NUMBER)
	{
		DeltaTime = OverrideDeltaTime.GetValue();
	}

	// if the pawn is pending destroy, the position of the pawn gets reset and camera will be teleported to weird positions.
	// so just use the old FOV without any update
	const bool DronePawnValid = IsValid(DronePawn);
	if (!DronePawnValid)
	{
		return;
	}

	FVector DesiredViewLocation = ViewTarget->GetActorLocation();
	FRotator DesiredViewRotation = ViewTarget->GetActorRotation();

	// now apply a tilt to simulate motion
	if (MeasuredVelocity.IsSet()) // && bIsTiltingEnabled)
	{
		FMatrix const OldCamToWorld = FRotationMatrix(DesiredViewRotation);
		FMatrix const UntiltedDroneToWorld = FRotationMatrix::MakeFromZX(FVector::UpVector, OldCamToWorld.GetUnitAxis(EAxis::X));

		FMatrix const OldCamToDrone = OldCamToWorld * UntiltedDroneToWorld.Inverse();

		// more velocity => more tilt
		// greater up vector magnitude => less tilt per unit velocity
		FVector const TiltedUpVector = (MeasuredVelocity.GetValue() + TiltUpVector);
		FMatrix TiltedDroneToWorld = FRotationMatrix::MakeFromZX(TiltedUpVector, UntiltedDroneToWorld.GetUnitAxis(EAxis::X));

		// interpolate drone tilt to smooth it out
		// only interpolating pitch and roll though!
		FRotator GoalTiltedDroneRot = TiltedDroneToWorld.Rotator();
		if (bEnableTiltLimits)
		{
			GoalTiltedDroneRot.Pitch = FMath::Clamp<>(GoalTiltedDroneRot.Pitch, -TiltLimits.Pitch, TiltLimits.Pitch);
			GoalTiltedDroneRot.Roll = FMath::Clamp<>(GoalTiltedDroneRot.Roll, -TiltLimits.Roll, TiltLimits.Roll);
		}

		GoalTiltedDroneRot.Yaw = 0.f;
		FRotator InterpedTiltedDroneRot = DroneTiltInterpolator.Eval(GoalTiltedDroneRot, DeltaTime);

//   		float const InterpSpeed = GoalTiltedDroneRot.IsZero() ? DroneTiltInterpSpeed_Decel : DroneTiltInterpSpeed_Accel;
//   		FRotator InterpedTiltedDroneRot = FMath::RInterpTo(LastTiltedDroneRot, GoalTiltedDroneRot, DeltaTime, InterpSpeed);
		LastTiltedDroneRot = InterpedTiltedDroneRot;
		InterpedTiltedDroneRot.Yaw = DesiredViewRotation.Yaw; // keep original Yaw
		TiltedDroneToWorld = FRotationMatrix(InterpedTiltedDroneRot);

		FMatrix const NewCamToWorld = OldCamToDrone * TiltedDroneToWorld;

		DesiredViewRotation = NewCamToWorld.Rotator();
	}

	//UpdateCameraVelocity(DeltaTime, DesiredViewLocation);
	//CineCamComp->SetWorldLocationAndRotation(DesiredViewLocation, DesiredViewRotation);


	OutVT.POV.Location = DesiredViewLocation;
	OutVT.POV.Rotation = DesiredViewRotation;
	OutVT.POV.FOV = FOV;
}

void UCitySampleCamera_Drone::OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode)
{
	Super::OnBecomeActive(ViewTarget, PreviouslyActiveMode);
}
