// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleVehicleBase.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Rendering/MotionVectorSimulation.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MassActorHelper.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassTrafficFragments.h"
#include "MassActorSubsystem.h"

#include "Character/CitySampleCharacter.h"
#include "CitySample.h"
#include "Game/CitySampleGameState.h"
#include "Game/CitySamplePlayerController.h"
#include "Util/CitySampleBlueprintLibrary.h"
#include "MassAgentComponent.h"

float MaxBlendTransitionTime = 3.0f;
FAutoConsoleVariableRef CVarMaxBlendTransitionTime(TEXT("p.Vehicle.MaxBlendTransitionTime"), MaxBlendTransitionTime, TEXT("Time to transition from low LOD to High LOD vehicles."));
float BlendTransitionVelocity = 3.0f;
FAutoConsoleVariableRef CVarMaxBlendTransitionVelocity(TEXT("p.Vehicle.BlendTransitionVelocity"), BlendTransitionVelocity, TEXT("Velocity below which LOD blend will occur."));
int bLODBlendEnabled = true;
FAutoConsoleVariableRef CVarChaosVehicleLODBlendEnabled(TEXT("p.Vehicle.LODBlendEnabled"), bLODBlendEnabled, TEXT("Enable/Disable vehicle LOD blend damping."));
int bShowLODDamping = false;
FAutoConsoleVariableRef CVarChaosVehicleShowLODDamping(TEXT("p.Vehicle.ShowLODDamping"), bShowLODDamping, TEXT("Enable/Disable vehicle LOD damping debug graphics."));
int bShowSleepState = false;
FAutoConsoleVariableRef CVarChaosVehicleShowSleepState(TEXT("p.Vehicle.ShowSleepState"), bShowSleepState, TEXT("Enable/Disable vehicle sleep state debug graphics."));

bool bDrawCenterOfMassDebug = false;
FAutoConsoleVariableRef CVarDrawCenterOfMassDebug(TEXT("p.Vehicle.DrawCenterOfMassDebug"), bDrawCenterOfMassDebug, TEXT("Enable/Disable Center of Mass debug widget used to help tune vehicles."));

bool bDrawRumbleInfoDebug = false;
FAutoConsoleVariableRef CVarDrawRumbleDebugInfo(TEXT("p.Vehicle.DrawRumbleDebugInfo"), bDrawRumbleInfoDebug, TEXT("Enable/Disable vehicle rumble debug info"));

ACitySampleVehicleBase::ACitySampleVehicleBase()
{
	bHideDrivingUI = false;
	TeleportOffsetStep = 100.0f;
	FlipVehicleMaxDistance = 3000.0f;
	bBurnoutActivated = false;
	DefaultAngularDamping = 0.0f;

	CCDSpeedThreshold = 60.0f;

	const int32 DriveableVehicleWheelCount = 4;
	WheelSuspensionStates.Init(FCitySampleWheelSuspensionState(), DriveableVehicleWheelCount);

	TransformsOverTime.AddDefaulted(4);

	CreateDefaultSubobject<UMassAgentComponent>(TEXT("MassAgent"));
}

void ACitySampleVehicleBase::BeginPlay()
{
	Super::BeginPlay();

	// Because we don't need an AI controller to move. MassTraffic will take care of that in the AI case and
	// the player controller in the player case.
	GetVehicleMovementComponent()->SetRequiresControllerForInputs(false);

	UpdateDrivingState();

	const FBodyInstance* BodyInstance = GetMesh()->GetBodyInstance();
	if (BodyInstance != nullptr)
	{
		DefaultCenterOfMassOffset = BodyInstance->COMNudge;
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("Failed to find FBodyInstance on vehicle, any center of mass adjustments won't be properly applied."));
	}

	InitializeCenterOfMassOffsets();
	InitializeAngularDampingDefault();

	LocationLastTick = GetActorLocation();

	ParkingReleaseTimerDelegate.BindUFunction(this, FName("SetParked"), false);
}

void ACitySampleVehicleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MeasuredVelocity = (GetActorLocation() - LocationLastTick) / DeltaTime;

	UpdateDrivingState();

	UpdateAngularDamping();

	UpdateCenterOfMassOffsets(DeltaTime);

	UpdateWheelFrictionMultiplierAdjustments();

	UpdateTimeSpentSharpSteering(DeltaTime);

	UpdateVehicleDriftingState();

	if (bIsPossessedByPlayer)
	{
		UpdateBurnout(DeltaTime);
		UpdateControllerRumble();
		UpdateWheelSuspensionStates(DeltaTime);
	}

	if (bLODBlendEnabled)
	{
		UpdateLODBlend(DeltaTime);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UpdateDebugDraw();
#endif
}

void ACitySampleVehicleBase::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::LookActionBinding);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &ThisClass::LookActionBinding);

		EnhancedInputComponent->BindAction(LookDeltaAction, ETriggerEvent::Triggered, this, &ThisClass::LookDeltaActionBinding);

		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ThisClass::ThrottleActionBinding);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ThisClass::ThrottleActionBinding);

		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ThisClass::BrakeActionBinding);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ThisClass::BrakeActionBinding);

		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ThisClass::SteeringActionBinding);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ThisClass::SteeringActionBinding);
		 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ThisClass::HandbrakeActionBinding);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ThisClass::HandbrakeActionBinding);
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("Failed to setup player input for %s, InputComponent type is not UEnhancedInputComponent."), *GetName());
	}
}

void ACitySampleVehicleBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	ACitySamplePlayerController* CitySamplePlayerController = UCitySampleBlueprintLibrary::GetCitySamplePlayerController(GetWorld());
	if (CitySamplePlayerController != nullptr && CitySamplePlayerController == NewController)
	{
		bIsPossessedByPlayer = true;
		
		SpeedRumbleHandle = CitySamplePlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Start);
		CorneringRumbleHandle = CitySamplePlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Start);
		LeftSurfaceRumbleHandle = CitySamplePlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Start);
		RightSurfaceRumbleHandle = CitySamplePlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Start);

		OnIgnitionStart.AddDynamic(this, &ACitySampleVehicleBase::StartParkingReleaseTimer);
	
		if (ACitySampleGameState* const CitySampleGameState = UCitySampleBlueprintLibrary::GetCitySampleGameState(GetWorld()))
		{
			CitySampleGameState->OnEnterPhotomode.AddDynamic(this, &ThisClass::OnEnterPhotomode);
			CitySampleGameState->OnExitPhotomode.AddDynamic(this, &ThisClass::OnExitPhotomode);
		}

		if (UCitySampleUIComponent* const UIComponent = CitySamplePlayerController->GetCitySampleUIComponent())
		{
			UIComponent->OnOptionsMenuOpen.AddDynamic(this, &ThisClass::OnOptionsMenuOpened);
		}

		OnPlayerEnterVehicle.Broadcast();

		const UWorld* World = GetWorld();
		{
			// When removing this player vehicle tag whe have to do it this slightly more complex way
			// instead of UE::MassActor::RemoveEntityTagFromActor<FTagFragment_MassTrafficPlayerVehicle>(*PreviousPlayerVehicle);
			// because the actor might not actually exist any more. This entity might be represented by an ISMC or something
			// else. By using the handle we know we're changing the Mass entity no matter what it is now.
			const FMassEntityHandle & MassAgentHandle = CitySamplePlayerController->GetCurrentPlayerVehicleMassHandle();
			if (MassAgentHandle.IsValid())
			{
				// An entity might be removed from Mass (because the vehicle was damaged and LODs out), hence
				// the thorough IsEntityValid() check.
				UMassEntitySubsystem* EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(World);
				if (EntitySubsystem && EntitySubsystem->GetMutableEntityManager().IsEntityValid(MassAgentHandle))
				{
					EntitySubsystem->GetMutableEntityManager().RemoveTagFromEntity(MassAgentHandle, FMassTrafficPlayerVehicleTag::StaticStruct());
				}
			}
		}

		if (UMassActorSubsystem* MassActorSubsystem = UWorld::GetSubsystem<UMassActorSubsystem>(World))
		{
			const FMassEntityHandle MassAgentHandle = MassActorSubsystem->GetEntityHandleFromActor(this);
			if (MassAgentHandle.IsValid())
			{
				UE::MassActor::AddEntityTagToActor<FMassTrafficPlayerVehicleTag>(*this);
			}
			else
			{
				UE_LOG(LogCitySample, Verbose, TEXT("MassAgentHandle not valid for vehicle. Mass AI spawner may not be setup for current level. Vehicle = %s"), *GetName());
			}
		}

		CitySamplePlayerController->SetCurrentPlayerVehicle(this);
	}
}

void ACitySampleVehicleBase::UnPossessed()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, SpeedRumbleHandle);

		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, CorneringRumbleHandle);

		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, LeftSurfaceRumbleHandle);
		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, RightSurfaceRumbleHandle);

		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, LeftSuspensionRumbleHandle);
		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, RightSuspensionRumbleHandle);

		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, LeftBumpRumbleHandle);
		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, RightBumpRumbleHandle);

		PlayerController->PlayDynamicForceFeedback(0.0f, -1.0f, false, false, false, false, EDynamicForceFeedbackAction::Stop, CollisionRumbleHandle);

		OnIgnitionStart.RemoveDynamic(this, &ACitySampleVehicleBase::StartParkingReleaseTimer);
		
		if (ACitySampleGameState* const CitySampleGameState = UCitySampleBlueprintLibrary::GetCitySampleGameState(GetWorld()))
		{
			CitySampleGameState->OnEnterPhotomode.RemoveDynamic(this, &ThisClass::OnEnterPhotomode);
			CitySampleGameState->OnExitPhotomode.RemoveDynamic(this, &ThisClass::OnExitPhotomode);
		}

		if (ACitySamplePlayerController* const CitySamplePlayerController = Cast<ACitySamplePlayerController>(PlayerController))
		{		
			if (UCitySampleUIComponent* const UIComponent = CitySamplePlayerController->GetCitySampleUIComponent())
			{
				UIComponent->OnOptionsMenuOpen.RemoveDynamic(this, &ThisClass::OnOptionsMenuOpened);
			}
		}
	}

	Super::UnPossessed();

	bIsPossessedByPlayer = false;
}

void ACitySampleVehicleBase::SetVehicleInputs_Implementation(const float Throttle, const float Brake, const bool bHandBrake, const float Steering, const bool bSetSteering)
{
	SetThrottleInput(Throttle);
	SetBrakeInput(Brake);
	SetHandbrakeInput(bHandBrake);

	if (bSetSteering)
	{
		SetSteeringInput(Steering);
	}
}

void ACitySampleVehicleBase::OnParkedVehicleSpawned_Implementation()
{
	bIsPossessableByPlayer = true;
}

void ACitySampleVehicleBase::OnTrafficVehicleSpawned_Implementation()
{
	bIsPossessableByPlayer = false;
}


void ACitySampleVehicleBase::PrepareForPooling_Implementation()
{
	// We're going to be recycled, get rid of our controller
	DetachFromControllerPendingDestroy();

	if (UChaosVehicleMovementComponent* MoveComp = GetVehicleMovement())
	{
		MoveComp->ResetVehicle();
		MoveComp->StopMovementImmediately();
	}

	CleanUpUnpossessedDriver();
}

bool ACitySampleVehicleBase::ShouldDrawRumbleDebugInfo() const
{
	return bDrawRumbleInfoDebug;
}

bool ACitySampleVehicleBase::ShouldDrawCenterOfMassDebug() const
{
	return bDrawCenterOfMassDebug;
}

void ACitySampleVehicleBase::CleanUpUnpossessedDriver()
{
	// If we have a zombie pawn still driving because we're in photo mode, destroy it
	for (int i = 0; i < (uint8)ECitySampleVehicleSeat::MAX; i++)
	{
		if (Occupants[i] && Occupants[i]->Controller == nullptr)
		{
			Occupants[i]->Destroy();
			Occupants[i] = nullptr;
		}
	}
}

void ACitySampleVehicleBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CleanUpUnpossessedDriver();
}

void ACitySampleVehicleBase::PrepareForGame_Implementation()
{
	SetActorEnableCollision(true);

	// Reset any wheel motion blur
	for (int i = 0; i < CachedMotionBlurWheelMIDs.Num(); i++)
	{
		if (CachedMotionBlurWheelMIDs[i] && CachedMotionBlurWheelAngle.IsValidIndex(i) && CachedMotionBlurWheelAngle[i] != 0.0f)
		{
			static FName NAME_Angle(TEXT("Angle"));
			CachedMotionBlurWheelMIDs[i]->SetScalarParameterValue(NAME_Angle, 0.0f);
		}
	}
	CachedMotionBlurWheelMIDs.Empty();
	CachedMotionBlurWheelAngle.Empty();

	ResetMotionBlurForAllComponentsThisFrame();
}

void ACitySampleVehicleBase::ResetMotionBlurForAllComponentsThisFrame()
{
	if (RootComponent)
	{
		TArray<USceneComponent*> ChildComps;
		RootComponent->GetChildrenComponents(true, ChildComps);
		ChildComps.Add(RootComponent);

		for (int i = 0; i < ChildComps.Num(); i++)
		{
			FMotionVectorSimulation::Get().SetPreviousTransform(ChildComps[i], ChildComps[i]->GetComponentToWorld());
		}
	}
}

void ACitySampleVehicleBase::TriggerLeftSuspensionRumble()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController != nullptr)
	{
		LeftSuspensionRumbleHandle = PlayerController->PlayDynamicForceFeedback(LeftSuspensionRumbleInfo.Intensity, LeftSuspensionRumbleInfo.Duration, LeftSuspensionRumbleInfo.bUseLeftLarge, LeftSuspensionRumbleInfo.bUseLeftSmall, LeftSuspensionRumbleInfo.bUseRightLarge, LeftSuspensionRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Start, LeftSuspensionRumbleHandle);
	}
}

