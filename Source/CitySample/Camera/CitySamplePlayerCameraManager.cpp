// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySamplePlayerCameraManager.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CineCameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Game/CitySamplePlayerController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsSettings.h"

ACitySamplePlayerCameraManager::ACitySamplePlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultMinPitchLimit = ViewPitchMin;
	DefaultMaxPitchLimit = ViewPitchMax;
}

/** for displayDebug only */
static FString BlendVolumeDebugModeName;
static float BlendVolumeDebugBlendWeight;

void ACitySamplePlayerCameraManager::DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.DrawString(TEXT("Camera Stack"));

	ACameraActor* const CamActor = Cast<ACameraActor>(GetViewTarget());

	if (CamActor)
	{
		const FString DisplayStr = FString::Printf(TEXT("  Viewing through CameraActor %s"), *GetNameSafe(CamActor));
		DisplayDebugManager.DrawString(DisplayStr);
	}
	else
	{
		for (int32 CamModeIndex = 0; CamModeIndex < CameraBlendStack.Num(); CamModeIndex++)
		{
			FActiveCitySampleCamera& StackEntry = CameraBlendStack[CamModeIndex];

			if ((CamModeIndex == 0) && (BlendVolumeDebugBlendWeight > 0.f))
			{
				const FString DisplayStr1 = FString::Printf(TEXT("  [%d] BLEND VOLUME : %f"), CamModeIndex, StackEntry.TransitionAlpha);
				const FString DisplayStr2 = FString::Printf(TEXT("      %s (base) : %f"), *GetNameSafe(StackEntry.Camera), 1.f - BlendVolumeDebugBlendWeight);
				const FString DisplayStr3 = FString::Printf(TEXT("      %s (override) : %f"), *BlendVolumeDebugModeName, BlendVolumeDebugBlendWeight);
				DisplayDebugManager.DrawString(DisplayStr1);
				DisplayDebugManager.DrawString(DisplayStr2);
				DisplayDebugManager.DrawString(DisplayStr3);
			}
			else
			{
				const FString DisplayStr1 = FString::Printf(TEXT("  [%d] %s : %f"), CamModeIndex, *GetNameSafe(StackEntry.Camera), StackEntry.TransitionAlpha);
				DisplayDebugManager.DrawString(DisplayStr1);

				if (!StackEntry.Camera->BlockingActors.IsEmpty())
				{
					FString DisplayStr2 = FString::Printf((TEXT("      Blocked by:")));

					AActor* Actor = StackEntry.Camera->BlockingActors[0];
					DisplayStr2.Append(FString::Printf(TEXT(" %s"), *GetNameSafe(Actor)));

					for (int32 ActorIndex = 1; ActorIndex < StackEntry.Camera->BlockingActors.Num(); ++ActorIndex)
					{
						Actor = StackEntry.Camera->BlockingActors[ActorIndex];
						DisplayStr2.Append(FString::Printf(TEXT(", %s"), *GetNameSafe(Actor)));
					}

					DisplayDebugManager.DrawString(DisplayStr2);
				}
			}
		}
	}

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
}

UCitySampleCameraMode* ACitySamplePlayerCameraManager::GetCurrentCameraMode()
{
	if (ViewTarget.Target)
	{
		const int32 InstanceIdx = GetBestCameraMode(ViewTarget.Target);
		UCitySampleCameraMode* const BestCamera = CameraModeInstances[InstanceIdx].CameraMode;
		return BestCamera;
	}

	return nullptr;
}

void ACitySamplePlayerCameraManager::UpdateCameraInStack(int32 StackIdx, float DeltaTime, FTViewTarget& OutVT)
{
	if (CameraBlendStack.IsValidIndex(StackIdx))
	{
		FActiveCitySampleCamera& CamEntry = CameraBlendStack[StackIdx];

		// if this is an outgoing camera who wants to lock outgoing, just
		// return the last POV and be done
		if ((StackIdx >= 1) && CamEntry.bLockOutgoingPOV)
		{
			OutVT.POV = CamEntry.LastPOV;
			return;
		}
		else
		{
			const int32 InstanceIdx = CamEntry.InstanceIndex;
			if (CameraModeInstances.IsValidIndex(InstanceIdx))
			{
				FCitySampleCameraModeInstance& ModeInstance = CameraModeInstances[InstanceIdx];
				ModeInstance.UpdateCamera(DeltaTime, OutVT);
			}

			CamEntry.LastPOV = OutVT.POV;
		}
	}
}

