// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/PhotoModeComponent.h"

#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "HoverDroneMovementComponent.h"

#include "Camera/CitySampleCamera_PhotoMode.h"
#include "Camera/CitySamplePlayerCameraManager.h"
#include "Character/CitySampleHoverDrone.h"
#include "CitySample.h"
#include "Game/CitySampleGameState.h"
#include "Game/CitySamplePlayerController.h"
#include "UI/CitySampleUIComponent.h"
#include "Vehicles/CitySampleVehicleBase.h"
#include "Vehicles/DrivableVehicleComponent.h"
#include "Util/CitySampleBlueprintLibrary.h"


DEFINE_LOG_CATEGORY(LogCitySamplePhotoMode)

UPhotoModeComponent::UPhotoModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	PhotoModeClass = UCitySampleCamera_PhotoMode::StaticClass();
}

void UPhotoModeComponent::OnComponentCreated()
{
	SaveCurrentSettingsAsDefault();
}

void UPhotoModeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bIsPhotoModeStillValid = false;
	if (const ACitySamplePlayerController* const PlayerController = GetOwner<ACitySamplePlayerController>())
	{
		if (RequestingPlayer == PlayerController && !RequestingPlayer->IsPendingKillPending())
		{
			if (ACitySamplePlayerCameraManager* const CameraManager = RequestingPlayer->GetCitySampleCameraManager())
			{
				if (CameraManager->IsUsingAlternateCamera() &&
					CameraManager->AltCameraMode == PhotoModeClass.Get())
				{
					bIsPhotoModeStillValid = true;

					// Make sure our HoverDroneMovementComponent is working with an up-to-date FOV
					if (HoverDroneMovementComp != nullptr)
					{
						const UCitySampleCameraMode* CurrentCameraMode = CameraManager->GetCurrentCameraMode();
						if (CurrentCameraMode != nullptr)
						{
							HoverDroneMovementComp->SetCurrentFOV(CurrentCameraMode->CineCam_DisplayOnly_FOV);
						}
					}
				}
			}
		}
	}

	if (!bIsPhotoModeStillValid)
	{
		DeactivatePhotoMode();
	}
	else if (Settings.bAutoFocusEnabled)
	{
		if (FMath::IsNearlyEqual(Settings.ManualFocusDistance, GoalAutoFocusDistance, 0.1f))
		{
			AutoFocusDistanceInterpolator.Reset();
		}

		Settings.ManualFocusDistance = AutoFocusDistanceInterpolator.Eval(GoalAutoFocusDistance, DeltaTime);
	}
}