void ACitySampleVehicleBase::TriggerRightSuspensionRumble()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController != nullptr)
	{
		RightSuspensionRumbleHandle = PlayerController->PlayDynamicForceFeedback(RightSuspensionRumbleInfo.Intensity, RightSuspensionRumbleInfo.Duration, RightSuspensionRumbleInfo.bUseLeftLarge, RightSuspensionRumbleInfo.bUseLeftSmall, RightSuspensionRumbleInfo.bUseRightLarge, RightSuspensionRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Start, RightSuspensionRumbleHandle);
	}
}

void ACitySampleVehicleBase::TriggerLeftBumpRumble()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController != nullptr)
	{
		LeftBumpRumbleHandle = PlayerController->PlayDynamicForceFeedback(LeftBumpRumbleInfo.Intensity, LeftBumpRumbleInfo.Duration, LeftBumpRumbleInfo.bUseLeftLarge, LeftBumpRumbleInfo.bUseLeftSmall, LeftBumpRumbleInfo.bUseRightLarge, LeftBumpRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Start, LeftBumpRumbleHandle);
	}
}

void ACitySampleVehicleBase::TriggerRightBumpRumble()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController != nullptr)
	{
		RightBumpRumbleHandle = PlayerController->PlayDynamicForceFeedback(RightBumpRumbleInfo.Intensity, RightBumpRumbleInfo.Duration, RightBumpRumbleInfo.bUseLeftLarge, RightBumpRumbleInfo.bUseLeftSmall, RightBumpRumbleInfo.bUseRightLarge, RightBumpRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Start, RightBumpRumbleHandle);
	}
}

void ACitySampleVehicleBase::TriggerCollisionRumble()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController != nullptr)
	{
		CollisionRumbleHandle = PlayerController->PlayDynamicForceFeedback(CollisionRumbleInfo.Intensity, CollisionRumbleInfo.Duration, CollisionRumbleInfo.bUseLeftLarge, CollisionRumbleInfo.bUseLeftSmall, CollisionRumbleInfo.bUseRightLarge, CollisionRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Start, CollisionRumbleHandle);
	}
}

EPhysicalSurface ACitySampleVehicleBase::GetPhysicalSurfaceUnderWheel(int32 WheelIndex) const
{
	if (UChaosWheeledVehicleMovementComponent* const WVMC = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement()))
	{
		if (WheelIndex < WVMC->GetNumWheels())
		{
			const FWheelStatus& WheelState = WVMC->GetWheelState(WheelIndex);
			if (WheelState.PhysMaterial.IsValid())
			{
				return WheelState.PhysMaterial->SurfaceType;
			}
		}
	}

	return EPhysicalSurface::SurfaceType_Default;
}

void ACitySampleVehicleBase::SetThrottleInput(float Throttle)
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{
		const float PrevThrottle = DrivingState.Throttle;

		Throttle = bThrottleDisabled ? 0.f : Throttle;
		VMC->SetThrottleInput(Throttle);
		DrivingState.Throttle = Throttle;

		if (FMath::IsNearlyZero(PrevThrottle) && !FMath::IsNearlyZero(Throttle))
		{
			OnAccelerationStart.Broadcast();
		}
		else if (!FMath::IsNearlyZero(PrevThrottle) && FMath::IsNearlyZero(Throttle))
		{
			OnAccelerationStop.Broadcast();
		}
	}
}

void ACitySampleVehicleBase::SetBrakeInput(float Brake)
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{
		const bool bIsThrottleDisableApplicable = bThrottleDisabled && (DrivingState.ForwardSpeed <= 0 || FMath::IsNearlyZero(DrivingState.ForwardSpeed, 1.0f));

		Brake = bIsThrottleDisableApplicable ? 0.f : Brake;
		VMC->SetBrakeInput(Brake);
		DrivingState.Brake = Brake;
	}
}

void ACitySampleVehicleBase::SetSteeringInput(float Steering)
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{
		Steering = Steering + SteeringModifier;
		VMC->SetSteeringInput(Steering);
		DrivingState.Steering = Steering;
	}
}

void ACitySampleVehicleBase::SetHandbrakeInput(bool bHandbrakeActive)
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{
		if (bHandbrakeActive)
		{
			VMC->SetHandbrakeInput(true);
			DrivingState.bHandbrakeOn = true;
			OnHandbrakeStart.Broadcast();
		}
		else
		{
			VMC->SetHandbrakeInput(false);
			DrivingState.bHandbrakeOn = false;
			OnHandbrakeStop.Broadcast();
		}
	}
}

FVector2D ACitySampleVehicleBase::GetCOMPositionRatio() const
{
	// Calculates Vehicle Bounding box assuming actor has a default rotation.
	FTransform TempTransform = GetTransform();
	TempTransform.SetRotation(FQuat::Identity);
	const UPhysicsAsset* PhysicsAsset = GetMesh()->GetPhysicsAsset();
	const FBox LocalBounds = PhysicsAsset != nullptr ? PhysicsAsset->CalcAABB(GetMesh(), TempTransform) : FBox();

	// Get Vehicle Center of Mass position in local space
	const FVector LocalCOMPosition = GetTransform().InverseTransformPosition(GetMesh()->GetCenterOfMass());

	return FVector2D(LocalCOMPosition.X / LocalBounds.GetExtent().X, LocalCOMPosition.Y / LocalBounds.GetExtent().Y);
}

bool ACitySampleVehicleBase::IsVehicleDrifting() const
{
	return bIsDrifting;
}

float ACitySampleVehicleBase::GetWheelSlipAngleInDegrees(int WheelIndex) const
{
	if (const UChaosWheeledVehicleMovementComponent* MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{		
		if (WheelIndex < MovementComponent->GetNumWheels())
		{
			return FMath::RadiansToDegrees(MovementComponent->GetWheelState(WheelIndex).SlipAngle);
		}
	}

	return 0.0f;
}

float ACitySampleVehicleBase::GetNormalizedSuspensionLength(int WheelIndex) const
{
	if (const UChaosWheeledVehicleMovementComponent* MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		if (WheelIndex < MovementComponent->GetNumWheels())
		{
			return MovementComponent->GetWheelState(WheelIndex).NormalizedSuspensionLength;
		}	
	}

	return 0.0f;
}

void ACitySampleVehicleBase::SetSteeringModifier(float NewModifier)
{
	const float PreviousRawSteering = DrivingState.Steering - SteeringModifier;
	SteeringModifier = NewModifier;
	SetSteeringInput(PreviousRawSteering);
}

void ACitySampleVehicleBase::OnExitVehicle()
{
	if (bIsPossessedByPlayer)
	{
		GetWorldTimerManager().ClearTimer(ParkingReleaseTimerHandle);
		OnPlayerExitVehicle.Broadcast();
	}
}

void ACitySampleVehicleBase::SetParked(bool bIsParked)
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{
		VMC->SetParked(bIsParked);
	}
}

