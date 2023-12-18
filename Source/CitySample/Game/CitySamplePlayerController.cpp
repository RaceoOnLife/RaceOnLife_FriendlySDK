// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySamplePlayerController.h"

#include "Algo/Sort.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MassActorSubsystem.h"

#include "Camera/CitySamplePlayerCameraManager.h"
#include "Camera/PhotoModeComponent.h"
#include "CitySample.h"
#include "Game/CitySampleCheatManager.h"
#include "Game/CitySampleGameInstanceBase.h"
#include "Game/CitySampleInteractionComponent.h"
#include "Game/CitySampleSaveGame.h"
#include "UI/CitySampleControlsOverlay.h"
#include "UI/CitySampleUIComponent.h"
#include "Util/CitySampleBlueprintLibrary.h"
#include "Util/ICitySampleInputInterface.h"
#include "Vehicles/CitySampleMassVehicleBase.h"
#include "Vehicles/CitySampleVehicleBase.h"


namespace CitySampleInteractionsPrivate
{
	FText GetGenericInvalid()
	{
		return NSLOCTEXT("CitySampleCharacter", "GenericInvalid", "Invalid");
	}

	enum class EInteractionState : uint8
	{
		Start,
		Abort,
		Finish,
		COUNT
	};

	bool IsInteractionStateValid(uint8 InteractionState)
	{
		return InteractionState >= 0 &&
				InteractionState < static_cast<uint8>(EInteractionState::COUNT);
	}

	FText InteractionStateToText(EInteractionState InteractionState)
	{
		switch(InteractionState)
		{
			case EInteractionState::Start:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionState::Start", "Start");

			case EInteractionState::Abort:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionState::Aborted", "Abort");

			case EInteractionState::Finish:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionState::Finished", "Finish");

			default:
				return GetGenericInvalid();
		}
	}

	FText InteractionStateToText(uint8 InteractionState)
	{
		return InteractionStateToText(static_cast<EInteractionState>(InteractionState));
	}

	enum class EInteractionResult : uint8
	{
		Success,
		CharacterBusy,
		InteractionBusy,
		OutOfRange,
		InvalidAuthority,
		InvalidInteraction,
		COUNT
	};
	
	bool IsInteractionResultValid(uint8 InteractionResult)
	{
		return InteractionResult >= 0 &&
				InteractionResult < static_cast<uint8>(EInteractionResult::COUNT);
	}

	FText InteractionResultToText(EInteractionResult InteractionResult)
	{
		switch(InteractionResult)
		{
			case EInteractionResult::Success:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionResult::Success", "Success");

			case EInteractionResult::CharacterBusy:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionResult::CharacterWasBusy", "Character was in another interaction.");

			case EInteractionResult::InteractionBusy:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionResult::InteractionBusy", "Interaction was being performed by another character.");

			case EInteractionResult::OutOfRange:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionResult::OutOfRange", "Interaction was out of range.");

			case EInteractionResult::InvalidAuthority:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionResult::InvalidAuthority", "Character does not have authority to perform the interaction.");

			case EInteractionResult::InvalidInteraction:
				return NSLOCTEXT("CitySampleCharacter", "EInteractionResult::InvalidInteraction", "Character was not currently performing the interaction.");

			default:
				return GetGenericInvalid();
		}
	}

	FText InteractionResultToText(uint8 InteractionResult)
	{
		return InteractionResultToText(static_cast<EInteractionResult>(InteractionResult));
	}
}

ACitySamplePlayerController::ACitySamplePlayerController()
{
	bAttachToPawn = true;

	bScaleInteractionRadius = true;
	InteractionRadius = 128.f;

	CheatClass = UCitySampleCheatManager::StaticClass();

	bEnableStreamingSource = false;
}

void ACitySamplePlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	
	if (UCitySampleGameInstanceBase* const GameInstance = GetGameInstance<UCitySampleGameInstanceBase>())
	{
		if (GameInstance->IsSaveGameLoaded())
		{
			// Ensure force feedback setting matches save game data
			bForceFeedbackEnabled = GameInstance->GetSaveGame()->bForceFeedbackEnabled;
		}
	}
}

