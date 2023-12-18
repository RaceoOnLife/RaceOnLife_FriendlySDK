// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Game/ICitySampleTraversalInterface.h"
#include "UI/CitySampleControlsOverlayInterface.h"
#include "UI/CitySampleUIComponentInterface.h"
#include "Util/ICitySampleInputInterface.h"

#include "CitySampleCharacter.generated.h"

class UMotionWarpingComponent;

UCLASS()
class CITYSAMPLE_API ACitySampleCharacter :	public ACharacter,
									public ICitySampleTraversalInterface,
									public ICitySampleUIComponentInterface,
									public ICitySampleControlsOverlayInterface,
									public ICitySampleInputInterface
{
	GENERATED_BODY()

public:
	/** Sets default values for this character's properties */
	ACitySampleCharacter(const FObjectInitializer& ObjectInitializer);

	//~ Begin ICitySampleControlsOverlayInterface Interface
	virtual TMap<UInputAction*, FText> GetInputActionDescriptions_Implementation() const override
	{
		return InputActionDescriptions;
	};

	virtual TMap<FKey, FText> GetInputKeyDescriptionOverrides_Implementation() const override
	{
		return InputKeyDescriptionOverrides;
	};
	//~ End ICitySampleControlsOverlayInterface Interface

	//~ Begin ICitySampleTraversalInterface Interface
	virtual EPlayerTraversalState GetTraversalState_Implementation() const override
	{
		return EPlayerTraversalState::OnFoot;
	}
	//~ End ICitySampleTraversalInterface Interface

	//~ Begin ICitySampleInputInterface Interface
	virtual void AddInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface) override;
	virtual void RemoveInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface) override;
	//~ End ICitySampleInputInterface Interface

	UFUNCTION(BlueprintPure)
	bool GetIsSprinting() const
	{
		return bIsSprinting;
	}

	/** Sets sprinting flag and calls OnIsSprintingChanged BIE if valid */
	UFUNCTION(BlueprintCallable)
	void SetIsSprinting(bool bNewIsSprinting);

	UFUNCTION(BlueprintPure)
	float GetDesiredWalkSpeed() const
	{
		return DefaultWalkSpeed;
	}

	UFUNCTION(BlueprintPure)
	float GetDesiredSprintSpeed() const
	{
		return DefaultSprintSpeed;
	}

	UFUNCTION(BlueprintPure)
	float GetDesiredWalkMinInputSize() const
	{
		return DefaultWalkMinInputSize;
	}

	UFUNCTION(BlueprintPure)
	float GetDesiredSprintMinInputSize() const
	{
		return DefaultSprintMinInputSize;
	}

	/** Returns true if this character is driving a vehicle */
	UFUNCTION(BlueprintCallable)
	bool IsDriving() const;

protected:

	//~ Begin APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//~ End APawn interface

	/* Allow cheat flying to move vertically */
	bool ShouldLimitMovementPitch() const;

	/** BP hook for when the character's sprinting state changes */
	UFUNCTION(BlueprintImplementableEvent)
	void OnIsSprintingChanged(bool bNewIsSprinting);

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** Added to accommodate Gamepad 2D Axis IA support. Passes Value.X to MoveRight and Value.Y to MoveForward */
	void Move(const FVector2D& Value);

	/** Called via input to turn at a given rate. Uses a normalized rate. i.e. 1.0 means 100% of desired turn rate */
	void TurnAtRate(float Rate);

	/** Called via input to look up/down at a given rate. Uses a normalized rate. i.e. 1.0 means 100% of desired turn rate */
	void LookUpAtRate(float Rate);

	/** Added to accommodate Gamepad 2D Axis IA support. Passes Value.X to TurnAtRate and Value.Y to LookUpAtRate */
	void Look(const FVector2D& Value);

	/** Reference to owned motion warping component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	UMotionWarpingComponent* MotionWarpingComponent;

	/** Mapping of pawn relevant InputActions to their string description for the controls overlay UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls Overlay")
	TMap<UInputAction*, FText> InputActionDescriptions;

	/** Mapping of pawn relevant input keys to their string description for the controls overlay UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls Overlay")
	TMap<FKey, FText> InputKeyDescriptionOverrides;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;


private:

	/** Flag that indicates whether the character is currently sprinting */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, BlueprintSetter=SetIsSprinting, Meta=(AllowPrivateAccess="true"))
	bool bIsSprinting = false;

	/** Speed Character attempts to reach when sprinting */
	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"))
	float DefaultSprintSpeed = 450.f;

	/** Speed Character attempts to reach when walking */
	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"))
	float DefaultWalkSpeed = 170.f;

	/** Starting input acceleration size passed to the movement component when walking */
	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"))
	float DefaultWalkMinInputSize = 0.0f;

	/** Starting input acceleration size passed to the movement component when sprinting */
	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"))
	float DefaultSprintMinInputSize = 0.5f;

	/** Input Mapping and Input Actions that Characters depend on */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputMappingContext* LookControlsInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookDeltaAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	int32 InputMappingPriority = 1;

	/** Methods binded to character input actions to facilitate player input */
	void MoveActionBinding(const struct FInputActionValue& ActionValue);
	void LookActionBinding(const struct FInputActionValue& ActionValue);
	void LookDeltaActionBinding(const struct FInputActionValue& ActionValue);
};
