// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCamera_PhotoMode.h"

#include "Camera/PhotoModeComponent.h"
#include "CineCameraComponent.h"
#include "Game/CitySamplePlayerController.h"
#include "GameFramework/DefaultPawn.h"
#include "Interfaces/Interface_PostProcessVolume.h"

static TAutoConsoleVariable<bool> CVarPhotoModeAutoFocusTraceDebug(
	TEXT("PhotoMode.DrawAutoFocusTraceDebug"),
	false,
	TEXT("Toggle for displaying auto focus trace debug info"),
	ECVF_Default);

UCitySampleCamera_PhotoMode::UCitySampleCamera_PhotoMode()
{
	FOV = 75.f;
	bUseCineCamSettings = true;
	bUseCineCam = true;

	AsyncFocusDistance = -1;
	AsyncFocusTraceDelegate.BindUObject(this, &UCitySampleCamera_PhotoMode::HandleAsyncFocusTrace);
}

void UCitySampleCamera_PhotoMode::UpdateCamera(class AActor* ViewTarget, UCineCameraComponent* CineCamComp, float DeltaTime, FTViewTarget& OutVT)
{
	UPhotoModeComponent* const PhotoModeComponent = GetPhotoModeComponent();
	if (!PhotoModeComponent)
	{
		return;
	}

	const FPhotoModeSettings& Settings = PhotoModeComponent->GetPhotoModeSettings();

	const FVector DesiredViewLocation = ViewTarget->GetActorLocation();
	OutVT.POV.Location = DesiredViewLocation;

	UpdateFinalRotation(ViewTarget, Settings, OutVT);

	LastCameraToWorld = FTransform(OutVT.POV.Rotation, OutVT.POV.Location);

	bUseCustomFocusDistance = true;
	CineCam_FocusDistanceAdjustment = Settings.FocusDistanceAdjustment;
	CineCam_CurrentAperture = Settings.CurrentAperture;
	CineCam_CurrentFocalLength = Settings.CurrentFocalLength;

	ApplyCineCamSettings(OutVT, CineCamComp, DeltaTime);
	UpdateExposureSettings(Settings, OutVT);
	UpdateFocusSettings(PhotoModeComponent, Settings, CineCamComp, OutVT);
}

void UCitySampleCamera_PhotoMode::OnBecomeActive(AActor* ViewTarget, UCitySampleCameraMode* PreviouslyActiveMode)
{
	Super::OnBecomeActive(ViewTarget, PreviouslyActiveMode);

	// We want to iterate through the PPVolumes in our world and grab the first auto exposure bias being used.
	// CitySample will only ever have one relevant PPVolume in this case, so we just grab our first valid bias and get out
	const UWorld* World = GetWorld();
	if (World != nullptr)
	{
		for (const IInterface_PostProcessVolume* const PPVolume : World->PostProcessVolumes)
		{
			const FPostProcessSettings* const PPSettings = PPVolume->GetProperties().Settings;
			if (PPSettings != nullptr && PPSettings->bOverride_AutoExposureBias > 0)
			{
				BaseExposureBias = PPSettings->AutoExposureBias;
				break;
			}
		}
	}

	if (UPhotoModeComponent* const PhotoModeComponent = GetPhotoModeComponent())
	{
		PhotoModeComponent->OnPhotoModeActivated_Internal(this);
	}
}

class UPhotoModeComponent* UCitySampleCamera_PhotoMode::GetPhotoModeComponent() const
{
	if (ACitySamplePlayerController* const OwningPC = GetOwningCitySamplePC())
	{
		return OwningPC->FindComponentByClass<UPhotoModeComponent>();
	}

	return nullptr;
}

void UCitySampleCamera_PhotoMode::UpdateExposureSettings(
	const struct FPhotoModeSettings& Settings,
	struct FTViewTarget& OutVT) const
{
	OutVT.POV.PostProcessSettings.bOverride_AutoExposureSpeedUp = true;
	OutVT.POV.PostProcessSettings.AutoExposureSpeedUp = -1.f;

