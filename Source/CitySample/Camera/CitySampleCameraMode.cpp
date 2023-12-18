// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCameraMode.h"

#include "Camera/CameraShakeBase.h"
#include "DrawDebugHelpers.h"

#include "Camera/CitySamplePlayerCameraManager.h"
#include "Character/CitySampleCharacter.h"
#include "Game/CitySamplePlayerController.h"

UCitySampleCameraMode::UCitySampleCameraMode()
	: TransitionInTime(0.5f)
{
	TransitionParams.BlendFunction = VTBlend_Cubic;
}

ACitySamplePlayerController* UCitySampleCameraMode::GetOwningCitySamplePC() const
{
	return PlayerCamera ? Cast<ACitySamplePlayerController>(PlayerCamera->PCOwner) : nullptr;
}

void UCitySampleCameraMode::SkipNextInterpolation()
{
	bSkipNextInterpolation = true;
	ShakeScaleInterpolator.Reset();
}

void UCitySampleCameraMode::OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode)
{
	if (CameraShakeClass != nullptr)
	{
		CameraShakeInstance = PlayerCamera->StartCameraShake(CameraShakeClass, 1.f);
	}

	if (PlayerCamera != nullptr)
	{
		if (bOverrideViewPitchMinAndMax)
		{
			PlayerCamera->SetViewPitchLimits(ViewPitchMinOverride, ViewPitchMaxOverride);
		}
		else
		{
			PlayerCamera->ResetViewPitchLimits();
		}
	}

	bIsActive = true;
}

void UCitySampleCameraMode::OnBecomeInactive(AActor* ViewTarget, UCitySampleCameraMode* NewActiveMode)
{
	if (CameraShakeInstance)
	{
		PlayerCamera->StopCameraShake(CameraShakeInstance, false);
		CameraShakeInstance = nullptr;
	}

	bIsActive = false;
}

float UCitySampleCameraMode::GetTransitionTime() const
{
	return TransitionInTime;
}

void UCitySampleCameraMode::ApplyCineCamSettings(FTViewTarget& OutVT, UCineCameraComponent* CineCamComp, float DeltaTime)
{
	if (CineCamComp)
	{
		// put cine cam component at final camera transform, then evaluate it
		CineCamComp->SetWorldTransform(LastCameraToWorld);
		if (bUseCineCamSettings)
		{
			CineCamComp->SetCurrentFocalLength(CineCam_CurrentFocalLength);
			const float FocusDistance = GetDesiredFocusDistance(OutVT.Target, LastCameraToWorld) + CineCam_FocusDistanceAdjustment;
			CineCamComp->FocusSettings.ManualFocusDistance = FocusDistance;
			CineCamComp->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
			CineCamComp->CurrentAperture = CineCam_CurrentAperture;
		}
		else
		{
			CineCamComp->SetFieldOfView(OutVT.POV.FOV);
			CineCamComp->FocusSettings.FocusMethod = ECameraFocusMethod::DoNotOverride;
			CineCamComp->CurrentAperture = 22.f;
		}

		CineCamComp->GetCameraView(DeltaTime, OutVT.POV);

		CineCam_DisplayOnly_FOV = OutVT.POV.FOV;
	}
}

float UCitySampleCameraMode::GetDesiredFocusDistance(AActor* ViewTarget, const FTransform& ViewToWorld) const
{
	if (bUseCustomFocusDistance)
	{
		const float Dist = GetCustomFocusDistance(ViewTarget, ViewToWorld);
		if (Dist > 0.f)
		{
			return Dist;
		}
	}

	FVector FocusPoint(0.f);
	if (ViewTarget)
	{
		FocusPoint = ViewTarget->GetActorLocation();
	}

	return (FocusPoint - ViewToWorld.GetLocation()).Size();
}


void UCitySampleCameraMode::UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, FTViewTarget& OutVT)
{
	if (CameraShakeInstance)
	{
		// cover all the else clauses below
		CameraShakeInstance->ShakeScale = 1.f;

		if (bScaleShakeWithViewTargetVelocity)
		{
			if (ViewTarget)
			{
				const float Speed = ViewTarget->GetVelocity().Size();
				const float GoalScale = FMath::GetMappedRangeValueClamped(ShakeScaling_SpeedRange, ShakeScaling_ScaleRange, Speed);
				CameraShakeInstance->ShakeScale = ShakeScaleInterpolator.Eval(GoalScale, DeltaTime);

				if (bDrawDebugShake)
				{
#if ENABLE_DRAW_DEBUG
					::FlushDebugStrings(ViewTarget->GetWorld());
					::DrawDebugString(ViewTarget->GetWorld(), ViewTarget->GetActorLocation() + FVector(0, 0, 60.f), FString::Printf(TEXT("%f"), CameraShakeInstance->ShakeScale), nullptr, FColor::Yellow);
#endif
				}
			}
		}
	}
}

bool UCitySampleCameraMode::ShouldLockOutgoingPOV() const
{
	return TransitionParams.bLockOutgoing;
}

void UCitySampleCameraMode::ResetToDefaultSettings_Implementation()
{
	UCitySampleCameraMode const * const ThisCDO = CastChecked<UCitySampleCameraMode>(GetClass()->GetDefaultObject());
	
	FOV = ThisCDO->FOV;
	bUseCineCamSettings = ThisCDO->bUseCineCamSettings;
	bUseCineCam = ThisCDO->bUseCineCam;
	CineCam_CurrentFocalLength = ThisCDO->CineCam_CurrentFocalLength;
	CineCam_CurrentAperture = ThisCDO->CineCam_CurrentAperture;
	CineCam_FocusDistanceAdjustment = ThisCDO->CineCam_FocusDistanceAdjustment;
	bUseCustomFocusDistance = ThisCDO->bUseCustomFocusDistance;
	TransitionInTime = ThisCDO->TransitionInTime;
	TransitionParams = ThisCDO->TransitionParams;
	ShakeScaling_SpeedRange  = ThisCDO->ShakeScaling_SpeedRange;
	ShakeScaling_ScaleRange = ThisCDO->ShakeScaling_ScaleRange;
	ShakeScaleInterpolator = ThisCDO->ShakeScaleInterpolator;
}