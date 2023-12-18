// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Util/CitySampleInterpolators.h"

#include "PhotoModeComponent.generated.h"

class ACitySampleHoverDrone;

DECLARE_LOG_CATEGORY_EXTERN(LogCitySamplePhotoMode, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhotoModeActivated);

/**
 * Enum to help manage Photo Mode states
 */
UENUM(BlueprintType)
enum class EPhotoModeState : uint8
{
	NotActive,
	Pending,
	Active
};

/**
 * Structure for storing all adjustable Photo Mode parameters
 */
USTRUCT(BlueprintType)
struct FPhotoModeSettings
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ManualExposureBias = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAutoFocusEnabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ManualFocusDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FocusDistanceAdjustment = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CurrentFocalLength = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CurrentAperture = 0.0f;
};

/**
 * Component for activating, deactivating, and managing Photo Mode
 */
UCLASS(BlueprintType, Blueprintable)
class CITYSAMPLE_API UPhotoModeComponent : public UActorComponent
{
	GENERATED_BODY()
public:

	UPhotoModeComponent();

	//~ Begin ActorComponent Interface
	virtual void OnComponentCreated() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End ActorComponent Interface
	
	UFUNCTION(BlueprintPure)
	EPhotoModeState GetPhotoModeState() const
	{
		return State;
	}
	
	/** Initializes Photo Mode and attempts spawning and possession of a Photo Mode pawn */
	UFUNCTION(BlueprintCallable)
	bool ActivatePhotoMode();
	
	/** BP hook for when Photo Mode is activated */
	UFUNCTION(BlueprintImplementableEvent)
	void OnActivatePhotoMode();

	/** Deactivates Photo Mode and handles proper transition to previous gameplay state */
	UFUNCTION(BlueprintCallable)
	void DeactivatePhotoMode();

	/** BP hook for when Photo Mode is deactivated */
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeactivatePhotoMode();

	/** Adds input mapping that is relevant to Photo Mode */
	UFUNCTION(BlueprintCallable)
	void AddPhotomodeInputContext();

	/** Removes input mapping that is relevant to Photo Mode */
	UFUNCTION(BlueprintCallable)
	void RemovePhotomodeInputContext();

	/** Enables auto focus and initializes auto focus interpolator */
	UFUNCTION(BlueprintCallable)
	void EnableAutoFocus();

	/** BP hook for when auto focus is enabled */
	UFUNCTION(BlueprintImplementableEvent)
	void OnEnableAutoFocus();

	/** Disables auto focus, leaving the focus distance at whatever was set upon disable */
	UFUNCTION(BlueprintCallable)
	void DisableAutoFocus();

	/** BP hook for when auto focus is disabled */
	UFUNCTION(BlueprintImplementableEvent)
	void OnDisableAutoFocus();

	/** Saves current Photo Mode settings as the new default settings used for resets */
	UFUNCTION(BlueprintCallable)
	void SaveCurrentSettingsAsDefault();

	UFUNCTION(BlueprintCallable)
	void SetUseAutoFocus(bool bUseAutoFocus)
	{
		Settings.bAutoFocusEnabled = bUseAutoFocus;
	}

	UFUNCTION(BlueprintCallable)
	void SetCurrentFocalLength(float NewFocalLength)
	{
		Settings.CurrentFocalLength = NewFocalLength;
	}

	UFUNCTION(BlueprintCallable)
	void SetCurrentAperture(float NewAperture)
	{
		Settings.CurrentAperture = NewAperture;
	}

	UFUNCTION(BlueprintCallable)
	void SetFocusDistanceAdjustment(float NewFocusDistanceAdjustment)
	{
		Settings.FocusDistanceAdjustment = NewFocusDistanceAdjustment;
	}

	UFUNCTION(BlueprintCallable)
	void SetManualExposureBias(float NewManualExposureBias)
	{
		Settings.ManualExposureBias = NewManualExposureBias;
	}

	UFUNCTION(BlueprintCallable)
	void SetManualFocusDistance(float NewManualFocusDistance)
	{
		Settings.ManualFocusDistance = NewManualFocusDistance;
	}
	
	UFUNCTION(BlueprintPure)
	const FPhotoModeSettings& GetPhotoModeSettings() const
	{
		return Settings;
	}

	UFUNCTION(BlueprintPure)
	APawn* GetCachedPlayerPawn()
	{
		return OldPawn;
	}

	void SetGoalAutoFocusDistance(float NewGoalAutoFocusDistance) 
	{ 
		GoalAutoFocusDistance = NewGoalAutoFocusDistance; 
	}

	/** Called by this component's owner to bind input methods to their corresponding input actions */
	void SetUpInputs();

protected:

	/** BP hook for when the UI is hidden while in Photo Mode */
	UFUNCTION(BlueprintImplementableEvent)
	void OnHideUIToggle(bool bIsHiding);

