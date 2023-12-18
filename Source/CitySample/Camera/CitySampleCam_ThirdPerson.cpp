// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCam_ThirdPerson.h"

#include "CineCameraComponent.h"
#include "Curves/CurveVector.h"
#include "DrawDebugHelpers.h"
#include "Engine/Private/KismetTraceUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Character/CitySampleCharacter.h"
#include "CitySamplePlayerCameraManager.h"
#include "Game/CitySamplePlayerController.h"

static int DrawCameraDebugInfo = 0;
FAutoConsoleVariableRef CVar_DrawCameraDebugInfo(TEXT("CitySample.DrawCameraDebugInfo"), DrawCameraDebugInfo, TEXT("True to draw camera debugging info."), ECVF_Cheat);

UCitySampleCam_ThirdPerson::UCitySampleCam_ThirdPerson()
{
	CameraToPivot.SetTranslation(FVector(-300.f, 0.f, 0.f));
	FOV = 75.f;

	PivotPitchLimits = FVector2D(-80.f, 80.f);
	PivotYawLimits = FVector2D(-179.9f, 180.f);

	AutoFollowMode = ECameraAutoFollowMode::None;
	LazyAutoFollowPitchLimits = FVector2D(-50.f, -20.f);

	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 0.0f, 0.0f), 1.0f, 10.f, 0, true));

	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 3.0f, 0.0f),  1.f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -3.0f, 0.0f), 1.f, 8.f, 0, false));

	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 6.0f, 0.0f),  0.9f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -6.0f, 0.0f), 0.9f, 8.f, 0, false));

	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 9.0f, 0.0f),  0.8f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -9.0f, 0.0f), 0.8f, 8.f, 0, false));
	
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 12.0f, 0.0f),  0.7f, 8.f, 0, false));
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, -12.0f, 0.0f), 0.7f, 8.f, 0, false));
	
	CameraPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(8.0f, 0.0f, 0.0f),  1.0f, 8.f, 0, false));

	SafeLocPenetrationAvoidanceRays.Add(FPenetrationAvoidanceRay(FRotator(0.0f, 0.0f, 0.0f), 1.0f, 14.f, 0, true));

	SafeLocationOffset = FVector(0.f, 0.f, 95.f);
	bValidateSafeLoc = true;
	bPreventCameraPenetration = true;
	bDoPredictiveAvoidance = true;
}

FQuat UCitySampleCam_ThirdPerson::GetAutoFollowPivotToWorldRotation(const AActor* FollowActor) const
{
	FRotator const ActorRot = FollowActor ? FollowActor->GetActorRotation() : FRotator::ZeroRotator;
	return ActorRot.Quaternion();
}

float UCitySampleCam_ThirdPerson::GetViewTargetMeshHeightOffset(const AActor* ViewTarget) const
{
	const ACitySampleCharacter* const Char = Cast<ACitySampleCharacter>(ViewTarget);
	if (Char && PlayerCamera)
	{
		USkeletalMeshComponent const* const CharMesh = Char ? Char->GetMesh() : nullptr;
		if (CharMesh)
		{
			const float BaseRelZ = PlayerCamera->BasePelvisRelativeZ;
			const float CurRelZ = CharMesh->GetBoneLocation(PlayerCamera->PelvisBoneName, EBoneSpaces::ComponentSpace).Z;
			return CurRelZ - BaseRelZ;
		}
	}

	return 0.f;
}

FTransform UCitySampleCam_ThirdPerson::GetViewTargetToWorld(const AActor* ViewTarget) const
{
	return ViewTarget ? ViewTarget->GetActorTransform() : FTransform::Identity;
}