void ACitySamplePlayerController::TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaSeconds, TickType, ThisTickFunction);

	if (IsLocalPlayerController())
	{
		UpdateInteractionPrompt();
	}
}

void ACitySamplePlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CitySampleUIComponent = FindComponentByClass<UCitySampleUIComponent>();
	PhotoModeComponent = FindComponentByClass<UPhotoModeComponent>();
}

void ACitySamplePlayerController::UpdateRotation(float DeltaTime)
{
	LastRotationInput = RotationInput;

	Super::UpdateRotation(DeltaTime);
}

void ACitySamplePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	AddInputContext();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(ToggleOptionsMenuAction, ETriggerEvent::Started, this, &ThisClass::ToggleOptionsMenuActionBinding);
		EnhancedInputComponent->BindAction(ToggleOptionsMenuUIAction, ETriggerEvent::Started, this, &ThisClass::ToggleOptionsMenuActionBinding);
		EnhancedInputComponent->BindAction(ToggleOptionsMenuReleaseAction, ETriggerEvent::Triggered, this, &ThisClass::ToggleOptionsMenuReleaseActionBinding);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ThisClass::InteractActionBinding);
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("Failed to setup player input for %s, InputComponent type is not UEnhancedInputComponent."), *GetName());
	}

	// Since this function is called after PostInitializeComponents(), we can set up the input bindings for components here
	if (CitySampleUIComponent)
	{
		CitySampleUIComponent->SetupInputBindings();
	}

	if (PhotoModeComponent)
	{
		PhotoModeComponent->SetUpInputs();
	}
}

void ACitySamplePlayerController::SetPawn(APawn* InPawn)
{
	TScriptInterface<IEnhancedInputSubsystemInterface> EnhancedInputInterface;
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		UEnhancedInputLocalPlayerSubsystem* PlayerSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		EnhancedInputInterface = PlayerSubsystem;
	}

	APawn* PrevPawn = GetPawn();
	RemovePawnInputContext(PrevPawn);

	Super::SetPawn(InPawn);

	if (PrevPawn != GetPawn())
	{
		NotifyPawnChanged.Broadcast(this, PrevPawn, GetPawn());
	}

	if (InPawn)
	{
		if (CitySampleUIComponent && !CitySampleUIComponent->IsOptionsMenuActive())
		{
			AddPawnInputContext(GetPawn());
		}
	}
	else
	{
		CurrentTraversalState = EPlayerTraversalState::OnFoot;
	}
}