bool UPhotoModeComponent::ActivatePhotoMode()
{
	if (EPhotoModeState::NotActive != State)
	{
		UE_LOG(LogCitySamplePhotoMode, Log, TEXT("UPhotoModeComponent::ActivatePhotoMode - Attempted to activate photomode but photomode was already in an active state."));

		return true;
	}

	ACitySamplePlayerController* const CitySamplePlayerController = GetOwner<ACitySamplePlayerController>();
	if (!CitySamplePlayerController || !CitySamplePlayerController->IsLocalController())
	{
		UE_LOG(LogCitySamplePhotoMode, Log, TEXT("UPhotoModeComponent::ActivatePhotoMode - Attempted to activate photomode but failed to find valid player controller."));

		return false;
	}
	
	ACitySamplePlayerCameraManager* const CitySamplePlayerCameraManager = CitySamplePlayerController->GetCitySampleCameraManager();
	if (!CitySamplePlayerCameraManager)
	{
		UE_LOG(LogCitySamplePhotoMode, Log, TEXT("UPhotoModeComponent::ActivatePhotoMode - Attempted to activate photomode but failed to find valid player camera manager."));

		return false;
	}
	
	RequestingPlayer = CitySamplePlayerController;
	CitySamplePlayerCameraManager->ConfigureAlternateCamera(PhotoModeClass, nullptr, 0.f);
	CitySamplePlayerCameraManager->SetUsingAlternateCamera(true);
	State = EPhotoModeState::Pending;

	UCitySampleUIComponent* const UIComp = RequestingPlayer->GetCitySampleUIComponent();
	if (UIComp != nullptr)
	{
		UIComp->OnHideUI.AddDynamic(this, &ThisClass::OnHideUIToggle);
	}
	else
	{
		UE_LOG(LogCitySamplePhotoMode, Warning, TEXT("UPhotoModeComponent::ActivatePhotoMode - Failed to bind to CitySampleUIComponent's 'OnHideUI' multicast delegate"));
	}

	OldPawn = RequestingPlayer->GetPawn();

	if (UDrivableVehicleComponent* const DrivableComponent = OldPawn->FindComponentByClass<UDrivableVehicleComponent>())
	{
		DrivableComponent->OnDriverExit.AddDynamic(this, &UPhotoModeComponent::OnVehicleDriverExit);
	}

	const FMinimalViewInfo POV = CitySamplePlayerCameraManager->ViewTarget.POV;
	CameraPawn = Cast<ACitySampleHoverDrone>(GetWorld()->SpawnActor(PhotomodePawnClass.Get(), &POV.Location, &POV.Rotation));
	HoverDroneMovementComp = CameraPawn->FindComponentByClass<UHoverDroneMovementComponent>();

	StartingLocation = CameraPawn->GetActorLocation();
	StartingRotation = CameraPawn->GetActorRotation();

	//Make it so that our new pawn doesn't create physics interactions with physics simulating entities
	if (USphereComponent* const CollisionComp = CameraPawn->GetCollisionComponent())
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (UMeshComponent* const MeshComp = CameraPawn->GetMeshComponent())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ACitySampleGameState* const CitySampleGameState = UCitySampleBlueprintLibrary::GetCitySampleGameState(GetWorld());
	if (CitySampleGameState != nullptr)
	{
		CitySampleGameState->OnEnterPhotomode.Broadcast();
	}

	RequestingPlayer->Possess(CameraPawn);

	OnActivatePhotoMode();

	UE_LOG(LogCitySamplePhotoMode, Log, TEXT("UPhotoModeComponent::ActivatePhotoMode - Photomode successfully activated. Photomode pawn spanwed and awaiting camera mode transition."));

	return true;
}

void UPhotoModeComponent::DeactivatePhotoMode()
{
	if (!RequestingPlayer)
	{
		UE_LOG(LogCitySamplePhotoMode, Warning, TEXT("UPhotoModeComponent::DeactivatePhotomode - Attempted to deactivate photomode but Requesting Player Controller reference was null. Please investigate."));

		return;
	}

	RemovePhotomodeInputContext();

	if (UDrivableVehicleComponent* const DrivableComponent = OldPawn->FindComponentByClass<UDrivableVehicleComponent>())
	{
		DrivableComponent->OnDriverExit.RemoveDynamic(this, &UPhotoModeComponent::OnVehicleDriverExit);
	}

	if (UCitySampleUIComponent* const CitySampleUI = RequestingPlayer->GetCitySampleUIComponent())
	{
		CitySampleUI->OnHideUI.RemoveDynamic(this, &ThisClass::OnHideUIToggle);

		if (!CitySampleUI->IsOptionsMenuActive())
		{
			RequestingPlayer->AddPawnInputContext(OldPawn);
		}
	}

	if (EPhotoModeState::NotActive != State)
	{
		ACitySamplePlayerCameraManager* CitySamplePlayerCameraManager = RequestingPlayer->GetCitySampleCameraManager();
		if (CitySamplePlayerCameraManager &&
			CitySamplePlayerCameraManager->bUsingAltCameraMode &&
			CitySamplePlayerCameraManager->AltCameraMode == PhotoModeClass.Get())
		{
			CitySamplePlayerCameraManager->ConfigureAlternateCamera(nullptr, nullptr, 0.f);
			CitySamplePlayerCameraManager->SetUsingAlternateCamera(false);
		}
	}

	if (CameraPawn)
	{
		EndingLocation = CameraPawn->GetActorLocation();
		EndingRotation = CameraPawn->GetActorRotation();

		CameraPawn->Destroy();
		CameraPawn = nullptr;
		HoverDroneMovementComp = nullptr;
	}

	bool bValidOldPawn = OldPawn != nullptr;

	// Vehicles need to still be driven by the local player or it means it went into the pool and destroyed the driving pawn
	if (const ACitySampleVehicleBase* const Vehicle = Cast<ACitySampleVehicleBase>(OldPawn))
	{
		const ACitySampleCharacter* const Driver = Vehicle->GetOccupantInSeat(ECitySampleVehicleSeat::Driver);
		if (Driver == nullptr)
		{
			bValidOldPawn = false;
		}
	}

	bWasOldPawnADrone = false;

	if (bValidOldPawn)
	{
		/**If our old pawn was an instance of CitySamplePC's drone pawn class, it's safe to assume we're returning back to drone mode.
		   We spawn a new drone actor instead of reusing the previous one because the current CitySampleHoverDrone implementation does some
		   movement work under the hood that undos any direct rotation changes to the actor or control rotation. Spawning a fresh instance ensures we get the correct behavior.*/
		UClass* const DroneClass = RequestingPlayer->GetDronePawnClass();
		if (OldPawn->IsA(DroneClass))
		{
			OldPawn->Destroy();

			OldPawn = Cast<APawn>(GetWorld()->SpawnActor(DroneClass, &EndingLocation, &EndingRotation));
			bWasOldPawnADrone = true;
		}

		RequestingPlayer->Possess(OldPawn);
	}
	else
	{
		RequestingPlayer->SpawnNewPawnLeavingPhotoMode();
	}
	
	if (RequestingPlayer->GetPawn())
	{
		const float TravelDistance = (RequestingPlayer->GetPawn()->GetActorLocation() - EndingLocation).Size2D();
		if (TravelDistance > RequestingPlayer->GetCameraTransitionLevelStreamDistance2D())
		{
			RequestingPlayer->HandleLongDistanceCameraTransition();
		}
	}

	OldPawn = nullptr;

	RequestingPlayer = nullptr;
	State = EPhotoModeState::NotActive;
	SetComponentTickEnabled(false);

	ACitySampleGameState* const CitySampleGameState = UCitySampleBlueprintLibrary::GetCitySampleGameState(GetWorld());
	if (CitySampleGameState != nullptr)
	{
		CitySampleGameState->OnExitPhotomode.Broadcast();
	}

	OnDeactivatePhotoMode();

	UE_LOG(LogCitySamplePhotoMode, Log, TEXT("UPhotoModeComponent::DeactivatePhotomode - Photomode successfully deactivated."));
}