ACitySamplePlayerController* ACitySamplePlayerCameraManager::GetOwningCitySamplePC() const
{
	return Cast<ACitySamplePlayerController>(PCOwner);
}

void ACitySamplePlayerCameraManager::TransitionToAltCamera()
{
	if (ACitySamplePlayerController* const PC = GetOwningCitySamplePC())
	{
		SavedMainViewTarget = PC->GetViewTarget();

		bUsingAltCameraMode = true;
		FViewTargetTransitionParams TransitionParams;
		TransitionParams.BlendTime = AltCameraTransitionTime;
		PC->SetViewTarget(AltViewTarget, TransitionParams);
	}
}

void ACitySamplePlayerCameraManager::TransitionFromAltCamera()
{
	if (ACitySamplePlayerController* const PC = GetOwningCitySamplePC())
	{
		bUsingAltCameraMode = false;
		FViewTargetTransitionParams TransitionParams;
		TransitionParams.BlendTime = AltCameraTransitionTime;
		PC->SetViewTarget(SavedMainViewTarget, TransitionParams);

		SavedMainViewTarget = nullptr;
	}
}

void ACitySamplePlayerCameraManager::SetUsingAlternateCamera(bool bNewUsingAltCamera)
{
	if (bUsingAltCameraMode != bNewUsingAltCamera)
	{
		if (bNewUsingAltCamera && HasAlternateCameraAvailable())
		{
			TransitionToAltCamera();
			bUsingAltCameraMode = true;
		}
		else if (!bNewUsingAltCamera)
		{
			TransitionFromAltCamera();
			bUsingAltCameraMode = false;
		}
	}
}

void ACitySamplePlayerCameraManager::ConfigureAlternateCamera(TSubclassOf<UCitySampleCameraMode> NewAltCameraMode, AActor* NewAltViewTarget, float NewAltCameraTransitionTime)
{
	AltCameraMode = NewAltCameraMode;
	AltViewTarget = NewAltViewTarget;
	AltCameraTransitionTime = NewAltCameraTransitionTime;
}

void ACitySamplePlayerCameraManager::ClearAlternateCamera()
{
	if (bUsingAltCameraMode)
	{
		SetUsingAlternateCamera(false);
	}

	AltCameraMode = nullptr;
}

static FRotator BlendRots(FRotator A, float AWeight, FRotator B, float BWeight)
{
	// normalize weights
	float NormalizedBWeight = (1.f / (AWeight + BWeight)) * BWeight;
	const FRotator Delta = (B - A).GetNormalized();
	return (A + (Delta * NormalizedBWeight)).GetNormalized();
};