void ACitySampleVehicleBase::OnOptionsMenuOpened(UCitySampleUIComponent* const CitySampleUI, UCitySampleMenu* const OptionsMenu)
{
	// When options menu is opened, we want to zero out our inputs to have a clean slate to return to and to avoid the car driving off on its own while the player lacks control of it
	ResetInputs();
}

void ACitySampleVehicleBase::ResetInputs()
{
	SetSteeringInput(0.0f);
	SetThrottleInput(0.0f);
	SetBrakeInput(0.0f);
	SetHandbrakeInput(false);
}

void ACitySampleVehicleBase::Look(const FVector2D& Value)
{
	TurnAtRate(Value.X);
	LookUpAtRate(Value.Y);
}

void ACitySampleVehicleBase::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ACitySampleVehicleBase::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ACitySampleVehicleBase::UpdateDrivingState()
{
	if (UChaosWheeledVehicleMovementComponent* VMC = StaticCast<UChaosWheeledVehicleMovementComponent *>(GetVehicleMovement()))
	{
		DrivingState.bAutomatic = VMC->GetUseAutoGears();
		DrivingState.ForwardSpeed = VMC->GetForwardSpeed();
		DrivingState.RPM = VMC->GetEngineRotationSpeed();
		DrivingState.Gear = VMC->GetCurrentGear();

		const bool bIsParked = VMC->IsParked();

		//Handle Brake events
		if (bIsParked)
		{
			OnLiteralBrakeStop();
		}
		else if (!FMath::IsNearlyZero(DrivingState.ForwardSpeed, 1.0f))
		{
			if (DrivingState.ForwardSpeed > 0)
			{
				if (!FMath::IsNearlyZero(DrivingState.Brake))
				{
					OnLiteralBrakeStart();
				}
				else
				{
					OnLiteralBrakeStop();
				}
			}
			else
			{
				if (!FMath::IsNearlyZero(DrivingState.Throttle))
				{
					OnLiteralBrakeStart();
				}
				else
				{
					OnLiteralBrakeStop();
				}
			}
		}
		else
		{
			if (!FMath::IsNearlyZero(DrivingState.Brake) && !FMath::IsNearlyZero(DrivingState.Throttle))
			{
				OnLiteralBrakeStart();
			}
			else
			{
				OnLiteralBrakeStop();
			}
		}

		//Handle Reverse events
		if (bIsParked || FMath::IsNearlyZero(DrivingState.ForwardSpeed, 1.0f) || DrivingState.ForwardSpeed > 0)
		{
			OnLiteralReverseStop();			
		}
		else if (DrivingState.ForwardSpeed < 0 && !FMath::IsNearlyZero(DrivingState.Brake) && FMath::IsNearlyZero(DrivingState.Throttle))
		{
			OnLiteralReverseStart();
		}

		//Handle Vehicle Airstate flag and events
		DrivingState.bAllWheelsOnGround = CheckAllWheelsOnGround();

		const bool bPrevNoWheelsOnGround = DrivingState.bNoWheelsOnGround;
		DrivingState.bNoWheelsOnGround = CheckNoWheelsOnGround();

		if (bIsOkayToFireAirborneEvents && bPrevNoWheelsOnGround != DrivingState.bNoWheelsOnGround)
		{
			if (DrivingState.bNoWheelsOnGround)
			{
				OnAirborneStart.Broadcast();
			}
			else
			{
				OnAirborneStop.Broadcast();
			}
		}

		// Don't start firing airborne events until our vehicle has completely touched the ground once. This stops airborne events from firing on spawn.
		if (DrivingState.bAllWheelsOnGround)
		{
			bIsOkayToFireAirborneEvents = true;
		}
	}
}


void ACitySampleVehicleBase::InitializeAngularDampingDefault()
{
	DefaultAngularDamping = GetMesh()->GetAngularDamping();
}

void ACitySampleVehicleBase::UpdateAngularDamping()
{
	if (bAdjustAngularDampingToSteering)
	{
		if (DrivingState.bNoWheelsOnGround)
		{
			GetMesh()->SetAngularDamping(DefaultAngularDamping);
		}
		else
		{
			float FinalAngularDamping = 0.0f;
			if (SteeringToAngularDamping != nullptr)
			{
				FinalAngularDamping = SteeringToAngularDamping->GetFloatValue(FMath::Abs(DrivingState.Steering)) * AngularDampingScale;

			}
			else
			{
				FinalAngularDamping = FMath::Square((1.0f - FMath::Abs(DrivingState.Steering))) * AngularDampingScale;
			}

			GetMesh()->SetAngularDamping(FinalAngularDamping);
		}
	}
}

void ACitySampleVehicleBase::UpdateLODBlend(float DeltaTime)
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{	
		FVector LinearVelocity2D = GetMesh()->GetPhysicsLinearVelocity();
		LinearVelocity2D.Z = 0.0f; // 2D velocity

		if (FBodyInstance* TargetInstance = VMC->UpdatedPrimitive ? VMC->UpdatedPrimitive->GetBodyInstance() : nullptr)
		{
			bool NowAwake = TargetInstance->IsInstanceAwake();
			if (NowAwake != PreviouslyAwake)
			{
				if (NowAwake)
				{
					OnSleepStop.Broadcast();

					// Restart LOD Blend if vehicle not moving fast
					if (LinearVelocity2D.SizeSquared() < (BlendTransitionVelocity * BlendTransitionVelocity))
					{
						BlendTimer = 0;
					}

				}
				else
				{
					OnSleepStart.Broadcast();
				}

				PreviouslyAwake = NowAwake;
			}
		}

		const float K = -15.0f;
		if (BlendTimer < MaxBlendTransitionTime && MaxBlendTransitionTime > SMALL_NUMBER)
		{
			BlendTimer += DeltaTime;

			FVector LinearVelocity = GetMesh()->GetPhysicsLinearVelocity();

			float UpVelocity = GetMesh()->GetUpVector().Dot(LinearVelocity);
			float Acceleration = -0.5f * UpVelocity / DeltaTime;

			FBodyInstance* TargetInstance = VMC->UpdatedPrimitive ? VMC->UpdatedPrimitive->GetBodyInstance() : nullptr;
			if (TargetInstance && TargetInstance->IsInstanceAwake() && (LinearVelocity2D.SizeSquared() < (BlendTransitionVelocity * BlendTransitionVelocity)))
			{
				TargetInstance->AddForce(Acceleration * GetMesh()->GetUpVector(), true, true);

				if (bShowLODDamping)
				{
					DrawDebugSphere(GetWorld(), TargetInstance->GetUnrealWorldTransform().GetLocation(), 400, 16, FColor::White);
				}
			}
		}
	}
}