void UPhotoModeComponent::AddPhotomodeInputContext()
{
	ULocalPlayer* const LocalPlayer = RequestingPlayer != nullptr ? RequestingPlayer->GetLocalPlayer() : nullptr;
	if (LocalPlayer != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(ActiveInputMappingContext, InputMappingPriority);
		}
	}
}

void UPhotoModeComponent::RemovePhotomodeInputContext()
{
	ULocalPlayer* const LocalPlayer = RequestingPlayer != nullptr ? RequestingPlayer->GetLocalPlayer() : nullptr;
	if (LocalPlayer != nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(ActiveInputMappingContext);
		}
	}
}

void UPhotoModeComponent::EnableAutoFocus()
{
	Settings.bAutoFocusEnabled = true;

	AutoFocusDistanceInterpolator.SetInitialValue(Settings.ManualFocusDistance);
	SetGoalAutoFocusDistance(Settings.ManualFocusDistance);

	OnEnableAutoFocus();
}

void UPhotoModeComponent::DisableAutoFocus()
{
	Settings.bAutoFocusEnabled = false;
	OnDisableAutoFocus();
}

void UPhotoModeComponent::SaveCurrentSettingsAsDefault()
{	
	DefaultSettings = Settings;
}

void UPhotoModeComponent::SetUpInputs()
{
	ACitySamplePlayerController* const CitySamplePlayerController = GetOwner<ACitySamplePlayerController>();
	if (!CitySamplePlayerController || !CitySamplePlayerController->IsLocalController())
	{
		return;
	}

	if (UEnhancedInputComponent* const EnhancedInputComponent = CitySamplePlayerController->FindComponentByClass<UEnhancedInputComponent>())
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::MoveActionBinding);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::LookActionBinding);
		EnhancedInputComponent->BindAction(ChangeAltitudeAction, ETriggerEvent::Triggered, this, &ThisClass::ChangeAltitudeActionBinding);
		
		EnhancedInputComponent->BindAction(UseAutoFocusAction, ETriggerEvent::Started, this, &ThisClass::EnableAutoFocusActionBinding);

		EnhancedInputComponent->BindAction(UseAutoFocusAction, ETriggerEvent::Completed, this, &ThisClass::DisableAutoFocusActionBinding);
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("Failed to setup player input for %s, InputComponent type is not UEnhancedInputComponent."), *GetName());
	}
}

