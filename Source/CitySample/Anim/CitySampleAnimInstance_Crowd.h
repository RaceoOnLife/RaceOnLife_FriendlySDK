// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/MassCrowdAnimInstance.h"
#include "CitySampleAnimInstance_Crowd.generated.h"

class ACitySampleCrowdCharacter;
class UCharacterMovementComponent;
class UCitySampleAnimSet_Locomotion;

UENUM(BlueprintType)
enum class ECitySampleLocomotionState : uint8
{
	Idle, 
	Walk, 
	Jog, 
	Animating
};

UCLASS()
class UCitySampleAnimInstance_Crowd : public UMassCrowdAnimInstance
{
	GENERATED_BODY()

public:

	UCitySampleAnimInstance_Crowd(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

protected:

	UPROPERTY(Transient, BlueprintReadWrite, Category = "References")
	TObjectPtr<ACitySampleCrowdCharacter> Character;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "References")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Essential Data")
	FVector Velocity;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Essential Data")
	float Speed;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Essential Data")
	float MaxSpeed;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Essential Data")
	FRotator CharacterRotation;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Essential Data")
	FRotator LookAtRotation;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Essential Data")
	float RandomAimPitchVariation;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	ECitySampleLocomotionState LocomotionState;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float TimeInLocomotionState;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float PlayRate;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float CycleDirection;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float Stride;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float Gait;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float BasePoseMTN;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float BasePoseFTN;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float BasePoseFTU;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float BasePoseFTO;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float BasePoseMTO;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float BasePoseMTU;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Data")
	TObjectPtr<UCitySampleAnimSet_Locomotion> AnimSetLocomotion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Data")
	float WalkSpeedThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim Data")
	float TimeInLocomotionStateThreshold;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float CalculatePlayRate() const;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float CalculateCycleDirection() const;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float CalculateStride() const;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	void GetMassMoveState(EMassMovementAction& OutCurrentMovementAction, EMassMovementAction& OutPreviousMovementAction) const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float GetGait() const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float Get_BasePose_MTN() const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float Get_BasePose_FTN() const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float Get_BasePose_FTU() const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float Get_BasePose_FTO() const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float Get_BasePose_MTO() const;

	UFUNCTION(BlueprintPure, Category = "Layering")
	float Get_BasePose_MTU() const;

	UFUNCTION()
	void LinkAccessoryLayer();
};