void ACitySamplePlayerController::AttachToPawn(APawn* InPawn)
{
	if (RootComponent)
	{
		if (InPawn)
		{
			// Only attach if not already attached.
			if (InPawn->GetRootComponent() && RootComponent->GetAttachParent() != InPawn->GetRootComponent())
			{
				RootComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
				RootComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
				RootComponent->AttachToComponent(InPawn->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
		}
		else
		{
			DetachFromPawn();
		}
	}
}


ACitySamplePlayerCameraManager* ACitySamplePlayerController::GetCitySampleCameraManager() const
{

	return Cast<ACitySamplePlayerCameraManager>(PlayerCameraManager);
}


//////////////////////////////////////////////////////////////////////////
// Begin Interactions

bool ACitySamplePlayerController::IsInteracting() const
{
	return CurrentInteractionComponent != nullptr;
}

bool ACitySamplePlayerController::IsInteractingWith(UCitySampleInteractionComponent* Interaction) const
{
	return Interaction != nullptr && CurrentInteractionComponent == Interaction;
}

void ACitySamplePlayerController::TryToInteractWithRelevantInteraction()
{
	TryToInteract(CurrentVisibleInteractionComponent);
}

void ACitySamplePlayerController::TryToInteract(UCitySampleInteractionComponent* Interaction)
{
	using namespace CitySampleInteractionsPrivate;

	if (IsInteracting())
	{
		UE_LOG(LogCitySample, Log, TEXT("%s::TryToInteract: Already interacting with %s"),
			*GetPathName(this), *GetPathName(CurrentInteractionComponent));

		return;
	}

	if (!Interaction)
	{
		UE_LOG(LogCitySample, Log, TEXT("%s::TryToInteract: Invalid interaction"),
			*GetPathName(this));

		return;
	}

	if (!Interaction->TryToLockInteraction(this))
	{
		UE_LOG(LogCitySample, Log, TEXT("%s::TryToInteract: Interaction is busy %s"),
			*GetPathName(this), *GetPathName(Interaction));
			
		return;
	}

	CurrentInteractionComponent = Interaction;
	TryToUpdateInteraction(CurrentInteractionComponent, static_cast<uint8>(EInteractionState::Start));
}

void ACitySamplePlayerController::FinishInteraction()
{
	using namespace CitySampleInteractionsPrivate;

	if (IsInteracting())
	{
		TryToUpdateInteraction(CurrentInteractionComponent, static_cast<uint8>(EInteractionState::Finish));
	}
}

void ACitySamplePlayerController::AbortInteraction()
{
	using namespace CitySampleInteractionsPrivate;

	if (IsInteracting())
	{
		TryToUpdateInteraction(CurrentInteractionComponent, static_cast<uint8>(EInteractionState::Abort));
	}
}

void ACitySamplePlayerController::TryToUpdateInteraction(UCitySampleInteractionComponent* ToInteractWith, uint8 InInteractionState)
{
	using namespace CitySampleInteractionsPrivate;

	if (!ToInteractWith)
	{
		UE_LOG(LogCitySample, Error, TEXT("TryToUpdateInteraction_Validate:%s: Invalid Interaction Object. Object=%s, Type=%s"),
			*GetPathName(this), *GetPathNameSafe(ToInteractWith), *InteractionStateToText(InInteractionState).ToString());

		return;
	}

	if (!IsInteractionStateValid(InInteractionState))
	{
		UE_LOG(LogCitySample, Error, TEXT("TryToUpdateInteraction_Validate:%s: Invalid Interaction Type. Object=%s, Type=%s"),
			*GetPathName(this), *GetPathName(ToInteractWith), *InteractionStateToText(InInteractionState).ToString())
		
		return;
	}
	
	const EInteractionState InteractionState = static_cast<EInteractionState>(InInteractionState);
	EInteractionResult InteractionResult = EInteractionResult::Success;

	auto LogInteraction = [&]()
	{
		UE_LOG(LogCitySample, Log, TEXT("TryToUpdateInteraction:%s: Interaction. Object=%s, Type=%s, Reason=%s"),
			*GetPathName(this), *GetPathName(ToInteractWith), *InteractionStateToText(InteractionState).ToString(),
			*InteractionResultToText(InteractionResult).ToString());
	};

	if (InteractionQueue.Num() > 0)
	{
		const FInteractionItem& CurrentlyProcessing = InteractionQueue[0];
		if (!ensure(EInteractionState::Start == static_cast<EInteractionState>(CurrentlyProcessing.InteractionState)) ||
			!ensure(ToInteractWith == CurrentlyProcessing.ToInteractWith))
		{
			// We're trying to start a new interaction while processing an existing one.
			// This should not be possible out of the box
			InteractionResult = EInteractionResult::InteractionBusy;
			LogInteraction();
		}
		else
		{
			// We receive an Abort or Finished from within a Start.
			// Go ahead and just push this interaction state onto the queue until Start has finished.
			InteractionQueue.Push({ToInteractWith, InInteractionState});
		}

		return;
	}

	if (EInteractionState::Start == InteractionState)
	{
		if (!IsInteractingWith(ToInteractWith))
		{
			if (!ensure(!IsInteracting()))
			{
				InteractionResult = EInteractionResult::CharacterBusy;
			}
			else if (!OverlappingInteractions.Contains(ToInteractWith))
			{
				InteractionResult = EInteractionResult::OutOfRange;
			}
			else if (!ToInteractWith->TryToLockInteraction(this))
			{
				InteractionResult = EInteractionResult::InteractionBusy;
			}
		}
	}
	else if (EInteractionState::Finish == InteractionState || EInteractionState::Abort == InteractionState)
	{
		// Currently, the only ways in which we'll fail a Finish or Abort would be if we aren't interacting
		// with the specified component, or if the specified component thinks it isn't being interacted with
		// by this character.
		// Neither should never actually happen, and if they do there's a bug somewhere else.
		
		if (!ensure(IsInteractingWith(ToInteractWith)))
		{
			InteractionResult = EInteractionResult::InvalidInteraction;
		}

		if (!ensure(CurrentInteractionComponent->TryToReleaseInteraction(this)))
		{
			// The interaction is in an inconsistent state and actually
			// busy with another actor (shouldn't be possible).
			InteractionResult = EInteractionResult::InteractionBusy;
			CurrentInteractionComponent = nullptr;
		}
	}

	LogInteraction();

	if (EInteractionResult::Success == InteractionResult)
	{
		// Push our current interaction onto the stack. That way if we trigger a subsequent one while processing,
		// it knows to deffer itself.
		// If we aren't the only interaction on the stack, go ahead with the defferal.
		InteractionQueue.Push({ToInteractWith, InInteractionState});

		switch (InteractionState)
		{
		case EInteractionState::Start:
			CurrentInteractionComponent = ToInteractWith;
			RemoveInputContext();
			CurrentInteractionComponent->OnInteractionStarted(this);
			CurrentInteractionComponent->OnInteract.Broadcast(this);
			break;

		case EInteractionState::Finish:
			CurrentInteractionComponent->OnInteractionFinished(this);
			CurrentInteractionComponent = nullptr;
			AddInputContext();
			break;

		case EInteractionState::Abort:
			CurrentInteractionComponent->OnInteractionAborted(this);
			CurrentInteractionComponent = nullptr;
			AddInputContext();
			break;

		default:
			// This shouldn't be possible.
			ensure(false);
			break;
		}

		InteractionQueue.RemoveAt(0);

		if (InteractionQueue.Num() > 0)
		{
			const FInteractionItem ToProcess = InteractionQueue[0];
			InteractionQueue.RemoveAt(0);
			TryToUpdateInteraction(ToProcess.ToInteractWith.Get(), ToProcess.InteractionState);
		}
	}
}

void ACitySamplePlayerController::UpdateInteractionPrompt()
{
	QUICK_SCOPE_CYCLE_COUNTER(UpdateInteractionPrompt);

	OverlappingInteractions.Reset();

	if (!bCanPerformInteractions)
	{
		// If we can't perform interactions, we clear out any currently visible prompts
		if (CurrentVisibleInteractionComponent != nullptr)
		{
			if (UCitySampleUIComponent* CitySampleUI = GetCitySampleUIComponent())
			{
				CitySampleUI->RemoveInteractionPrompt();
				CurrentVisibleInteractionComponent = nullptr;
			}
		}

		return;
	}

	if (UCitySampleUIComponent* CitySampleUI = GetCitySampleUIComponent())
	{
		UCitySampleInteractionComponent* ToShow = nullptr;
		const TScriptInterface<ICitySampleInteractorInterface> Interactor(this);

		if (!IsInteracting())
		{
			const FTransform RootComponentTransform = GetRootComponent()->GetComponentToWorld();
			const FVector RootComponentLocation = RootComponentTransform.GetLocation();

			TArray<struct FOverlapResult> OutOverlaps;

			FCollisionShape CollisionShape;
			CollisionShape.SetSphere((bScaleInteractionRadius ? RootComponentTransform.GetMinimumAxisScale() : 1.f) * InteractionRadius);

			if (GetWorld()->OverlapMultiByChannel(OutOverlaps, RootComponentLocation, RootComponentTransform.GetRotation(), CitySampleECC_InteractionsTrace, CollisionShape))
			{
				using FRankType = TPair<UCitySampleInteractionComponent*, float>;
				TArray<FRankType> FoundComponents;
				for (int32 i = OutOverlaps.Num() - 1; i >= 0; --i)
				{
					if (UCitySampleInteractionComponent* PotentialOveraction = Cast<UCitySampleInteractionComponent>(OutOverlaps[i].GetComponent()))
					{
						if (PotentialOveraction->CanInteractWith(Interactor))
						{
							const float DistanceSquared = (PotentialOveraction->GetComponentToWorld().GetLocation() - RootComponentLocation).SizeSquared();
							
							OverlappingInteractions.Add(PotentialOveraction);
							FoundComponents.Emplace(PotentialOveraction, DistanceSquared);
						}
					}
				}

				if (FoundComponents.Num() > 0)
				{
					auto DistanceSort = [](const FRankType& LHS)
					{
						return LHS.Get<1>();
					};

					Algo::SortBy<>(FoundComponents, DistanceSort);
					ToShow = FoundComponents[0].Get<0>();
				}
			}
		}

		if (CurrentVisibleInteractionComponent != ToShow)
		{
			CitySampleUI->RemoveInteractionPrompt();

			if (ToShow)
			{
				CitySampleUI->AddInteractionPrompt(ToShow);
			}

			CurrentVisibleInteractionComponent = ToShow;
		}
	}
}

// End Interactions
//////////////////////////////////////////////////////////////////////////

bool ACitySamplePlayerController::ToggleOptionsMenu()
{
	bool bAreOptionsActive = false;
	if (CitySampleUIComponent)
	{
		bAreOptionsActive = CitySampleUIComponent->SetOptionsMenuActive(!CitySampleUIComponent->IsOptionsMenuActive());

		// If the options were closed via the PC's toggle options input, it's safe to say we want our game-centric input contexts back
		if (!bAreOptionsActive)
		{
			AddInputContext();
			AddPawnInputContext(GetPawn());
		}

		return bAreOptionsActive;
	}

	return bAreOptionsActive;
}

void ACitySamplePlayerController::SetCurrentPlayerVehicle(AActor* NewPlayerVehicle)
{
	CurrentPlayerVehicle = NewPlayerVehicle;

	UMassActorSubsystem* MassActorSubsystem = UWorld::GetSubsystem<UMassActorSubsystem>(GetWorld());
	if (MassActorSubsystem)
	{
		CurrentPlayerVehicleMassHandle = MassActorSubsystem->GetEntityHandleFromActor(NewPlayerVehicle);
	}
}

void ACitySamplePlayerController::AddInputContext(const bool bForce/*=false*/)
{
	// Unless forced, do nothing when interacting or the options menu is open, as they self-manage the input context
	if (!bForce && (IsInteracting() || (CitySampleUIComponent && CitySampleUIComponent->IsOptionsMenuActive())))
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(InputMappingContext, InputMappingPriority);
			MarkControlsDirty();
		}
	}
}