FTransform UCitySampleCam_ThirdPerson::ComputeCameraToWorld(const AActor* ViewTarget, FTransform const& PivotToWorld) const
{
	FTransform AdjustedCameraToPivot = GetCameraToPivot(ViewTarget);
	
	// maybe adjust based on pitch
	if (CameraToPivot_PitchAdjustmentCurve)
	{
		const float Pitch = PivotToWorld.Rotator().Pitch;
		const float PitchAlpha = FMath::GetRangePct(PivotPitchLimits, Pitch);
		const FVector PitchAdjustmentOffset = CameraToPivot_PitchAdjustmentCurve->GetVectorValue(PitchAlpha) * CameraToPivot_PitchAdjustmentCurveScale;
		AdjustedCameraToPivot.AddToTranslation(PitchAdjustmentOffset);
	}

	// maybe adjust based on speed
	if (CameraToPivot_SpeedAdjustmentCurve)
	{
		const float CurSpeed = ViewTarget ? ViewTarget->GetVelocity().Size() : 0.f;
		const float SpeedAlpha = FMath::Clamp(FMath::GetRangePct(CameraToPivot_SpeedAdjustment_SpeedRange, CurSpeed), 0.f, 1.f);

		const FVector SpeedAdjustmentOffset = CameraToPivot_SpeedAdjustmentCurve->GetVectorValue(SpeedAlpha) * CameraToPivot_SpeedAdjustmentCurveScale;
		AdjustedCameraToPivot.AddToTranslation(SpeedAdjustmentOffset);
	}
	
	return AdjustedCameraToPivot * PivotToWorld;
}

void UCitySampleCam_ThirdPerson::ComputePredictiveLookAtPoint(FVector& LookAtPointOutput, const AActor* ViewTarget, float DeltaTime)
{
	LookAtPointOutput = LookAtPointOutput + ViewTarget->GetVelocity() * PredictiveLookatTime;
}

FVector UCitySampleCam_ThirdPerson::ComputeWorldLookAtPosition(const FVector IdealWorldLookAt, float DeltaTime)
{
	return LookatWorldSpaceInterpolator.Eval(IdealWorldLookAt, DeltaTime);
}

FRotator UCitySampleCam_ThirdPerson::ComputeSmoothPivotRotation(const FRotator IdealPivotToWorldRot, float DeltaTime)
{
	if (bSkipNextInterpolation)
	{
		PivotRotInterpolator.Reset();
	}

	return PivotRotInterpolator.Eval(IdealPivotToWorldRot, DeltaTime);
}

FTransform UCitySampleCam_ThirdPerson::ComputePivotToWorld(const AActor* ViewTarget) const
{
	const ACitySamplePlayerController* const PC = PlayerCamera ? Cast<ACitySamplePlayerController>(PlayerCamera->PCOwner) : nullptr;
	if (PC == nullptr)
	{
		return LastPivotToWorld;
	}

	FTransform ViewTargetToWorld = GetViewTargetToWorld(ViewTarget);
	const float MeshZOffset = GetViewTargetMeshHeightOffset(ViewTarget);
	ViewTargetToWorld.AddToTranslation(FVector(0.f, 0.f, MeshZOffset));

	FTransform PivotToWorld = GetPivotToViewTarget(ViewTarget) * ViewTargetToWorld;

	// use control rotation by default, this may get overridden below
	PivotToWorld.SetRotation(PlayerCamera->PCOwner->GetControlRotation().Quaternion());

	if (AutoFollowMode == ECameraAutoFollowMode::FullFollow)
	{
		FQuat AutoFollowPivotToWorldRot = GetAutoFollowPivotToWorldRotation(ViewTarget);
		PivotToWorld.SetRotation(AutoFollowPivotToWorldRot);
	}
	else if (AutoFollowMode == ECameraAutoFollowMode::LazyFollow)
	{
		if (!bSkipNextInterpolation)
		{
			if (LazyFollowDelay_TimeRemaining > 0.f)
			{
				// user is rotating the camera or we are in the delay period, let that happen
				PivotToWorld.SetRotation(PC->GetControlRotation().Quaternion());
			}
			else
			{
				FRotator NewPivotRot = FRotator::ZeroRotator;
				if (LazyFollowLaziness <= 0.f)
				{
					NewPivotRot = ViewTarget->GetActorRotation();
				}
				else
				{
					const FVector LastPivotDir = -LastUnsmoothedPivotToWorld.GetRotation().GetForwardVector();
					const FVector LastPivotAimPoint = LastUnsmoothedPivotToWorld.GetLocation() + LastPivotDir * LazyFollowLaziness;
					const FVector NewPivotDir = PivotToWorld.GetLocation() - LastPivotAimPoint;
					NewPivotRot = FRotationMatrix::MakeFromXZ(NewPivotDir, FVector::UpVector).Rotator();
				}

				if (bAllowLazyAutoFollowPitchControl)
				{
					// only take the yaw from the auto follow rotation
					float const LazyAutoFollowYaw = NewPivotRot.Yaw;
					float const ControlPitch = PlayerCamera->PCOwner->GetControlRotation().Pitch;
					FRotator const NewP2W(ControlPitch, LazyAutoFollowYaw, 0.f);
					PivotToWorld.SetRotation(NewP2W.Quaternion());
				}
				else
				{
					PlayerCamera->LimitViewPitch(NewPivotRot, LazyAutoFollowPitchLimits.X, LazyAutoFollowPitchLimits.Y);
					PivotToWorld.SetRotation(NewPivotRot.Quaternion());
				}
			}
		}
	}
	
	// Clamp final rotation values
	FRotator P2WRot = PivotToWorld.Rotator();
	const FRotator ViewTargetToWorldRot = ViewTargetToWorld.Rotator();
	PlayerCamera->LimitViewPitch(P2WRot, PivotPitchLimits.X, PivotPitchLimits.Y);
	PlayerCamera->LimitViewYaw(P2WRot, ViewTargetToWorldRot.Yaw + PivotYawLimits.X, ViewTargetToWorldRot.Yaw + PivotYawLimits.Y);
	P2WRot.Roll = 0.f;
	PivotToWorld.SetRotation(P2WRot.Quaternion());

	return PivotToWorld;
}