void ACitySamplePlayerCameraManager::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
	// Make sure we have a valid target
	if (OutVT.Target == nullptr)
	{
		return;
	}

	// If our viewtarget is simulating using physics, we may need to clamp deltatime
	{
		const USceneComponent* const TargetRoot = OutVT.Target->GetRootComponent();
		if (TargetRoot && TargetRoot->IsSimulatingPhysics())
		{
			// Use the same max timestep cap as the physics system to avoid camera jitter when the viewtarget simulates less time than the camera
			DeltaTime = FMath::Min(DeltaTime, UPhysicsSettings::Get()->MaxPhysicsDeltaTime);
		}
	}

	const FMinimalViewInfo OriginalPOV = OutVT.POV;

	ACameraActor* const CamActor = Cast<ACameraActor>(OutVT.Target);
	if (CamActor && (IsUsingAlternateCamera() == false))
	{
		// Viewing through a camera actor.
		CamActor->GetCameraComponent()->GetCameraView(DeltaTime, OutVT.POV);
	}
	else
	{
		// Only keep Location, Rotation, and aspect ratio between frames. All other POV settings should be assigned by the camera below.
		OutVT.POV = FMinimalViewInfo();
		OutVT.POV.Location = OriginalPOV.Location;
		OutVT.POV.Rotation = OriginalPOV.Rotation;
		OutVT.POV.AspectRatio = OriginalPOV.AspectRatio;

		// find the camera instance for this class and viewtarget, creating one if necessary
		const int32 InstanceIdx = GetBestCameraMode(OutVT.Target);
		UCitySampleCameraMode* const BestCamera = CameraModeInstances[InstanceIdx].CameraMode;
		check(BestCamera);

		UCitySampleCameraMode* DeactivatedCamera = nullptr;
		AActor* DeactivatedCameraViewTarget = nullptr;

		// Update camera we're blending to, if it's different.
		if ((CameraBlendStack.Num() == 0) || (CameraBlendStack[0].Camera != BestCamera))
		{
			// Notify current camera that it's becoming inactive
			if ((CameraBlendStack.Num() > 0) && CameraBlendStack[0].Camera)
			{
				// cache this, so we can call OnBecomeInactive later
				DeactivatedCamera = CameraBlendStack[0].Camera;
				DeactivatedCameraViewTarget = CameraBlendStack[0].ViewTarget;

				CameraBlendStack[0].bLockOutgoingPOV = DeactivatedCamera->ShouldLockOutgoingPOV();
			}

			// See if we already have that camera in our stack.
			bool bAlreadyInStack = false;
			for (int32 CamModeIndex = 0; CamModeIndex < CameraBlendStack.Num(); CamModeIndex++)
			{
				FActiveCitySampleCamera& CamEntry = CameraBlendStack[CamModeIndex];
				if (CamEntry.Camera == BestCamera)
				{
					bAlreadyInStack = true;

					// Put it back on top, as our active camera.
					CameraBlendStack.Swap(0, CamModeIndex);

					// If this camera has no transition, set it full weight and kill remaining modes.
					if (FMath::IsNearlyZero(CamEntry.TransitionUpdateRate))
					{
						CameraBlendStack[0].TransitionAlpha = 1.f;
						// we already checked that first camera is not the BestCamera, so if we're here, we have at least 2.
						check(CameraBlendStack.Num() > 0);

						for (int32 Idx = 1; Idx < CameraBlendStack.Num(); Idx++)
						{
							CameraBlendStack[Idx].Camera->OnRemovedFromStack();
						}
						CameraBlendStack.RemoveAt(1, CameraBlendStack.Num() - 1);
					}
					break;
				}
			}

			if (!bAlreadyInStack)
			{
				// get the transition time
				float TransitionTime = (CameraBlendStack.Num() > 0) ? GetModeTransitionTime(BestCamera) : 0.f;

				// If we're looking at a new object and we're over 100m away, just warp there
				if (CameraBlendStack.Num() > 0)
				{
					if (OutVT.Target && CameraBlendStack[0].ViewTarget != OutVT.Target && CameraBlendStack[0].Camera && CameraBlendStack[0].Camera->PlayerCamera)
					{
						if ((OutVT.Target->GetActorLocation() - CameraBlendStack[0].Camera->PlayerCamera->GetTransitionGoalPOV().Location).Size2D() > 10000.f)
						{
							TransitionTime = 0.0f;
						}
					}
				}

				// now place new entry for this camera at top of stack at (with current effective contribution if existing)
				FActiveCitySampleCamera NewCamEntry;
				if (TransitionTime > 0.f)
				{
					NewCamEntry.TransitionAlpha = 0.f;
					NewCamEntry.TransitionUpdateRate = (1.f / TransitionTime);
				}
				else
				{
					NewCamEntry.TransitionAlpha = 1.f;
					NewCamEntry.TransitionUpdateRate = 0.f;
					CameraBlendStack.Empty();
				}
				NewCamEntry.Camera = BestCamera;
				NewCamEntry.ViewTarget = OutVT.Target;
				NewCamEntry.InstanceIndex = InstanceIdx;
				CameraBlendStack.Insert(NewCamEntry, 0);
			}

			if (DeactivatedCamera)
			{
				// notify deactivated camera
				// we do this here so the resultant camera stack is in a solid state in case this function wants to 
				// influence other camera modes (e.g. start a turn)
				DeactivatedCamera->OnBecomeInactive(DeactivatedCameraViewTarget, BestCamera);
			}

			// notify new camera it is "active" now
			BestCamera->OnBecomeActive(OutVT.Target, DeactivatedCamera);
		}

		// Update transition for incoming camera.
		// Incoming camera drives transition blend rate
		const float DeltaTransitionPosition = CameraBlendStack[0].TransitionUpdateRate * DeltaTime;
		CameraBlendStack[0].TransitionAlpha = FMath::Clamp<float>(CameraBlendStack[0].TransitionAlpha + DeltaTransitionPosition, 0.f, 1.f);
		CameraBlendStack[0].BlendWeight = CameraBlendStack[0].Camera->TransitionParams.GetBlendAlpha(CameraBlendStack[0].TransitionAlpha);
		float TotalWeight = CameraBlendStack[0].BlendWeight;

		// Update transition for outgoing cameras
		for (int32 StackIdx = 1; StackIdx < CameraBlendStack.Num(); StackIdx++)
		{
			FActiveCitySampleCamera& CamEntry = CameraBlendStack[StackIdx];

			if (CamEntry.Camera)
			{
				CamEntry.TransitionAlpha = FMath::Clamp<float>(CamEntry.TransitionAlpha - DeltaTransitionPosition, 0.f, 1.f);
				CamEntry.BlendWeight = CamEntry.Camera->TransitionParams.GetBlendAlpha(CamEntry.TransitionAlpha);
			}

			// Remove non relevant or invalid cameras
			if (!CamEntry.Camera || !CamEntry.ViewTarget || FMath::IsNearlyZero(CamEntry.TransitionAlpha) || FMath::IsNearlyZero(CamEntry.BlendWeight))
			{
				CameraBlendStack[StackIdx].Camera->OnRemovedFromStack();
				CameraBlendStack.RemoveAt(StackIdx);
				StackIdx--;
				continue;
			}

			TotalWeight += CamEntry.BlendWeight;
		}

		// Normalize weights, evaluate and blend!
		UpdateCameraInStack(0, DeltaTime, OutVT);

		if (TotalWeight == 0.0f)
		{
			CameraBlendStack[0].BlendWeight = 1.0f;
		}
		else
		{
			CameraBlendStack[0].BlendWeight /= TotalWeight;
		}

		FMinimalViewInfo BlendedPOV = OutVT.POV;
		BlendedPOV.ApplyBlendWeight(CameraBlendStack[0].BlendWeight);


		float SkippedFStopWeight = 0.f;
		float SkippedFocalDistWeight = 0.f;

		// handle depth of field post processing settings
		{
			BlendedPOV.PostProcessSettings.bOverride_DepthOfFieldFstop |= OutVT.POV.PostProcessSettings.bOverride_DepthOfFieldFstop;
			const float FStop = OutVT.POV.PostProcessSettings.bOverride_DepthOfFieldFstop ? OutVT.POV.PostProcessSettings.DepthOfFieldFstop : 22.f;
			BlendedPOV.PostProcessSettings.DepthOfFieldFstop = FStop * CameraBlendStack[0].BlendWeight;

			if (OutVT.POV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance)
			{
				BlendedPOV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
				BlendedPOV.PostProcessSettings.DepthOfFieldFocalDistance = OutVT.POV.PostProcessSettings.DepthOfFieldFocalDistance * CameraBlendStack[0].BlendWeight;
			}
			else
			{
				SkippedFocalDistWeight += CameraBlendStack[0].BlendWeight;
				BlendedPOV.PostProcessSettings.DepthOfFieldFocalDistance = 0.f;
			}
		}


		for (int32 StackIdx = 1; StackIdx < CameraBlendStack.Num(); ++StackIdx)
		{
			FActiveCitySampleCamera& CamEntry = CameraBlendStack[StackIdx];

			UpdateCameraInStack(StackIdx, DeltaTime, OutVT);

			CamEntry.BlendWeight /= TotalWeight;

			BlendedPOV.AddWeightedViewInfo(OutVT.POV, CamEntry.BlendWeight);

			BlendedPOV.PostProcessSettings.bOverride_DepthOfFieldFstop |= OutVT.POV.PostProcessSettings.bOverride_DepthOfFieldFstop;
			const float FStop = OutVT.POV.PostProcessSettings.bOverride_DepthOfFieldFstop ? OutVT.POV.PostProcessSettings.DepthOfFieldFstop : 8.f;
			BlendedPOV.PostProcessSettings.DepthOfFieldFstop += FStop * CamEntry.BlendWeight;

			if (OutVT.POV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance)
			{
				BlendedPOV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
				BlendedPOV.PostProcessSettings.DepthOfFieldFocalDistance += OutVT.POV.PostProcessSettings.DepthOfFieldFocalDistance * CamEntry.BlendWeight;
			}
			else
			{
				SkippedFocalDistWeight += CamEntry.BlendWeight;
			}
		}

		if (BlendedPOV.PostProcessSettings.bOverride_DepthOfFieldFocalDistance)
		{
			BlendedPOV.PostProcessSettings.DepthOfFieldFocalDistance /= (1.f - SkippedFocalDistWeight);
		}

		OutVT.POV = BlendedPOV;

		// do custom blending of the rotators, because the method above always blends through 0 
		// here we roll up the camera stack bottom up -- last 2 together, then that result into the one above that, and so on
		// the key is that BlendRots will go the shortest way, instead of always through 0
		{
			FActiveCitySampleCamera& LastCamEntry = CameraBlendStack.Last();
			FRotator AggregateRot = LastCamEntry.LastPOV.Rotation;
			float AggregateWeight = LastCamEntry.BlendWeight;
			for (int Idx = CameraBlendStack.Num() - 2; Idx >= 0; --Idx)
			{
				FActiveCitySampleCamera& CamEntry = CameraBlendStack[Idx];
				FRotator BlendedRot = BlendRots(CamEntry.LastPOV.Rotation, CamEntry.BlendWeight, AggregateRot, AggregateWeight);
				AggregateRot = BlendedRot;
				AggregateWeight += CamEntry.BlendWeight;
			}

			OutVT.POV.Rotation = AggregateRot;
		}

		TransitionGoalPOV = OutVT.POV;
	}

	// Apply camera modifiers at the end (view shakes for example)
	ApplyCameraModifiers(DeltaTime, OutVT.POV);

	// Synchronize the actor with the view target results
	SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);

	// keep camera out of player
	// NOTE: this assumes one single player character
	if (PCOwner)
	{
		ACharacter* const PlayerChar = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (CamActor)
		{
			// assume whoever is controlling the character knows what they are doing
			PCOwner->HiddenActors.Remove(PlayerChar);
		}
		else
		{
			const UCapsuleComponent* const Capsule = PlayerChar ? PlayerChar->GetCapsuleComponent() : nullptr;
			if (Capsule)
			{
				// assumes upright capsule
				const FVector CapsuleCenter = Capsule->GetComponentLocation();
				const FVector Offset = FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight_WithoutHemisphere());
				const FVector BottomCapsulePoint = CapsuleCenter - Offset;
				const FVector TopCapsulePoint = CapsuleCenter + Offset;
				const float DistToSegment = FMath::PointDistToSegment(OutVT.POV.Location, BottomCapsulePoint, TopCapsulePoint);

				const float HideCharacterDistPadding = 15.f;
				if (DistToSegment < (Capsule->GetScaledCapsuleRadius() + HideCharacterDistPadding))
				{
					PCOwner->HiddenActors.AddUnique(PlayerChar);
				}
				else
				{
					PCOwner->HiddenActors.Remove(PlayerChar);
				}
			}
		}

	}

	UpdateCameraLensEffects(OutVT);

	// if engine-level camera wants control (e.g. for debug cameras), let it override what we've done here
	if (CameraStyle != NAME_Default)
	{
		OutVT.POV = OriginalPOV;
		Super::UpdateViewTarget(OutVT, DeltaTime);
	}
}