void ACitySamplePlayerController::RemoveInputContext()
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
			MarkControlsDirty();
		}
	}
}

void ACitySamplePlayerController::AddPawnInputContext(APawn* ToAdd)
{
	if (!ToAdd)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* PlayerSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			const TScriptInterface<IEnhancedInputSubsystemInterface> EnhancedInputInterface = PlayerSubsystem;
			if (ToAdd->GetClass()->ImplementsInterface(UCitySampleInputInterface::StaticClass()))
			{
				ICitySampleInputInterface::Execute_AddInputContext(ToAdd, EnhancedInputInterface);
				MarkControlsDirty();
			}
		}
	}
}

void ACitySamplePlayerController::RemovePawnInputContext(APawn* ToRemove)
{
	if (!ToRemove)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* PlayerSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			const TScriptInterface<IEnhancedInputSubsystemInterface> EnhancedInputInterface = PlayerSubsystem;
			if (ToRemove->GetClass()->ImplementsInterface(UCitySampleInputInterface::StaticClass()))
			{
				ICitySampleInputInterface::Execute_RemoveInputContext(ToRemove, EnhancedInputInterface);
				MarkControlsDirty();
			}
		}
	}
}

void ACitySamplePlayerController::MarkControlsDirty()
{
	if (IsValid(CitySampleUIComponent))
	{
		CitySampleUIComponent->RequestControlsOverlayUpdate();
	}
}