void ACitySampleVehicleBase::UpdateDebugDraw()
{
	if (UChaosVehicleMovementComponent* const VMC = GetVehicleMovement())
	{
		if (FBodyInstance* TargetInstance = VMC->UpdatedPrimitive ? VMC->UpdatedPrimitive->GetBodyInstance() : nullptr)
		{
			if (bShowSleepState)
			{
				// draw whether active or sleeping
				if (TargetInstance->IsInstanceAwake())
				{
					DrawDebugSphere(GetWorld(), TargetInstance->GetUnrealWorldTransform().GetLocation(), 250, 16, FColor::Green);
				}
				else
				{
					DrawDebugSphere(GetWorld(), TargetInstance->GetUnrealWorldTransform().GetLocation(), 250, 16, FColor::Red);
				}
			}
		}

	}
}

void ACitySampleVehicleBase::UpdateControllerRumble()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController != nullptr)
	{
		PlayerController->PlayDynamicForceFeedback(SpeedRumbleInfo.Intensity, SpeedRumbleInfo.Duration, SpeedRumbleInfo.bUseLeftLarge, SpeedRumbleInfo.bUseLeftSmall, SpeedRumbleInfo.bUseRightLarge, SpeedRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Update, SpeedRumbleHandle);
		PlayerController->PlayDynamicForceFeedback(CorneringRumbleInfo.Intensity, CorneringRumbleInfo.Duration, CorneringRumbleInfo.bUseLeftLarge, CorneringRumbleInfo.bUseLeftSmall, CorneringRumbleInfo.bUseRightLarge, CorneringRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Update, CorneringRumbleHandle);
		PlayerController->PlayDynamicForceFeedback(LeftSurfaceRumbleInfo.Intensity, LeftSurfaceRumbleInfo.Duration, LeftSurfaceRumbleInfo.bUseLeftLarge, LeftSurfaceRumbleInfo.bUseLeftSmall, LeftSurfaceRumbleInfo.bUseRightLarge, LeftSurfaceRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Update, LeftSurfaceRumbleHandle);
		PlayerController->PlayDynamicForceFeedback(RightSurfaceRumbleInfo.Intensity, RightSurfaceRumbleInfo.Duration, RightSurfaceRumbleInfo.bUseLeftLarge, RightSurfaceRumbleInfo.bUseLeftSmall, RightSurfaceRumbleInfo.bUseRightLarge, RightSurfaceRumbleInfo.bUseLeftSmall, EDynamicForceFeedbackAction::Update, RightSurfaceRumbleHandle);
	}
}

void ACitySampleVehicleBase::UpdateWheelSuspensionStates(float DeltaTime)
{
	for (int i = 0; i < WheelSuspensionStates.Num(); ++i)
	{
		FCitySampleWheelSuspensionState& SuspensionState = WheelSuspensionStates[i];
		const float NormalizedSuspensionLength = GetNormalizedSuspensionLength(i);

		// Fire a BIE to react to a bump 
		const float SuspensionDiff = SuspensionState.PreviousNormalizedSuspensionLength - NormalizedSuspensionLength;
		if (SuspensionDiff > NormalizedSuspensionDiffThreshold)
		{
			const float BumpStrength = SuspensionDiff - NormalizedSuspensionDiffThreshold;

			OnWheelBump.Broadcast(i, BumpStrength);

			i % 2 == 0 ? UpdateLeftBumpRumbleInfo(BumpStrength) : UpdateRightBumpRumbleInfo(BumpStrength);
		}

		// Keep track of how long we're at full suspension length
		if (NormalizedSuspensionLength == 1.0f)
		{
			SuspensionState.TimeSpentAtFullLength += DeltaTime;
		}
		// On first frame of leaving full suspension length, fire a BIE so we can react to it accordingly
		else if (SuspensionState.PreviousNormalizedSuspensionLength == 1.0f)
		{
			// Even tires are on the right, odds on the left
			i % 2 == 0 ? UpdateLeftSuspensionRumbleInfo(SuspensionState.TimeSpentAtFullLength) : UpdateRightSuspensionRumbleInfo(SuspensionState.TimeSpentAtFullLength);

			SuspensionState.TimeSpentAtFullLength = 0.0f;
		}

		SuspensionState.PreviousNormalizedSuspensionLength = NormalizedSuspensionLength;
	}
}

void ACitySampleVehicleBase::InitializeCenterOfMassOffsets()
{
	if (bUseCenterOfMassAdjustments)
	{
		float CurrentForwardCOMOffset = 0;
		if (SteeringToForwardCOMOffset != nullptr)
		{
			CurrentForwardCOMOffset = SteeringToForwardCOMOffset->GetFloatValue(FMath::Abs(DrivingState.Steering)) * ForwardCOMOffsetScale;
			ForwardCOMOffsetInterpolator.SetInitialValue(CurrentForwardCOMOffset);
		}

		float CurrentHorizontalCOMOffset = 0;
		if (SteeringToHorizontalCOMOffset != nullptr)
		{
			const float HorizontalCOMOffsetDirection = DrivingState.Steering >= 0 ? 1.0f : -1.0f;
			CurrentHorizontalCOMOffset = SteeringToHorizontalCOMOffset->GetFloatValue(FMath::Abs(DrivingState.Steering)) * HorizontalCOMOffsetScale * HorizontalCOMOffsetDirection;
			HorizontalCOMOffsetInterpolator.SetInitialValue(CurrentHorizontalCOMOffset);
		}

		float CurrentVerticalCOMOffset = 0;
		if (SteeringToVerticalCOMOffset != nullptr)
		{
			CurrentVerticalCOMOffset = SteeringToVerticalCOMOffset->GetFloatValue(FMath::Abs(DrivingState.Steering)) * VerticalCOMOffsetScale;
			VerticalCOMOffsetInterpolator.SetInitialValue(CurrentVerticalCOMOffset);
		}

		const FVector NewCOMOffset = FVector(DefaultCenterOfMassOffset.X + CurrentForwardCOMOffset, DefaultCenterOfMassOffset.Y + CurrentHorizontalCOMOffset, DefaultCenterOfMassOffset.Z + CurrentVerticalCOMOffset);
		GetMesh()->SetCenterOfMass(NewCOMOffset);
	}
}