void UCitySampleCam_ThirdPerson::UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, FTViewTarget& OutVT)
{
	// let super do any updates it wants (e.g. camera shakes), but we will fully determine the POV below
	Super::UpdateCamera(ViewTarget, CineCamComp, DeltaTime, OutVT);

	UWorld const* const World = ViewTarget ? ViewTarget->GetWorld() : nullptr;
	if (World)
	{
		// if the pawn is pending destroy, the position of the pawn gets reset and camera will be teleported to weird positions.
		// so just use the old FOV without any update
		if (!IsValid(ViewTarget) || !PlayerCamera || !PlayerCamera->PCOwner)
		{
			return;
		}

		AdjustAutoFollowMode(ViewTarget);

		if ((AutoFollowMode == ECameraAutoFollowMode::LazyFollow) && !LastControlRotation.Equals(PlayerCamera->PCOwner->GetControlRotation(), 0.1f))
		{
			LazyFollowDelay_TimeRemaining = LazyFollowDelayAfterUserControl;
		}
		else if (LazyFollowDelay_TimeRemaining > 0.f)
		{
			LazyFollowDelay_TimeRemaining -= DeltaTime;
		}
		
		// Compute pivot goal transform
		FTransform const PivotToWorld = ComputePivotToWorld(ViewTarget);

		LastUnsmoothedPivotToWorld = PivotToWorld;

		// set control rotation back to pivot rotation, so controls keep making sense
		PlayerCamera->PCOwner->SetControlRotation(PivotToWorld.GetRotation().Rotator());

		// Smooth the pivot transform to feel good
		FTransform SmoothedPivotToWorld = PivotToWorld;
		{
			// smooth rotation	
			FRotator const SmoothedPivotRot = ComputeSmoothPivotRotation(PivotToWorld.Rotator(), DeltaTime);
			SmoothedPivotToWorld.SetRotation(SmoothedPivotRot.Quaternion());

			// smooth translation
			if (bSkipNextInterpolation)
			{
				PivotLocInterpolator.Reset();
			}

			FVector const SmoothedPivotLoc = PivotLocInterpolator.Eval(PivotToWorld.GetLocation(), DeltaTime);
			SmoothedPivotToWorld.SetLocation(SmoothedPivotLoc);
		}
		LastPivotToWorld = SmoothedPivotToWorld;

		// update camera to pivot interpolation, which will be used after this
		CameraToPivotTranslationInterpolator.Eval(GetBaseCameraToPivot(ViewTarget).GetTranslation(), DeltaTime);

		// Get final ideal camera location
		FTransform const CameraToWorld = ComputeCameraToWorld(ViewTarget, SmoothedPivotToWorld);
		FVector const CameraGoalLoc = CameraToWorld.GetLocation();

		{
			// figure out look at position
			FRotator FinalCameraRot = CameraToWorld.Rotator();
			if (bUseLookatPoint)
			{
				FVector IdealWorldLookat = ViewTarget->GetTransform().TransformPosition(LookatOffsetLocal);
				if (bDoPredictiveLookat)
				{
					if (ACharacter* VTChar = Cast<ACharacter>(ViewTarget))
					{
						if (UCharacterMovementComponent* CharMoveComp = VTChar->GetCharacterMovement())
						{
							IdealWorldLookat = IdealWorldLookat + CharMoveComp->Velocity * PredictiveLookatTime;
						}
					}
					else
					{
						ComputePredictiveLookAtPoint(IdealWorldLookat, ViewTarget, DeltaTime);
					}
				}

				FVector const WorldLookat = ComputeWorldLookAtPosition(IdealWorldLookat, DeltaTime);
				LastLookatWorldSpace = WorldLookat;

#if ENABLE_DRAW_DEBUG
				if (bDrawDebugLookat || DrawCameraDebugInfo)
				{
					::DrawDebugBox(World, IdealWorldLookat, FVector(12.f), FQuat::Identity, FColor::Yellow, false);
					::DrawDebugBox(World, LastLookatWorldSpace, FVector(8.f), FQuat::Identity, FColor::White, false);
				}
#endif

				FinalCameraRot = (LastLookatWorldSpace - CameraToWorld.GetLocation()).Rotation();
			}

#if ENABLE_DRAW_DEBUG
			if (bDrawDebugPivot || DrawCameraDebugInfo)
			{
				::DrawDebugCoordinateSystem(World, PivotToWorld.GetLocation(), PivotToWorld.Rotator(), 24.f, false, -1.f, 0, 2.f);
				::DrawDebugCoordinateSystem(World, SmoothedPivotToWorld.GetLocation(), SmoothedPivotToWorld.Rotator(), 24.f, false, -1.f, 0, 2.f);
			}
#endif
			FinalCameraRot.Yaw += ComputeYawModifier(ViewTarget, DeltaTime);
			FinalCameraRot.Roll += ComputeRollModifier(ViewTarget, DeltaTime);
			
			// Set desired pov members with results
			OutVT.POV.Location = CameraGoalLoc;
			OutVT.POV.Rotation = FinalCameraRot;
			OutVT.POV.FOV = ComputeFinalFOV(ViewTarget);
		}

		PlayerCamera->ApplyCameraModifiers(DeltaTime, OutVT.POV);

		FVector DesiredCamLoc = OutVT.POV.Location;

		// now that we have the IDEAL position the camera wants to be in, 
		// we can do tests against the world to get a final position

		// adjust desired camera location, to again, prevent any penetration
		FVector ValidatedCameraLocation = DesiredCamLoc;
		if (bPreventCameraPenetration)
		{
			// find "worst" location, or location we will shoot the penetration tests from

			FTransform ViewTargetTransform = GetViewTargetToWorld(ViewTarget);

			const float MeshZOffset = GetViewTargetMeshHeightOffset(ViewTarget);
			FVector IdealSafeLocationLocal = ViewTarget->GetActorLocation() + ViewTarget->GetActorQuat().RotateVector(SafeLocationOffset) + FVector(0.f, 0.f, MeshZOffset);
		
			const FMatrix CamSpaceToWorld = FQuatRotationTranslationMatrix(ViewTarget->GetActorQuat(), ViewTarget->GetActorLocation());
			IdealSafeLocationLocal = CamSpaceToWorld.InverseTransformPosition(IdealSafeLocationLocal);

			const FVector SafeLocationLocal = SafeLocationInterpolator.Eval(IdealSafeLocationLocal, DeltaTime);
			LastSafeLocationLocal = SafeLocationLocal;

			// rotate back to world space
			const FVector IdealSafeLocation = CamSpaceToWorld.TransformPosition(LastSafeLocationLocal);

			FVector ValidatedSafeLocation = IdealSafeLocation;
			float LastSafeLocBlockedPct = 1.f;

			// adjust worst location origin to prevent any penetration
			if (bValidateSafeLoc)
			{
				PreventCameraPenetration(ViewTarget, SafeLocPenetrationAvoidanceRays, ViewTarget->GetActorLocation(), IdealSafeLocation, DeltaTime, ValidatedSafeLocation, LastSafeLocBlockedPct, true);
			}

#if ENABLE_DRAW_DEBUG
			if (bDrawDebugSafeLoc || DrawCameraDebugInfo)
			{
				::DrawDebugSphere(World, IdealSafeLocation, 12.f, 12.f, FColor::Yellow);
				::DrawDebugSphere(World, ValidatedSafeLocation, 10.f, 10.f, FColor::White);
			}			
#endif

			// note: we skip predictive avoidance while this mode is blending out
			bool const bSingleRayPenetrationCheck = !bDoPredictiveAvoidance || !bIsActive;
			PreventCameraPenetration(ViewTarget, CameraPenetrationAvoidanceRays, ValidatedSafeLocation, DesiredCamLoc, DeltaTime, ValidatedCameraLocation, LastPenetrationBlockedPct, bSingleRayPenetrationCheck);
		}

		OutVT.POV.Location = ValidatedCameraLocation;
			   		 
		LastCameraToWorld = FTransform(OutVT.POV.Rotation, OutVT.POV.Location);
		LastControlRotation = PlayerCamera->PCOwner->GetControlRotation();
	}

	ApplyCineCamSettings(OutVT, CineCamComp, DeltaTime);

	bSkipNextInterpolation = false;
}