	OutVT.POV.PostProcessSettings.bOverride_AutoExposureSpeedDown = true;
	OutVT.POV.PostProcessSettings.AutoExposureSpeedDown = -1.f;

	OutVT.POV.PostProcessSettings.bOverride_AutoExposureBias = true;

	const float NewExposureBias = BaseExposureBias + Settings.ManualExposureBias;
	OutVT.POV.PostProcessSettings.AutoExposureBias = NewExposureBias;
}

void UCitySampleCamera_PhotoMode::UpdateFocusSettings(
		UPhotoModeComponent* PhotoModeComponent,
		const struct FPhotoModeSettings& Settings,
		UCineCameraComponent* CineCamComp,
		const struct FTViewTarget& VT)
{
	//If Autofocus is enabled, we want to trigger a raycast to set our GoalFocusDistance
	if (Settings.bAutoFocusEnabled)
	{
		// trace to get depth at center of screen
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PhotoModeUpdateFocus), true, VT.Target);
		FCollisionShape TraceShape = FCollisionShape::MakeSphere(SphereTraceRadius);
		FHitResult Hit;

		const FVector TraceStart = VT.POV.Location;
		const FVector TraceEnd = TraceStart + VT.POV.Rotation.Vector() * PhotoModeAutoFocusTraceDistance;

		//Send an Async cast, idea is that we only send this when we need to otherwise
		FTraceHandle ASyncFocusTrace = GetWorld()->AsyncSweepByChannel(
			EAsyncTraceType::Single,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			ECC_WorldStatic,
			TraceShape,
			TraceParams,
			FCollisionResponseParams::DefaultResponseParam,
			&AsyncFocusTraceDelegate
		);

		float AutoFocusDistance = AsyncFocusDistance;
		if (AutoFocusDistance == -1)
		{
			bool const bHit = GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_WorldStatic, TraceShape, TraceParams);
			AutoFocusDistance = bHit ? (TraceStart - Hit.ImpactPoint).Size() : PhotoModeAutoFocusTraceDistance;
		}

		PhotoModeComponent->SetGoalAutoFocusDistance(AutoFocusDistance);
	}
	else  
	{
		AsyncFocusDistance = -1;
	}

	// This value is later fetched via BP and fed into the BP Implementable Event: "GetCustomFocusDistance", which is used to feed a custom focus distance to cinecam
	CustomFocusDistance = Settings.ManualFocusDistance;
	ensure(CustomFocusDistance >= 0.f);
}

void UCitySampleCamera_PhotoMode::UpdateFinalRotation(const AActor* ViewTarget, const struct FPhotoModeSettings& Settings, struct FTViewTarget& OutVT)
{
	FRotator DesiredViewRotation;
	const ADefaultPawn* const ViewTargetPawn = Cast<ADefaultPawn>(ViewTarget);
	if (ViewTargetPawn)
	{
		DesiredViewRotation = ViewTargetPawn->GetViewRotation();
	}

	OutVT.POV.Rotation = DesiredViewRotation;
}

void UCitySampleCamera_PhotoMode::HandleAsyncFocusTrace(const FTraceHandle& InTraceHandle, FTraceDatum& InTraceDatum)
{
	if (InTraceDatum.OutHits.Num() > 0)
	{
		const FHitResult& Hit = InTraceDatum.OutHits[0];
		AsyncFocusDistance = (Hit.TraceStart - Hit.ImpactPoint).Size();

#if !(UE_BUILD_SHIPPING)
		if (CVarPhotoModeAutoFocusTraceDebug.GetValueOnGameThread() && GEngine != nullptr)
		{
			const FString DisplayString = FString::Printf(TEXT("Object Hit: %s"), *Hit.HitObjectHandle.GetName());
			GEngine->AddOnScreenDebugMessage(30, 2.0f, FColor::Purple, *DisplayString);
			::DrawDebugSphere(GetWorld(), Hit.Location, SphereTraceRadius, 12.f, FColor::Green);
		}
#endif
	}
	else
	{
		AsyncFocusDistance = PhotoModeAutoFocusTraceDistance;
	}
}