void ACitySampleVehicleBase::UpdateCenterOfMassOffsets(float DeltaTime)
{
	if (bUseCenterOfMassAdjustments)
	{
		// Update Center of Mass Steering Offset Interp Speeds depending on current Steering value driving them
		if (DrivingState.Steering != 0.0f)
		{
			ForwardCOMOffsetInterpolator.InterpSpeed = ForwardCOMInterpRiseAndFallSpeeds.X;
			HorizontalCOMOffsetInterpolator.InterpSpeed = HorizontalCOMInterpRiseAndFallSpeeds.X;
			VerticalCOMOffsetInterpolator.InterpSpeed = VerticalCOMInterpRiseAndFallSpeeds.X;
		}
		else
		{
			ForwardCOMOffsetInterpolator.InterpSpeed = ForwardCOMInterpRiseAndFallSpeeds.Y;
			HorizontalCOMOffsetInterpolator.InterpSpeed = HorizontalCOMInterpRiseAndFallSpeeds.Y;
			VerticalCOMOffsetInterpolator.InterpSpeed = VerticalCOMInterpRiseAndFallSpeeds.Y;
		}

		const float HandBrakeInterpGoal = DrivingState.bHandbrakeOn ? 1.0f : 0.0f;
		const bool DisableForwardCOMOffsets = DrivingState.ForwardSpeed <= MinVelocityToDisableForwardCOMOffset;

		// Calculate Forward Center of Mass Offset
		float CurrentSteeringForwardCOMOffset = 0.0f;
		if (SteeringToForwardCOMOffset != nullptr)
		{
			const float SteeringGoalForwardCOMOffset = DisableForwardCOMOffsets ? 0.0f : (SteeringToForwardCOMOffset->GetFloatValue(FMath::Abs(DrivingState.Steering)) * ForwardCOMOffsetScale);
			CurrentSteeringForwardCOMOffset = ForwardCOMOffsetInterpolator.Eval(SteeringGoalForwardCOMOffset, DeltaTime);
		}

		float CurrentHandbrakeForwardCOMOffset = 0.0f;
		if (HandbrakeForwardCOMOffset != nullptr && !DisableForwardCOMOffsets)
		{
			const float InterpSpeed = DrivingState.bHandbrakeOn ? HandbrakeForwardCOMInterpRiseAndFallSpeeds.X : HandbrakeForwardCOMInterpRiseAndFallSpeeds.Y;
			ForwardHandbrakeCurveInput = FMath::FInterpConstantTo(ForwardHandbrakeCurveInput, HandBrakeInterpGoal, DeltaTime, InterpSpeed);
			CurrentHandbrakeForwardCOMOffset = HandbrakeForwardCOMOffset->GetFloatValue(ForwardHandbrakeCurveInput) * HandbrakeForwardCOMOffsetScale;
		}

		const float CurrentForwardCOMOffset = CurrentSteeringForwardCOMOffset + CurrentHandbrakeForwardCOMOffset;

		// Calculate Horizontal Center of Mass Offset
		float CurrentHorizontalCOMOffset = 0.0f;
		if (SteeringToHorizontalCOMOffset != nullptr)
		{
			const float HorizontalCOMOffsetDirection = DrivingState.Steering >= 0 ? 1.0f : -1.0f;
			const float GoalHorizontalCOMOffset = SteeringToHorizontalCOMOffset->GetFloatValue(FMath::Abs(DrivingState.Steering)) * HorizontalCOMOffsetScale * HorizontalCOMOffsetDirection;
			CurrentHorizontalCOMOffset = HorizontalCOMOffsetInterpolator.Eval(GoalHorizontalCOMOffset, DeltaTime);
		}

		// Calculate Vertical Center of Mass Offset
		float CurrentSteeringVerticalCOMOffset = 0.0f;
		if (SteeringToVerticalCOMOffset != nullptr)
		{
			const float SteeringGoalVerticalCOMOffset = (SteeringToVerticalCOMOffset->GetFloatValue(FMath::Abs(DrivingState.Steering)) * VerticalCOMOffsetScale);
			CurrentSteeringVerticalCOMOffset = VerticalCOMOffsetInterpolator.Eval(SteeringGoalVerticalCOMOffset, DeltaTime);
		}

		float CurrentHandbrakeVerticalCOMOffset = 0.0f;
		if (HandbrakeVerticalCOMOffset != nullptr)
		{
			const float InterpSpeed = DrivingState.bHandbrakeOn ? HandbrakeVerticalCOMInterpRiseAndFallSpeeds.X : HandbrakeVerticalCOMInterpRiseAndFallSpeeds.Y;
			VerticalHandbrakeCurveInput = FMath::FInterpConstantTo(VerticalHandbrakeCurveInput, HandBrakeInterpGoal, DeltaTime, InterpSpeed);
			CurrentHandbrakeVerticalCOMOffset = HandbrakeVerticalCOMOffset->GetFloatValue(VerticalHandbrakeCurveInput) * HandbrakeVerticalCOMOffsetScale;
		}

		const float CurrentVerticalCOMOffset = CurrentSteeringVerticalCOMOffset + CurrentHandbrakeVerticalCOMOffset;

		const FVector NewCOMOffset = FVector(DefaultCenterOfMassOffset.X + CurrentForwardCOMOffset, DefaultCenterOfMassOffset.Y + CurrentHorizontalCOMOffset, DefaultCenterOfMassOffset.Z + CurrentVerticalCOMOffset);
		GetMesh()->SetCenterOfMass(NewCOMOffset);

#if ENABLE_DRAW_DEBUG
		if (bDrawCenterOfMassDebug && bIsPossessedByPlayer)
		{
			const FBodyInstance* BodyInstance = GetMesh()->GetBodyInstance();
			const FVector CurrentCOMOffset = BodyInstance ? GetMesh()->GetBodyInstance()->COMNudge : FVector();

			::DrawDebugSphere(GetWorld(), GetMesh()->GetCenterOfMass(), 16.0f, 16.0f, FColor::Purple);
			if (GEngine != nullptr)
			{
				const FString DisplayString = FString::Printf(TEXT("COM Offset - X: %f | Y: %f | Z: %f"), CurrentCOMOffset.X, CurrentCOMOffset.Y, CurrentCOMOffset.Z);
				GEngine->AddOnScreenDebugMessage(1, 1.0f, FColor::Purple, *DisplayString);
			}
		}
#endif
	}
}

void ACitySampleVehicleBase::UpdateWheelFrictionMultiplierAdjustments()
{
	if (bUseWheelFrictionMultiplierAdjustments)
	{
		if (UChaosWheeledVehicleMovementComponent* const VMC = StaticCast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovement()))
		{
			//This logic assumes that the front wheels correspond to wheel indices 0 and 1
			if (SteeringToFrontWheelFrictionMultiplierAdjustment != nullptr && VMC->WheelSetups.Num() >= 2)
			{
				const float WheelFrictionAdjustment = SteeringToFrontWheelFrictionMultiplierAdjustment->GetFloatValue(DrivingState.Steering) * FrontWheelFrictionMultiplierAdjustmentScale;

				//This function only modifies the front wheel friction, so it shouldn't conflict with ACitySampleVehicleBase::UpdateBurnout. However if more friction adjustments are added then UpdateBurnout will need to be taken into account 
				for (int32 WheelIdx = 0; WheelIdx < 2; ++WheelIdx)
				{
					const FChaosWheelSetup& WheelSetup = VMC->WheelSetups[WheelIdx];
					const UChaosVehicleWheel* Wheel = WheelSetup.WheelClass.GetDefaultObject();
					VMC->SetWheelFrictionMultiplier(WheelIdx, Wheel->FrictionForceMultiplier + WheelFrictionAdjustment);
				}
			}
			else
			{
				UE_LOG(LogCitySample, Warning, TEXT("Attempted to use WheelFrictionMultiplierAdjustments on a vehicle that doesn't have the proper wheels set up or with a missing WheelFrictionMultiplierCurve."))
			}
		}
	}
}