void UCitySampleCam_ThirdPerson::OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode)
{
	Super::OnBecomeActive(ViewTarget, PreviouslyActiveMode);

	SkipNextInterpolation();
	LastCameraToWorld = PlayerCamera ? FTransform(PlayerCamera->GetCameraRotation(), PlayerCamera->GetCameraLocation()) : FTransform::Identity;

	LazyFollowDelay_TimeRemaining = 0.f;
}

void UCitySampleCam_ThirdPerson::SkipNextInterpolation()
{
	Super::SkipNextInterpolation();

	PivotLocInterpolator.Reset();
	PivotRotInterpolator.Reset();
	LookatWorldSpaceInterpolator.Reset();
	SafeLocationInterpolator.Reset();
	CameraToPivotTranslationInterpolator.Reset();

	bSkipNextPredictivePenetrationAvoidanceBlend = true;
}

FTransform UCitySampleCam_ThirdPerson::GetCameraToPivot(const AActor* ViewTarget) const
{
	FTransform CamToPivot = GetBaseCameraToPivot(ViewTarget);
	CamToPivot.SetTranslation(CameraToPivotTranslationInterpolator.GetCurrentValue());
	return CamToPivot;
}

void UCitySampleCam_ThirdPerson::PreventCameraPenetration(AActor* Target, TArray<FPenetrationAvoidanceRay>& Rays, const FVector& SafeLoc, const FVector& IdealCameraLoc, float DeltaTime, FVector& OutCameraLoc, float& DistBlockedPct, bool bSingleRayOnly)
{
	UWorld* const World = PlayerCamera->GetWorld();

	float HardBlockedPct = DistBlockedPct;
	float SoftBlockedPct = DistBlockedPct;

	FVector BaseRay = IdealCameraLoc - SafeLoc;
	FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
	FVector BaseRayLocalUp, BaseRayLocalFwd, BaseRayLocalRight;
	BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);
	float DistBlockedPctThisFrame = 1.f;

	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(CameraPenetration), false, Target);
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(0.f);
	
	BlockingActors.Empty();

	for (auto& Ray : Rays)
	{
		if (bSingleRayOnly && !Ray.bPrimaryRay)
		{
			continue;
		}

		if (Ray.bEnabled)
		{
			if (Ray.FramesUntilNextTrace <= 0)
			{
				// calculate ray target
				FVector RayTarget;
				{
					FVector RotatedRay = BaseRay.RotateAngleAxis(Ray.AdjustmentRot.Yaw, BaseRayLocalUp);
					RotatedRay = RotatedRay.RotateAngleAxis(Ray.AdjustmentRot.Pitch, BaseRayLocalRight);
					RayTarget = SafeLoc + RotatedRay;
				}

				SphereShape.Sphere.Radius = Ray.Radius;
				ECollisionChannel TraceChannel = ECC_Camera;

				FHitResult Hit;
				const FVector TraceStart = SafeLoc;
				const FVector TraceEnd = RayTarget;
				bool bHit = World->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, TraceChannel, SphereShape, SphereParams);
				Ray.FramesUntilNextTrace = Ray.TraceInterval;

#if ENABLE_DRAW_DEBUG
				if (bDrawDebugPenetrationAvoidance)
				{
					::DrawDebugLineTraceSingle(World, TraceStart, TraceEnd, EDrawDebugTrace::ForDuration, bHit, Hit, FColor::White, FColor::Red, 0.1f);
				}
#endif

				if (bHit)
				{
					const float NewBlockPct = FMath::GetMappedRangeValueClamped(FVector2D(1.f, 0.f), FVector2D(Hit.Time, 1.f), Ray.WorldWeight);
					DistBlockedPctThisFrame = FMath::Min(NewBlockPct, DistBlockedPctThisFrame);

					// This feeler got a hit, so do another trace next frame
					Ray.FramesUntilNextTrace = 0;

					BlockingActors.AddUnique(Hit.GetActor());
				}

				if (Ray.bPrimaryRay)
				{
					// don't interpolate toward this one, snap to it, assumes ray 0 is the center/main ray 
					HardBlockedPct = DistBlockedPctThisFrame;
				}
				else
				{
					SoftBlockedPct = DistBlockedPctThisFrame;
				}
			}
			else
			{
				--Ray.FramesUntilNextTrace;
			}
		}
	}

	if (DistBlockedPct < DistBlockedPctThisFrame)
	{
		// interpolate smoothly out
		if ((PenetrationBlendOutTime > DeltaTime) && (bSkipNextPredictivePenetrationAvoidanceBlend == false))
		{
			DistBlockedPct = DistBlockedPct + DeltaTime / PenetrationBlendOutTime * (DistBlockedPctThisFrame - DistBlockedPct);
		}
		else
		{
			DistBlockedPct = DistBlockedPctThisFrame;
		}
	}
	else
	{
		if (DistBlockedPct > HardBlockedPct)
		{
			DistBlockedPct = HardBlockedPct;
		}
		else if (DistBlockedPct > SoftBlockedPct)
		{
			// interpolate smoothly in
			if ((PenetrationBlendInTime > DeltaTime) && (bSkipNextPredictivePenetrationAvoidanceBlend == false))
			{
				DistBlockedPct = DistBlockedPct - DeltaTime / PenetrationBlendInTime * (DistBlockedPct - SoftBlockedPct);
			}
			else
			{
				DistBlockedPct = SoftBlockedPct;
			}
		}
	}

	DistBlockedPct = FMath::Clamp<float>(DistBlockedPct, 0.f, 1.f);
	if (DistBlockedPct < KINDA_SMALL_NUMBER)
	{
		DistBlockedPct = 0.f;
	}

	if (DistBlockedPct < 1.f)
	{
		OutCameraLoc = SafeLoc + (IdealCameraLoc - SafeLoc) * DistBlockedPct;
	}
	else
	{
		OutCameraLoc = IdealCameraLoc;
	}

	bSkipNextPredictivePenetrationAvoidanceBlend = false;
}