void ACitySamplePlayerCameraManager::CleanUpOutdatedCameraModeInstances()
{
	for (int32 RemoveCheckIndex = CameraModeInstances.Num() - 1; RemoveCheckIndex >= 0; RemoveCheckIndex--)
	{
		if (CameraModeInstances[RemoveCheckIndex].ViewTarget == nullptr)
		{
			CameraModeInstances.RemoveAt(RemoveCheckIndex);
		}
	}
}

// assumes valid inputs
int32 ACitySamplePlayerCameraManager::FindOrCreateCameraModeInstance(TSubclassOf<UCitySampleCameraMode> CameraModeClass, AActor* InViewTarget)
{
	for (int Idx = 0; Idx < CameraModeInstances.Num(); ++Idx)
	{
		const FCitySampleCameraModeInstance& Inst = CameraModeInstances[Idx];
		if ((Inst.CameraModeClass == CameraModeClass) && (Inst.ViewTarget == InViewTarget) && (Inst.CameraMode))
		{
			return Idx;
		}
	}

	// no camera mode found, so we create one
	UCitySampleCameraMode* const NewCameraMode = NewObject<UCitySampleCameraMode>(this, CameraModeClass);

	NewCameraMode->PlayerCamera = this;

	FCitySampleCameraModeInstance NewInstance;
	NewInstance.CameraModeClass = CameraModeClass;
	NewInstance.ViewTarget = InViewTarget;
	NewInstance.CameraMode = NewCameraMode;

	UCineCameraComponent* NewCineComp;
	NewCineComp = NewObject<UCineCameraComponent>(this);
	NewCineComp->AttachToComponent(GetTransformComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
	NewCineComp->SetRelativeTransform(FTransform::Identity);
	NewCineComp->SetConstraintAspectRatio(false);
	if (NewCameraMode->bOverrideFilmback)
	{
		NewCineComp->Filmback = NewCameraMode->CineCam_FilmbackOverride;
	}
	NewInstance.CineCameraComponent = NewCineComp;

	int32 NewIdx = CameraModeInstances.Emplace(NewInstance);

	return NewIdx;
}

int32 ACitySamplePlayerCameraManager::GetBestCameraMode(AActor* Target)
{
	// Find which CitySampleCameraMode Class we should be using for this ViewTarget.
	TSubclassOf<UCitySampleCameraMode> CameraModeClass = DetermineBestCameraClass(Target);
	if (!CameraModeClass)
	{
		// Making sure we always have a valid class
		CameraModeClass = UCitySampleCameraMode::StaticClass();
	}

	return FindOrCreateCameraModeInstance(CameraModeClass, Target);
}

namespace DebugCameraStyles
{
	static const FName NAME_Fixed = FName(TEXT("Fixed"));
	static const FName NAME_ThirdPerson = FName(TEXT("ThirdPerson"));
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	static const FName NAME_FreeCam_Default = FName(TEXT("FreeCam_Default"));
	static const FName NAME_FirstPerson = FName(TEXT("FirstPerson"));
}

void ACitySamplePlayerCameraManager::SetDebugCameraStyle(EDebugCameraStyle NewDebugCameraStyle)
{
	switch (NewDebugCameraStyle)
	{
	case EDebugCameraStyle::None:
		CameraStyle = NAME_Default;
		break;
	case EDebugCameraStyle::Fixed:
		CameraStyle = DebugCameraStyles::NAME_Fixed;
		break;
	case EDebugCameraStyle::ThirdPerson:
		CameraStyle = DebugCameraStyles::NAME_ThirdPerson;
		break;
	case EDebugCameraStyle::FreeCam:
		CameraStyle = DebugCameraStyles::NAME_FreeCam;
		break;
	case EDebugCameraStyle::FreeCam_Default:
		CameraStyle = DebugCameraStyles::NAME_FreeCam_Default;
		break;
	case EDebugCameraStyle::FirstPerson:
		CameraStyle = DebugCameraStyles::NAME_FirstPerson;
		break;
	default:
		break;
	}
}

void ACitySamplePlayerCameraManager::SetViewPitchLimits(float MinPitch, float MaxPitch)
{
	ViewPitchMin = MinPitch;
	ViewPitchMax = MaxPitch;
}

void ACitySamplePlayerCameraManager::ResetViewPitchLimits()
{
	SetViewPitchLimits(DefaultMinPitchLimit, DefaultMaxPitchLimit);
}

TSubclassOf<UCitySampleCameraMode> ACitySamplePlayerCameraManager::GetCameraClassForCharacter_Implementation(const AActor* InViewTarget) const
{
	return nullptr;
}

float ACitySamplePlayerCameraManager::GetModeTransitionTime(UCitySampleCameraMode* ToMode) const
{
	if (ToMode)
	{
		return ToMode->GetTransitionTime();
	}

	return 0.f;
}

TSubclassOf<UCitySampleCameraMode> ACitySamplePlayerCameraManager::DetermineBestCameraClass(AActor const* InViewTarget) const
{
	if (bUsingAltCameraMode && HasAlternateCameraAvailable())
	{
		return AltCameraMode;
	}

	TSubclassOf<UCitySampleCameraMode> BestCameraType = UCitySampleCameraMode::StaticClass();

	TSubclassOf<class UCitySampleCameraMode> CameraClass = GetCameraClassForCharacter(InViewTarget);
	if (CameraClass)
	{
		BestCameraType = CameraClass;
	}

	// Make sure we return a valid camera type
	return BestCameraType;
}

void FCitySampleCameraModeInstance::UpdateCamera(float DeltaTime, FTViewTarget& OutVT)
{
	CameraMode->UpdateCamera(ViewTarget, (CameraMode->bUseCineCam ? CineCameraComponent : nullptr), DeltaTime, OutVT);
}