void ACitySampleVehicleBase::UpdateBurnout(float DeltaTime)
{
	if (bUseBurnoutAdjustments)
	{
		if (UChaosWheeledVehicleMovementComponent* const VMC = StaticCast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovement()))
		{
			float ForwardsSpeed = VMC->GetForwardSpeedMPH();

			// rules for when burnout is activated
			if (!bBurnoutActivated)
			{
				if (ForwardsSpeed < BurnoutMinSpeedThreshold && VMC->GetThrottleInput() > 0.1f)
				{
					bBurnoutActivated = true;
				}
			}

			// rules for when burnout continues
			if (bBurnoutActivated && (ForwardsSpeed < BurnoutMaxSpeedThreshold) && (VMC->GetCurrentGear() == 1) && VMC->GetThrottleInput() > 0.1f)
			{
				if (auto& PhysicsVehicleOutput = VMC->PhysicsVehicleOutput())
				{					
					for (int32 WheelIdx = 2; WheelIdx < VMC->WheelSetups.Num(); ++WheelIdx)
					{
						const FChaosWheelSetup& WheelSetup = VMC->WheelSetups[WheelIdx];
						const UChaosVehicleWheel* Wheel = WheelSetup.WheelClass.GetDefaultObject();
						VMC->SetUseAutomaticGears(false);					// stop the gears changing up while in middle of burnout behavior
						VMC->SetTractionControlEnabled(WheelIdx, false);	// disable traction control if it was enabled so we can skid the wheels
						VMC->SetWheelFrictionMultiplier(WheelIdx, BurnoutFrictionMultiplier * Wheel->FrictionForceMultiplier);
						VMC->SetWheelSlipGraphMultiplier(WheelIdx, BurnoutLateralSlipMultiplier);
					}

				}
			}
			else
			{
				// that's the end of that
				bBurnoutActivated = false;

				// put original setup values back
				for (int32 WheelIdx = 2; WheelIdx < VMC->WheelSetups.Num(); ++WheelIdx)
				{
					const FChaosWheelSetup& WheelSetup = VMC->WheelSetups[WheelIdx];
					const UChaosVehicleWheel* Wheel = WheelSetup.WheelClass.GetDefaultObject();
					VMC->SetUseAutomaticGears(VMC->TransmissionSetup.bUseAutomaticGears);
					VMC->SetTractionControlEnabled(WheelIdx, Wheel->bTractionControlEnabled);
					VMC->SetWheelFrictionMultiplier(WheelIdx, Wheel->FrictionForceMultiplier);
					VMC->SetWheelSlipGraphMultiplier(WheelIdx, 1.0f);			
				}
			}
		}
	}

}

void ACitySampleVehicleBase::UpdateVehicleDriftingState()
{
	if (const UChaosWheeledVehicleMovementComponent* const MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		const bool bPrevIsDrifting = bIsDrifting;

		//Vehicle must surpass a speed threshold and have one of its rear wheels meet a slip angle threshold (angle between velocity and wheel forward) before it is considered to be drifting
		bIsDrifting =  ((GetWheelSlipAngleInDegrees(2) >= RearWheelSlipAngleDriftingMinThreshold) || (GetWheelSlipAngleInDegrees(3) >= RearWheelSlipAngleDriftingMinThreshold))
			&& (FMath::Abs(DrivingState.ForwardSpeed) >= DriftingSpeedMinThreshold);

		if (bIsDrifting != bPrevIsDrifting)
		{
			if (bIsDrifting)
			{
				OnDriftStart.Broadcast();
			}
			else
			{
				OnDriftStop.Broadcast();
			}
		}
	}
}

bool ACitySampleVehicleBase::CheckAllWheelsOnGround() const
{
	check(GetVehicleMovement());

	if (UChaosWheeledVehicleMovementComponent* const VMC = StaticCast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovement()))
	{
		for (int WheelIdx = 0; WheelIdx < VMC->GetNumWheels(); WheelIdx++)
		{
			if (!VMC->GetWheelState(WheelIdx).bInContact)
			{
				return false;
			}
		}
	}
	
	return true;
}

bool ACitySampleVehicleBase::CheckNoWheelsOnGround() const
{
	check(GetVehicleMovement());

	if (UChaosWheeledVehicleMovementComponent* const VMC = StaticCast<UChaosWheeledVehicleMovementComponent*>(GetVehicleMovement()))
	{
		for (int WheelIdx = 0; WheelIdx < VMC->GetNumWheels(); WheelIdx++)
		{
			if (VMC->GetWheelState(WheelIdx).bInContact)
			{
				return false;
			}
		}
	}

	return true;
}

void ACitySampleVehicleBase::OnLiteralBrakeStart()
{
	if (!bIsLiterallyBreaking)
	{
		bIsLiterallyBreaking = true;
		OnBrakeStart.Broadcast();
	}
}

void ACitySampleVehicleBase::OnLiteralBrakeStop()
{
	if (bIsLiterallyBreaking)
	{
		bIsLiterallyBreaking = false;
		OnBrakeStop.Broadcast();
	}
}

void ACitySampleVehicleBase::OnLiteralReverseStart()
{
	if (!bIsReversing)
	{
		bIsReversing = true;
		OnReverseStart.Broadcast();
	}
}

void ACitySampleVehicleBase::OnLiteralReverseStop()
{
	if (bIsReversing)
	{
		bIsReversing = false;
		OnReverseStop.Broadcast();
	}
}

void ACitySampleVehicleBase::OnEnterPhotomode()
{
	SetParked(true);
}

void ACitySampleVehicleBase::OnExitPhotomode()
{
	SetParked(false);
}

void ACitySampleVehicleBase::StartParkingReleaseTimer()
{
	GetWorldTimerManager().SetTimer(ParkingReleaseTimerHandle, ParkingReleaseTimerDelegate, PlayerDrivingDelay, false);
}

bool ACitySampleVehicleBase::IsVehicleFlippedOver(const float RollThreshold) const
{
	const FRotator ActorRot = GetActorTransform().GetRotation().Rotator();
	return FMath::Abs<>(ActorRot.Roll) > RollThreshold;
}

void ACitySampleVehicleBase::FlipVehicle()
{
	FVector NewLocation = GetActorLocation();
	FRotator NewRotation = FRotator::ZeroRotator;
	NewRotation.Yaw = GetActorRotation().Yaw;

	bool bFoundTeleportSpot = TeleportTo(NewLocation, NewRotation);

	if ((!bFoundTeleportSpot) && (TeleportOffsetStep != 0))
	{
		FVector TeleportOffset = FVector::ZeroVector;
		while ((!bFoundTeleportSpot) && (TeleportOffset.Size() < FlipVehicleMaxDistance))
		{
			TeleportOffset.X = 0;
			TeleportOffset.Y = 0;
			TeleportOffset.Z += TeleportOffsetStep;

			bFoundTeleportSpot = TeleportTo(NewLocation + TeleportOffset, NewRotation);

			float OffsetXY = 0;
			while ((!bFoundTeleportSpot) && TeleportOffset.Y < TeleportOffset.Z)
			{
				OffsetXY += TeleportOffsetStep;

				for (int32 X = -1; X <= 1; ++X)
				{
					TeleportOffset.X = OffsetXY * X;

					for (int32 Y = -1; Y <= 1; ++Y)
					{
						TeleportOffset.Y = OffsetXY * Y;

						if (TeleportTo(NewLocation + TeleportOffset, NewRotation))
						{
							bFoundTeleportSpot = true;
							break;
						}
					}

					if (bFoundTeleportSpot)
					{
						break;
					}
				}
			}


			if (!bFoundTeleportSpot)
			{
				// @TODO: Figure out whether we have a vehicle log category to use here
				UE_LOG(LogTemp, Warning, TEXT("Failed to flip vehicle. Could not find teleport spot."));
			}
		}
	}
}