	/** Pawn class to spawn and possess when activating Photo Mode */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ACitySampleHoverDrone> PhotomodePawnClass;

	/** Interpolator used to smoothly adjust auto focus distance values */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIIRInterpolatorFloat AutoFocusDistanceInterpolator = FIIRInterpolatorFloat(8.0f);

	/** Whether or not the Photo Mode pawn has moved far enough from its original position and orientation to warrant a fade transition */
	UFUNCTION(BlueprintPure, Category = "FadeOut")
	bool ShouldTriggerFadeOut() const;

	/** Photo Mode Pawn initial location when entering Photo Mode. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="FadeOut")
	FVector StartingLocation;

	/** Photo Mode Pawn initial rotation when entering Photo Mode. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FadeOut")
	FRotator StartingRotation;

	/** Photo Mode Pawn final location when exiting Photo Mode. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FadeOut")
	FVector EndingLocation;

	/** Photo Mode Pawn final rotation when exiting Photo Mode.*/
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FadeOut")
	FRotator EndingRotation;

	/** Translational Difference Threshold allowed before a Fade Out transition is warranted */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FadeOut")
	float FadeOutDistanceThreshold = 50.0f;

	/** Rotation Difference Threshold allowed before a Fade Out transition is warranted */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FadeOut")
	float FadeOutRotationThreshold = 30.0f;	

	/** Important to know since we don't want to trigger a fade transition if our previous pawn was a drone */
	bool bWasOldPawnADrone = false;

private:

	friend class UCitySampleCamera_PhotoMode;

	/** Final step of Photo Mode activation, called from our Camera Mode Instance where we can determine that Photo Mode has successfully activated. */
	void OnPhotoModeActivated_Internal(class UCitySampleCamera_PhotoMode* ActivatedPhotoMode);

	/** Updates our OldPawn variable so that we attempt to possess the proper pawn upon Photo Mode deactivation */
	UFUNCTION()
	void OnVehicleDriverExit(class UDrivableVehicleComponent* const DrivableComponent, class ACitySampleVehicleBase* const Vehicle, APawn* const Driver);

	/** Camera Mode to use for Photo Mode */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCitySampleCamera_PhotoMode> PhotoModeClass;

	/** Up to date Photo Mode settings */
	UPROPERTY(EditDefaultsOnly)
	FPhotoModeSettings Settings;

	/** Reference to pawn that is spawned and possessed when activating PhotoMode */
	UPROPERTY(Transient)
	class ACitySampleHoverDrone* CameraPawn = nullptr;

	/** Reference to HoverDroneMovementComponent that the Photo Mode pawn relies on for movement */
	UPROPERTY(Transient)
	class UHoverDroneMovementComponent* HoverDroneMovementComp = nullptr;

	/** Reference to pawn that was in possession prior to entering Photo Mode */
	UPROPERTY(Transient)
	class APawn* OldPawn = nullptr;

	/** Reference to player that requested Photo Mode activation */
	UPROPERTY(Transient)
	class ACitySamplePlayerController* RequestingPlayer = nullptr;

	/** Cached default Photo Mode settings for resets */
	UPROPERTY(Transient)
	FPhotoModeSettings DefaultSettings;

	/** Current Photo Mode State, keeps track of whether we're inactive, active, or in between */
	EPhotoModeState State = EPhotoModeState::NotActive;

	/** Auto focus distance that we interpolate to when using Auto Focus */
	float GoalAutoFocusDistance;

	/** Input Mapping Context and Input Actions that Photo Mode relies on */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* ActiveInputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ChangeAltitudeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* UseAutoFocusAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 InputMappingPriority = 1;

	/** Corresponds to the movement rate when using the left stick */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float HorizontalMovementRate = 1.0f;

	/** Corresponds to the movement rate when rising/falling with trigger inputs */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float VerticalMovementRate = 1.0f;

	/** Corresponds to camera vertical look rate*/
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float LookUpRate = 1.0f;

	/** Corresponds to camera horizontal look rate*/
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float TurnRate = 1.0f;

	/** Wrappers for pawn movement functions that check for a valid CameraPawn reference before proceeding */
	void MoveForward(float Val);

	void MoveRight(float Val);

	void MoveUp(float Val);

	void TurnAtRate(float Rate);

	void LookUpAtRate(float Rate);
	
	/** Methods binded to input actions to facilitate Photo Mode specific input */
	void MoveActionBinding(const struct FInputActionValue& ActionValue);
	void LookActionBinding(const struct FInputActionValue& ActionValue);
	void ChangeAltitudeActionBinding(const struct FInputActionValue& ActionValue);
	void EnableAutoFocusActionBinding();
	void DisableAutoFocusActionBinding();
};