void ACitySamplePlayerController::ToggleOptionsMenuActionBinding(const struct FInputActionValue& ActionValue)
{
	if (!bBlockToggleOptions)
	{
		ToggleOptionsMenu();
		bBlockToggleOptions = true;
	}
}

void ACitySamplePlayerController::ToggleOptionsMenuReleaseActionBinding(const struct FInputActionValue& ActionValue)
{
	bBlockToggleOptions = false;
}

void ACitySamplePlayerController::InteractActionBinding(const struct FInputActionValue& ActionValue)
{
	TryToInteractWithRelevantInteraction();
}

bool ACitySamplePlayerController::IsHitResultAcceptableForLeavingDrone(const FHitResult& HitResult)
{
	if (HitResult.HasValidHitObjectHandle())
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor->IsA(ACitySampleVehicleBase::StaticClass()))
		{
			return false;
		}
		else if (HitActor->IsA(ACitySampleMassVehicleBase::StaticClass()))
		{
			return false;
		}
		else if (HitActor->IsA(ABlockingVolume::StaticClass()))
		{
			// Over blocking volume implies water
			return false;
		}

		TArray<FName> BannedExitBaseClasses;
		BannedExitBaseClasses.Add(TEXT("BPP_Dock_A_C"));
		BannedExitBaseClasses.Add(TEXT("BPP_Dock_B_C"));
		BannedExitBaseClasses.Add(TEXT("BPP_Warehouse_A_C"));
		BannedExitBaseClasses.Add(TEXT("BPP_Warehouse_B_C"));
		BannedExitBaseClasses.Add(TEXT("BPP_Warehouse_C_C"));

		if (BannedExitBaseClasses.Contains(HitActor->GetClass()->GetFName()))
		{
			return false;
		}
	}

	return true;
}