FName ACitySampleVehicleBase::GetSeatSocket(ECitySampleVehicleSeat Seat) const
{
	return SeatSockets[static_cast<int>(Seat)];
}

ACitySampleCharacter* ACitySampleVehicleBase::GetOccupantInSeat(ECitySampleVehicleSeat Seat) const
{
	return Occupants[static_cast<int>(Seat)];
}

void ACitySampleVehicleBase::SetSeatOccupant(ECitySampleVehicleSeat Seat, ACitySampleCharacter* NewOccupant)
{
	Occupants[static_cast<int>(Seat)] = NewOccupant;
}

void ACitySampleVehicleBase::AddInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface)
{
	SubsystemInterface->AddMappingContext(InputMappingContext, InputMappingPriority);
	SubsystemInterface->AddMappingContext(LookControlsInputMappingContext, InputMappingPriority);

	bInputsActive = true;
}

void ACitySampleVehicleBase::RemoveInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface)
{
	SubsystemInterface->RemoveMappingContext(InputMappingContext);
	SubsystemInterface->RemoveMappingContext(LookControlsInputMappingContext);

	bInputsActive = false;
}

void ACitySampleVehicleBase::ThrottleActionBinding(const struct FInputActionValue& ActionValue)
{
	if (bInputsActive)
	{
		SetThrottleInput(ActionValue.GetMagnitude());
	}
}

void ACitySampleVehicleBase::BrakeActionBinding(const struct FInputActionValue& ActionValue)
{
	if (bInputsActive)
	{
		SetBrakeInput(ActionValue.GetMagnitude());
	}
}

void ACitySampleVehicleBase::SteeringActionBinding(const struct FInputActionValue& ActionValue)
{
	if (bInputsActive)
	{
		SetSteeringInput(ActionValue.GetMagnitude());
	}
}

void ACitySampleVehicleBase::HandbrakeActionBinding(const struct FInputActionValue& ActionValue)
{
	if (bInputsActive)
	{
		SetHandbrakeInput(ActionValue.IsNonZero());
	}
}

void ACitySampleVehicleBase::LookActionBinding(const struct FInputActionValue& ActionValue)
{
	Look(ActionValue.Get<FInputActionValue::Axis2D>());
}

void ACitySampleVehicleBase::LookDeltaActionBinding(const struct FInputActionValue& ActionValue)
{
	const FInputActionValue::Axis2D AxisValue = ActionValue.Get<FInputActionValue::Axis2D>();
	APawn::AddControllerYawInput(AxisValue.X);
	APawn::AddControllerPitchInput(AxisValue.Y);
}

void ACitySampleVehicleBase::UpdateTimeSpentSharpSteering(float DeltaTime)
{
	if (FMath::Abs(DrivingState.Steering) >= SharpSteeringThreshold)
	{
		TimeSpentSharpSteering += DeltaTime;
	}
	else
	{
		TimeSpentSharpSteering = 0.0f;
	}
}

void ACitySampleVehicleBase::ApplyWheelMotionBlurNative(const TArray<UMaterialInstanceDynamic*>& MotionBlurMIDs)
{
	static FName NAME_Angle(TEXT("Angle"));

	if (CachedMotionBlurWheelAngle.Num() < MotionBlurMIDs.Num())
	{
		CachedMotionBlurWheelAngle.AddZeroed(MotionBlurMIDs.Num() - CachedMotionBlurWheelAngle.Num());

		for (int Wheel = 0; Wheel < MotionBlurMIDs.Num(); Wheel++)
		{
			if (UMaterialInstanceDynamic* MID = MotionBlurMIDs[Wheel])
			{
				MID->SetScalarParameterValue(NAME_Angle, 0.f);
			}
		}
	}

	if (CachedMotionBlurWheelMIDs.Num() < MotionBlurMIDs.Num())
	{
		CachedMotionBlurWheelMIDs.AddZeroed(MotionBlurMIDs.Num() - CachedMotionBlurWheelMIDs.Num());
	}

	if (UChaosWheeledVehicleMovementComponent* MoveComp = Cast<UChaosWheeledVehicleMovementComponent>(GetMovementComponent()))
	{
		for (int i = 0; i < MotionBlurMIDs.Num(); i++)
		{
			if (UChaosVehicleWheel* Wheel = MoveComp->Wheels[i])
			{
				if (UMaterialInstanceDynamic* MID = MotionBlurMIDs[i])
				{
					const float AbsAngularVelocity = FMath::RadiansToDegrees(FMath::Abs(Wheel->GetWheelAngularVelocity()));
					float WheelAngle = AbsAngularVelocity / BlurAngleVelocityMax;
					WheelAngle = FMath::Clamp(WheelAngle, 0.f, 1.f) * BlurAngleMax;

					if (FMath::Abs(CachedMotionBlurWheelAngle[i] - WheelAngle) > KINDA_SMALL_NUMBER)
					{
						MID->SetScalarParameterValue(NAME_Angle, WheelAngle);
						CachedMotionBlurWheelAngle[i] = WheelAngle;
						CachedMotionBlurWheelMIDs[i] = MID;
					}
				}
			}
		}
	}
}

void ACitySampleVehicleBase::UpdateTransformsOverTime(USkeletalMeshComponent* SKM_Proxy)
{
	if (SKM_Proxy)
	{
		FTimestampedTransform NewTT(UKismetSystemLibrary::GetGameTimeInSeconds(this), SKM_Proxy->K2_GetComponentToWorld());
		TransformsOverTime.Insert(NewTT, 0);
		TransformsOverTime.Pop(false);
	}
}

void ACitySampleVehicleBase::UpdateContinuousCollisionDetectionAtSpeed()
{
	UChaosVehicleMovementComponent* MoveComp = GetVehicleMovementComponent();
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (IsPossessedByPlayer() && MoveComp && MoveComp->GetForwardSpeedMPH() > CCDSpeedThreshold)
		{
			if (!bUseCCDAtSpeed)
			{
				bUseCCDAtSpeed = true;
				MeshComp->SetAllUseCCD(true);
			}
		}
		else
		{
			if (bUseCCDAtSpeed)
			{
				bUseCCDAtSpeed = false;
				MeshComp->SetAllUseCCD(false);
			}
		}
	}
}
