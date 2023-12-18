// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/BlockingVolume.h"
#include "GameFramework/GameModeBase.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MotionWarpingComponent.h"

#include "Character/CitySampleCharacterMovementComponent.h"
#include "CitySample.h"
#include "Game/CitySamplePlayerController.h"
#include "Util/CitySampleBlueprintLibrary.h"
#include "Vehicles/CitySampleVehicleBase.h"


bool ACitySampleCharacter::IsDriving() const
{
	if (ACitySampleVehicleBase* const ParentVehicle = Cast<ACitySampleVehicleBase>(GetAttachParentActor()))
	{
		return (ParentVehicle->GetOccupantInSeat(ECitySampleVehicleSeat::Driver) == this);
	}

	return false;
}

ACitySampleCharacter::ACitySampleCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCitySampleCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	MotionWarpingComponent->bSearchForWindowsInAnimsWithinMontages = true;
}

void ACitySampleCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	if (UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::MoveActionBinding);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::LookActionBinding);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &ThisClass::LookActionBinding);
		EnhancedInputComponent->BindAction(LookDeltaAction, ETriggerEvent::Triggered, this, &ThisClass::LookDeltaActionBinding);
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("Failed to setup player input for %s, InputComponent type is not UEnhancedInputComponent."), *GetName());
	}
}

bool ACitySampleCharacter::ShouldLimitMovementPitch() const
{
	return GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling();
}

void ACitySampleCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const bool bLimitRotation = ShouldLimitMovementPitch();
		FRotator Rotation = Controller->GetControlRotation();
		if (bLimitRotation)
		{
			Rotation.Pitch = Rotation.Roll = 0.0f;
		}

		// get forward vector
		const FVector Direction = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ACitySampleCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ACitySampleCharacter::Move(const FVector2D& Value)
{
	MoveForward(Value.X);
	MoveRight(Value.Y);
}

void ACitySampleCharacter::Look(const FVector2D& Value)
{
	TurnAtRate(Value.X);
	LookUpAtRate(Value.Y);
}

void ACitySampleCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ACitySampleCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ACitySampleCharacter::SetIsSprinting(bool bNewIsSprinting)
{
	if (bNewIsSprinting != bIsSprinting)
	{
		bIsSprinting = bNewIsSprinting;
		OnIsSprintingChanged(bNewIsSprinting);
	}
}

void ACitySampleCharacter::AddInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface)
{
	SubsystemInterface->AddMappingContext(InputMappingContext, InputMappingPriority);
	SubsystemInterface->AddMappingContext(LookControlsInputMappingContext, InputMappingPriority);
}

void ACitySampleCharacter::RemoveInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface)
{
	SetIsSprinting(false);
	SubsystemInterface->RemoveMappingContext(InputMappingContext);
	SubsystemInterface->RemoveMappingContext(LookControlsInputMappingContext);
}

void ACitySampleCharacter::MoveActionBinding(const struct FInputActionValue& ActionValue)
{
	Move(ActionValue.Get<FInputActionValue::Axis2D>());
}

void ACitySampleCharacter::LookActionBinding(const struct FInputActionValue& ActionValue)
{
	Look(ActionValue.Get<FInputActionValue::Axis2D>());
}

void ACitySampleCharacter::LookDeltaActionBinding(const struct FInputActionValue& ActionValue)
{
	const FInputActionValue::Axis2D AxisValue = ActionValue.Get<FInputActionValue::Axis2D>();
	APawn::AddControllerYawInput(AxisValue.X);
	APawn::AddControllerPitchInput(AxisValue.Y);
}