void ACitySamplePlayerController::UpdateSpawnTransformForEnteringDrone()
{
	if (ACitySamplePlayerCameraManager* FCM = GetCitySampleCameraManager())
	{
		const FVector CameraLoc = FCM->GetCameraLocation();
		const FRotator CameraRot = FCM->GetCameraRotation();

		FVector NewSpawnLocation = CameraLoc;
		NewSpawnLocation.Z += 250.0f;
		FRotator NewSpawnRotation(0,0,0);
		NewSpawnRotation.Yaw = CameraRot.Yaw;

		TArray<TEnumAsByte<EObjectTypeQuery> > TraceObjectTypes;
		TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
		TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Vehicle));
		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(GetPawn());
		FHitResult HitResult;

		const float DroneRadius = 70.0f;

		UKismetSystemLibrary::SphereTraceSingleForObjects(this, CameraLoc, NewSpawnLocation, DroneRadius, TraceObjectTypes, false, IgnoreActors, EDrawDebugTrace::None, HitResult, true);
		
		if (HitResult.bBlockingHit)
		{
			NewSpawnLocation.Z = HitResult.Location.Z;
		}

		DroneToggleSpawnTransform.SetTranslation(NewSpawnLocation);
		DroneToggleSpawnTransform.SetRotation(NewSpawnRotation.Quaternion());
	}
}