bool UPhotoModeComponent::ShouldTriggerFadeOut() const
{
	const float PawnDistance = FVector::Distance(StartingLocation, EndingLocation);

	const FRotator RotatorDelta = StartingRotation - EndingRotation;
	const float BiggestRotDiff = FMath::Max3(FMath::Abs(RotatorDelta.Roll), FMath::Abs(RotatorDelta.Pitch), FMath::Abs(RotatorDelta.Yaw));

	return (PawnDistance >= FadeOutDistanceThreshold || BiggestRotDiff >= FadeOutRotationThreshold) && !bWasOldPawnADrone;
}

void UPhotoModeComponent::OnPhotoModeActivated_Internal(UCitySampleCamera_PhotoMode* ActivatedPhotoMode)
{
	State = EPhotoModeState::Active;

	if (ActivatedPhotoMode)
	{
		if (ACitySamplePlayerController* const LocalRequestingPlayer = Cast<ACitySamplePlayerController>(ActivatedPhotoMode->PlayerCamera->PCOwner))
		{
			RequestingPlayer = LocalRequestingPlayer;
			SetComponentTickEnabled(true);

			AddPhotomodeInputContext();

			if (const UCitySampleUIComponent* const CitySampleUI = RequestingPlayer->GetCitySampleUIComponent())
			{
				if (!CitySampleUI->IsOptionsMenuActive())
				{
					LocalRequestingPlayer->RemovePawnInputContext(OldPawn);
				}
			}
		}

		UE_LOG(LogCitySamplePhotoMode, Log, TEXT("UPhotoModeComponent::OnPhotoModeActivated_Internal - Full transition to photomode complete. Photomode camera mode is active."));
	}
	else
	{
		UE_LOG(LogCitySamplePhotoMode, Warning, TEXT("UPhotoModeComponent::OnPhotoModeActivated_Internal - Full transition to photomode complete but an invalid camera mode was passed in. Please investigate the CitySampleCameraManager."));
	}
}

void UPhotoModeComponent::MoveForward(float Val)
{
	if (CameraPawn)
	{
		CameraPawn->MoveForward(Val * HorizontalMovementRate);
	}
}

void UPhotoModeComponent::MoveRight(float Val)
{
	if (CameraPawn)
	{
		CameraPawn->MoveRight(Val * HorizontalMovementRate);
	}
}

void UPhotoModeComponent::MoveUp(float Val)
{
	if (CameraPawn)
	{
		CameraPawn->MoveUp(Val * VerticalMovementRate);
	}
}

void UPhotoModeComponent::TurnAtRate(float Rate)
{
	if (CameraPawn)
	{
		CameraPawn->TurnAccel(Rate);
	}
}

void UPhotoModeComponent::LookUpAtRate(float Rate)
{
	if (CameraPawn)
	{
		CameraPawn->LookUpAccel(Rate);
	}
}

void UPhotoModeComponent::OnVehicleDriverExit(UDrivableVehicleComponent* const DrivableComponent, ACitySampleVehicleBase* const Vehicle, APawn* const Driver)
{
	if (Vehicle == OldPawn && Driver)
	{
		OldPawn = Driver;
	}

	DrivableComponent->OnDriverExit.RemoveDynamic(this, &UPhotoModeComponent::OnVehicleDriverExit);
}

void UPhotoModeComponent::MoveActionBinding(const FInputActionValue& ActionValue)
{
	const FInputActionValue::Axis2D AxisValue = ActionValue.Get<FInputActionValue::Axis2D>();
	MoveForward(AxisValue.X);
	MoveRight(AxisValue.Y);
}

void UPhotoModeComponent::LookActionBinding(const FInputActionValue& ActionValue)
{
	const FInputActionValue::Axis2D AxisValue = ActionValue.Get<FInputActionValue::Axis2D>();
	TurnAtRate(AxisValue.X);
	LookUpAtRate(AxisValue.Y);
}

void UPhotoModeComponent::ChangeAltitudeActionBinding(const FInputActionValue& ActionValue)
{
	MoveUp(ActionValue.Get<FInputActionValue::Axis1D>());
}

void UPhotoModeComponent::EnableAutoFocusActionBinding()
{
	EnableAutoFocus();
}

void UPhotoModeComponent::DisableAutoFocusActionBinding()
{
	DisableAutoFocus();
}
