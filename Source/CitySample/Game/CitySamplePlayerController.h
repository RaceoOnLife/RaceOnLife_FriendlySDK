// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "MassCommonTypes.h"

#include "Game/CitySampleInteractorInterface.h"
#include "UI/CitySampleControlsOverlayInterface.h"
#include "Util/CitySampleTypes.h"

#include "CitySamplePlayerController.generated.h"

class ACitySamplePlayerCameraManager;
class UCitySampleInteractionComponent;
class UCitySampleUIComponent;
class UInputAction;
class UInputMappingContext;
class UPhotoModeComponent;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FNotifyPawnChanged, AController*, APawn*, APawn*);

/**
 * Player Controller implementation specialized for City Sample needs
 */
UCLASS()
class CITYSAMPLE_API ACitySamplePlayerController : public APlayerController, 
										   public ICitySampleControlsOverlayInterface, 
										   public ICitySampleInteractorInterface
{
	GENERATED_BODY()
	
public:

	ACitySamplePlayerController();

	//~ Begin AActor Interface
	void TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	//~ End AActor Interface
	
	//~ Begin APlayerController Interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void UpdateRotation(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	//~ End APlayerController Interface
	
	//~ Begin AController Interface
	virtual void SetPawn(APawn* InPawn) override;
	virtual void AttachToPawn(APawn* InPawn) override;
	//~ End AController Interface

	//~ Begin ICitySampleIteractorInterface
	virtual void FinishInteraction() override;
	virtual void AbortInteraction() override;
	virtual bool IsInteracting() const override;
	virtual class APawn* GetInteractingPawn() override
	{
		return GetPawn();
	}

	virtual void TryToInteract(UCitySampleInteractionComponent* Component) override;
	virtual bool IsInteractingWith(UCitySampleInteractionComponent* Component) const override;
	//~ End ICitySampleInteractorInterface

	//~ Begin ICitySampleControlsOverlayInterface Interface
	/** Returns mapping of player controller relevant InputActions to their string description for the controls overlay UI. */
	virtual TMap<UInputAction*, FText> GetInputActionDescriptions_Implementation() const override
	{
		return InputActionDescriptions;
	};
	
	/**  Returns mapping of player controller relevant input keys to their string description for the controls overlay UI. */
	virtual TMap<FKey, FText> GetInputKeyDescriptionOverrides_Implementation() const override
	{
		return InputKeyDescriptionOverrides;
	};
	//~ End ICitySampleControlsOverlayInterface Interface

	UFUNCTION(BlueprintPure, Category = "UI")
	UCitySampleUIComponent* GetCitySampleUIComponent() const
	{
		return CitySampleUIComponent;
	}

	UFUNCTION(BlueprintCallable)
	UPhotoModeComponent* GetPhotoModeComponent() const
	{
		return PhotoModeComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	AActor* GetCurrentPlayerVehicle() const
	{
		return CurrentPlayerVehicle;
	}

	const FMassEntityHandle& GetCurrentPlayerVehicleMassHandle() const
	{
		return CurrentPlayerVehicleMassHandle;
	}

	/** Attempts to return the PlayerController's camera manager casted as a CitySampleCameraManager */
	UFUNCTION(BlueprintCallable)
	ACitySamplePlayerCameraManager* GetCitySampleCameraManager() const;

	/** Caches reference to current player vehicle, and also acquires a vehicle mass handle from the Mass Actor Subsystem */
	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void SetCurrentPlayerVehicle(AActor* NewPlayerVehicle);

	UClass* GetDronePawnClass() const
	{
		return DronePawnClass.Get();
	}

	UFUNCTION(BlueprintCallable)
	float GetCameraTransitionLevelStreamDistance2D() const
	{
		return CameraTransitionLevelStreamDistance2D;
	}

	UFUNCTION(BlueprintCallable)
	void SetCanPerformInteractions(bool bNewCanPerformInteractions)
	{
		bCanPerformInteractions = bNewCanPerformInteractions;
	}

	UFUNCTION(BlueprintPure)
	bool CanPerformInteractions() const
	{
		return bCanPerformInteractions;
	}

	/** Activates the PlayerController's referenced Input Mapping Context */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddInputContext(const bool bForce = false);

	/** Deactivates the PlayerController's referenced Input Mapping Context */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveInputContext();

	/** Attempts to activate the input context mapping of the pawn that is passed in */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddPawnInputContext(APawn* ToAdd);

	/** Attempts to remove the input context mapping of the pawn that is passed in */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemovePawnInputContext(APawn* ToRemove);

	/** Toggles the in-game options menu on and off while adjusting the appropriate input contexts */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	bool ToggleOptionsMenu();

	/** Requests that the camera manager begin a fade to black, alongside triggering a streaming pause to give the new area time to stream */
	UFUNCTION(BlueprintCallable)
	void HandleLongDistanceCameraTransition();

	/** Attempts to interact with the current visible interaction component */
	UFUNCTION(BlueprintCallable)
	void TryToInteractWithRelevantInteraction();

	/** Called by the Photo Mode Component when there is no valid pawn to repossess upon exiting Photo Mode */
	UFUNCTION(BlueprintImplementableEvent)
	void SpawnNewPawnLeavingPhotoMode();

	bool RotationInputWasZero() const 
	{ 
		return LastRotationInput.IsNearlyZero(); 
	}

	/* Keeps track of how the current player is traversing the map */
	EPlayerTraversalState CurrentTraversalState;

	/* Multicast delegate triggered when the pawn being controlled by this controller has changed
	 * Param1: this Controller, Param2: old Pawn, Param3: new Pawn */
	FNotifyPawnChanged NotifyPawnChanged;

protected:

	/** Determines spawn location for drone pawn when toggling drone mode on */
	UFUNCTION(BlueprintCallable, Category = "Drone")
	void UpdateSpawnTransformForEnteringDrone();

	/** Determines spawn location for character pawn when toggling drone mode off */
	UFUNCTION(BlueprintCallable, Category = "Drone")
	void UpdateSpawnTransformForLeavingDrone();

	/** Uses a hit result to determine if a character pawn is in a valid place to spawn when leaving drone mode. If false, more distant backup spawn points are used */
	bool IsHitResultAcceptableForLeavingDrone(const FHitResult& HitResult);

	/** Transform where drone pawn will be spawned when toggling drone mode on */
	UPROPERTY(BlueprintReadWrite, Transient)
	FTransform DroneToggleSpawnTransform;

	/** Pawn class to spawn when toggling drone mode on */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone")
	TSubclassOf<APawn> DronePawnClass;

	/** The radius within which interactions will be queried. Relative to Root Component transform. */
	UPROPERTY(EditDefaultsOnly)
	float InteractionRadius;

	/** When true, we will apply the scale of the Root Component for the Player Controller to the Interaction Radius. */
	UPROPERTY(EditDefaultsOnly)
	bool bScaleInteractionRadius;

	/** Mapping of player controller relevant InputActions to their string description for the controls overlay UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls Overlay")
	TMap<UInputAction*, FText> InputActionDescriptions;

	/** Mapping of player controller relevant input keys to their string description for the controls overlay UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls Overlay")
	TMap<FKey, FText> InputKeyDescriptionOverrides;

private:

	/** Reference to owned CitySampleUIComponent */
	UPROPERTY(VisibleAnywhere, Category = "CitySample UI")
	UCitySampleUIComponent* CitySampleUIComponent;

	/** Reference to owned PhotoModeComponent */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Photo Mode")
	UPhotoModeComponent* PhotoModeComponent = nullptr;

	/** Reference to vehicle the player is currently driving */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Vehicle")
	AActor* CurrentPlayerVehicle = nullptr;

	/** Mass Entity Handle for the vehicle the player is currently driving */
	FMassEntityHandle CurrentPlayerVehicleMassHandle;

	/** Processes the validity of an interaction component's state. Used when attempting to start or finish an interaction */
	void TryToUpdateInteraction(UCitySampleInteractionComponent* Interaction, uint8 InteractionState);

	/** Determines which nearby interaction component is dominant and updates UI prompts to reflect that */
	void UpdateInteractionPrompt();

	/** If false the player cannot initiate any interactions with interaction components */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	bool bCanPerformInteractions = true;

	/** Current interaction component that corresponds with the interaction prompt, and is the interaction that will be processed if an interaction is attempted */
	UPROPERTY(Transient)
	UCitySampleInteractionComponent* CurrentVisibleInteractionComponent = nullptr;

	/** Reference for interaction component that the player controller is currently interacting with */
	UPROPERTY(Transient)
	UCitySampleInteractionComponent* CurrentInteractionComponent = nullptr;

	/** Set of possible interactions that the player is within range to interact with */
	TSet<UCitySampleInteractionComponent*> OverlappingInteractions;

	/** Struct for storing sets of interaction components along with their current state */
	struct FInteractionItem
	{
		TWeakObjectPtr<UCitySampleInteractionComponent> ToInteractWith;
		uint8 InteractionState;
	};

	/** Stored queue of interactions to be processed */
	TArray<FInteractionItem> InteractionQueue;

	/** Check if world streaming is complete so that we can end a camera transition */
	UFUNCTION()
	void CheckIfSafeToEndCameraTransition();

	/** Requests that the camera manager begin a fade to black, and sets up a timer to follow up with EndPenetratingCameraTransition */
	void HandlePenetratingCameraTransition();

	/** Requests that the camera manager begin a a fade from black */
	void EndPenetratingCameraTransition();

	/** Handle for looped timer that checks for level streaming status before finishing the camera transition */
	FTimerHandle CameraTransitionWaitingForLevelStreamingHandle;

	/** Handle for timer used to gate camera fades when needed for camera transitions */
	FTimerHandle CameraTransitionHidePenetrationHandle;

	/** Amount of time it takes the camera to fade to black when fade transition is triggered */
	UPROPERTY(Config)
	float CameraPenetratingFadeToBlackTime = 0.1f;

	/** Amount of time spent on a black screen before the fade from black is triggered */
	UPROPERTY(Config)
	float CameraPenetratingFadeFromBlackDelayTime = 0.75f;

	/** Amount of time it takes the camera to fade from black when fade transition is triggered */
	UPROPERTY(Config)
	float CameraPenetratingFadeFromBlackTime = 0.5f;

	/** Distance needed to trigger a long distance camera transition */
	UPROPERTY(Config)
	float CameraTransitionLevelStreamDistance2D = 20000.0f;

	/** Query distance used when determining if world partition streaming has completed during long distance camera transitions */
	UPROPERTY(Config)
	float CameraTransitionWPQueryDistance = 10000.0f;

	/** Stores last player rotation input to be used with RotationInputWasZero()*/
	FRotator LastRotationInput;

	/** Called when input context mappings have changed, informs the UIComponent to update the controls overlay */
	void MarkControlsDirty();

	/** Flag to stop the options menu from toggling twice from one input when swapping input contexts that share the ToggleOptionsMenuActionBinding */
	bool bBlockToggleOptions = false;

	/** Input Mapping and Input ACtions that the Player Controller relies on */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 InputMappingPriority = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* ToggleOptionsMenuAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* ToggleOptionsMenuUIAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* ToggleOptionsMenuReleaseAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* InteractAction;

	/** Methods binded to the player controller's input actions to facilitate input from the player controller */
	void ToggleOptionsMenuActionBinding(const struct FInputActionValue& ActionValue);
	void ToggleOptionsMenuReleaseActionBinding(const struct FInputActionValue& ActionValue);
	void InteractActionBinding(const struct FInputActionValue& ActionValue);

};