void ACitySamplePlayerController::UpdateSpawnTransformForLeavingDrone()
{
	// Could read this direct from the pawn class to be cleaner and more future proof
	const float PawnHalfHeight = 90.0f;

	const FVector DroneLocation = DroneToggleSpawnTransform.GetTranslation();
	FVector NewTranslation = DroneLocation;
	FRotator NewRotation = DroneToggleSpawnTransform.GetRotation().Rotator();
	FVector StartTrace(DroneLocation.X, DroneLocation.Y, DroneLocation.Z);
	// -200 should be safely under map
	FVector EndTrace(DroneLocation.X, DroneLocation.Y, -200.0f);
	TArray<TEnumAsByte<EObjectTypeQuery> > TraceObjectTypes;
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Vehicle));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(GetPawn());

	FHitResult HitResult;
	UKismetSystemLibrary::LineTraceSingleForObjects(this, StartTrace, EndTrace, TraceObjectTypes, false, IgnoreActors, EDrawDebugTrace::None, HitResult, true);

	NewRotation.Pitch = NewRotation.Roll = 0.0f;
	DroneToggleSpawnTransform.SetRotation(NewRotation.Quaternion());

	const float LaneSearchDistance = 10000.0f;
	static FName NAME_Pedestrian("Pedestrian");
	static FName NAME_Traffic("Traffic");
	NewTranslation.Z = 0.0f;
	DroneToggleSpawnTransform.SetTranslation(NewTranslation);
	FTransform PedestrianLaneTransform;
	const bool bFoundNearestPedestrianLane = UCitySampleBlueprintLibrary::FindNearestLaneLocationByName(this, DroneToggleSpawnTransform, NAME_Pedestrian, PedestrianLaneTransform, LaneSearchDistance);
	FTransform TrafficLaneTransform;
	const bool bFoundNearestTrafficLane = UCitySampleBlueprintLibrary::FindNearestLaneLocationByName(this, DroneToggleSpawnTransform, NAME_Traffic, TrafficLaneTransform, LaneSearchDistance);

	const float ZTolerance = PawnHalfHeight * 2.0f;
	bool bWithinAcceptableZ = false;
	if (bFoundNearestPedestrianLane)
	{
		if (FMath::Abs(HitResult.Location.Z - PedestrianLaneTransform.GetTranslation().Z) < ZTolerance)
		{
			bWithinAcceptableZ = true;
		}
	}
	
	if (bFoundNearestTrafficLane)
	{
		if (FMath::Abs(HitResult.Location.Z - TrafficLaneTransform.GetTranslation().Z) < ZTolerance)
		{
			bWithinAcceptableZ = true;
		}
	}

	if (!bFoundNearestPedestrianLane && !bFoundNearestTrafficLane)
	{
		if (UWorld* World = GetWorld())
		{
			if (AGameModeBase* GM = World->GetAuthGameMode())
			{
				if (AActor* PS = GM->FindPlayerStart(this))
				{
					if (FMath::Abs(HitResult.Location.Z - PS->GetActorLocation().Z) < ZTolerance)
					{
						bWithinAcceptableZ = true;
					}
				}
			}
		}
	}

	// Avoid standing on vehicles (and possibly other actors later)
	if (HitResult.bBlockingHit && IsHitResultAcceptableForLeavingDrone(HitResult) && bWithinAcceptableZ)
	{
		NewTranslation.Z = HitResult.Location.Z + PawnHalfHeight;
		DroneToggleSpawnTransform.SetTranslation(NewTranslation);
	}
	else
	{
		if (bFoundNearestPedestrianLane)
		{
			NewTranslation = PedestrianLaneTransform.GetTranslation();
			NewTranslation.Z += PawnHalfHeight;
			DroneToggleSpawnTransform.SetLocation(NewTranslation);
		}
		else if (bFoundNearestTrafficLane)
		{
			NewTranslation = TrafficLaneTransform.GetTranslation();
			NewTranslation.Z += PawnHalfHeight;
			DroneToggleSpawnTransform.SetLocation(NewTranslation);
		}
		else
		{
			if (UWorld* World = GetWorld())
			{
				if (AGameModeBase* GM = World->GetAuthGameMode())
				{
					if (AActor* PS = GM->FindPlayerStart(this))
					{
						NewTranslation = PS->GetActorLocation();
						DroneToggleSpawnTransform.SetLocation(NewTranslation);
					}
				}
			}
		}
	}

	const float TravelDistance = (DroneLocation - DroneToggleSpawnTransform.GetTranslation()).Size2D();
	if (TravelDistance > CameraTransitionLevelStreamDistance2D)
	{
		HandleLongDistanceCameraTransition();
	}
	else
	{
		TArray<TEnumAsByte<EObjectTypeQuery> > ExitTraceObjectTypes;
		ExitTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		ExitTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

		TArray<AActor*> ExitIgnoreActors;
		ExitIgnoreActors.Add(GetPawn());

		FHitResult ExitHitResult;
		UKismetSystemLibrary::LineTraceSingleForObjects(this, DroneLocation, DroneToggleSpawnTransform.GetTranslation(), ExitTraceObjectTypes, false, ExitIgnoreActors, EDrawDebugTrace::None, ExitHitResult, true);
		if (ExitHitResult.bBlockingHit)
		{
			HandlePenetratingCameraTransition();
		}
	}
}

void ACitySamplePlayerController::HandlePenetratingCameraTransition()
{
	const UWorld* World = GetWorld();

	if (!World->GetTimerManager().IsTimerActive(CameraTransitionHidePenetrationHandle))
	{
		if (PlayerCameraManager)
		{
			UE_LOG(LogCitySample, Log, TEXT("ACitySamplePlayerController::HandlePenetratingCameraTransition StartCameraFade to alpha 1 black (fade out)"));
			PlayerCameraManager->StartCameraFade(1.0f, 1.0f, CameraPenetratingFadeToBlackTime, FLinearColor::Black, true, true);
		}
		if (World)
		{
			World->GetTimerManager().SetTimer(CameraTransitionHidePenetrationHandle, this, &ThisClass::EndPenetratingCameraTransition, CameraPenetratingFadeFromBlackDelayTime);
		}
	}
}

void ACitySamplePlayerController::EndPenetratingCameraTransition()
{
	if (PlayerCameraManager && PlayerCameraManager->bEnableFading)
	{
		UE_LOG(LogCitySample, Log, TEXT("ACitySamplePlayerController::CheckIfSafeToEndCameraTransition StartCameraFade to alpha 0 (fade in)"));
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, CameraPenetratingFadeFromBlackTime, FLinearColor::Black, true, false);
	}
}

void ACitySamplePlayerController::HandleLongDistanceCameraTransition()
{
	const UWorld* World = GetWorld();

	if (!World->GetTimerManager().IsTimerActive(CameraTransitionWaitingForLevelStreamingHandle))
	{
		if (PlayerCameraManager)
		{
			UE_LOG(LogCitySample, Log, TEXT("ACitySamplePlayerController::HandleLongDistanceCameraTransition StartCameraFade to alpha 1 black (fade out)"));
			PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 0.1f, FLinearColor::Black, true, true);
		}

		if (GEngine->GameViewport && GEngine->BeginStreamingPauseDelegate && GEngine->BeginStreamingPauseDelegate->IsBound())
		{
			GEngine->BeginStreamingPauseDelegate->Execute(GEngine->GameViewport->Viewport);
		}

		if (World)
		{
			World->GetTimerManager().SetTimer(CameraTransitionWaitingForLevelStreamingHandle, this, &ThisClass::CheckIfSafeToEndCameraTransition, 0.5f, true, 2.0f);
		}
	}
}

void ACitySamplePlayerController::CheckIfSafeToEndCameraTransition()
{
	if (UWorld* World = GetWorld())
	{
		// Wait for the world partition bubble to be loaded
		UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>();
		UDataLayerSubsystem* DataLayerSubsystem = World->GetSubsystem<UDataLayerSubsystem>();
		if (WorldPartitionSubsystem && DataLayerSubsystem)
		{
			FVector VPLocation;
			FRotator VPRotation;
			GetActorEyesViewPoint(VPLocation, VPRotation);

			// Build a query source
			TArray<FWorldPartitionStreamingQuerySource> QuerySources;
			FWorldPartitionStreamingQuerySource& QuerySource = QuerySources.Emplace_GetRef();
			QuerySource.bSpatialQuery = true;
			QuerySource.Location = VPLocation;
			QuerySource.Rotation = VPRotation;
			QuerySource.bUseGridLoadingRange = false;
			QuerySource.Radius = CameraTransitionWPQueryDistance;
			QuerySource.bDataLayersOnly = false;
			QuerySource.DataLayers = DataLayerSubsystem->GetEffectiveLoadedDataLayerNames().Array();

			// Execute query
			const EWorldPartitionRuntimeCellState QueryState = EWorldPartitionRuntimeCellState::Loaded;
			const bool bStreamingCompleted = WorldPartitionSubsystem->IsStreamingCompleted(QueryState, QuerySources, /*bExactState*/ false);

			if (!bStreamingCompleted)
			{
				return;
			}

			World->GetTimerManager().ClearTimer(CameraTransitionWaitingForLevelStreamingHandle);
		}
	}

	// Don't fade from black, if fading wasn't done before
	if (PlayerCameraManager && PlayerCameraManager->bEnableFading)
	{
		UE_LOG(LogCitySample, Log, TEXT("ACitySamplePlayerController::CheckIfSafeToEndCameraTransition StartCameraFade to alpha 0 (fade in)"));
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.5f, FLinearColor::Black, true, false);
	}
	if (GEngine->EndStreamingPauseDelegate && GEngine->EndStreamingPauseDelegate->IsBound())
	{
		GEngine->EndStreamingPauseDelegate->Execute();
	}
}