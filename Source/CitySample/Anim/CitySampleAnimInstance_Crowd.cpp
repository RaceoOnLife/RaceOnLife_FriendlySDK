// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimInstance_Crowd.h"
#include "Crowd/CrowdCharacterActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "KismetAnimationLibrary.h"
#include "DrawDebugHelpers.h"
#include "DrawDebugHelpers.h"
#include "CitySampleAnimSet_Locomotion.h"
#include "CitySampleAnimSet_Accessory.h"

UCitySampleAnimInstance_Crowd::UCitySampleAnimInstance_Crowd(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WalkSpeedThreshold = 1.f;
	TimeInLocomotionStateThreshold = 0.15f;
	PlayRate = 1.f;
}

void UCitySampleAnimInstance_Crowd::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_NativeInitializeAnimation);

	// Set References and DefaultValues

	Character = Cast<ACitySampleCrowdCharacter>(TryGetPawnOwner());

	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();

		RandomAimPitchVariation = FMath::FRandRange(-20.f, 10.f);

		AnimSetLocomotion = Cast<UCitySampleAnimSet_Locomotion>(Character->GetCurrentLocomotionAnimSet());

		MassCrowdAnimInstanceData = Character->SpawnAnimData;

		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCitySampleAnimInstance_Crowd::LinkAccessoryLayer);
	}
}

void UCitySampleAnimInstance_Crowd::LinkAccessoryLayer()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_LinkAccessoryLayer);

	if(Character)
	{
		if(const UCitySampleAnimSet_Accessory* AnimSet = Cast<UCitySampleAnimSet_Accessory>(Character->GetCurrentAccessoryAnimSet()))
		{
			LinkAnimClassLayers(AnimSet->AccessoryAnimGraphClass);
		}
	}
}

void UCitySampleAnimInstance_Crowd::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_NativeUpdateAnimation);

	if(Character && MovementComponent)
	{
		// Set Essential Data

		Velocity = MovementComponent->Velocity;

		Speed = Velocity.Size2D();

		MaxSpeed = MovementComponent->GetMaxSpeed();

		CharacterRotation = Character->GetActorRotation();

		if (!Character->IsPlayerControlled())
		{
			const FRotator Rotator = MassCrowdAnimInstanceData.LookAtDirection.ToOrientationRotator();

			LookAtRotation.Roll = 0.f;
			LookAtRotation.Pitch = RandomAimPitchVariation + Rotator.Pitch;
			LookAtRotation.Yaw = Rotator.Yaw;
		}

		// Set Locomotion State

		ECitySampleLocomotionState LastLocomotionState = LocomotionState;

		if (Character->HasAnyRootMotion())
		{
			LocomotionState = ECitySampleLocomotionState::Idle;
		}
		else
		{
			TimeInLocomotionState += DeltaSeconds;

			if (TimeInLocomotionState > TimeInLocomotionStateThreshold)
			{
				// Check movement data to determine desired locomotion state.
				if (Speed > WalkSpeedThreshold)
				{
					LocomotionState = ECitySampleLocomotionState::Walk;
				}
				else
				{
					LocomotionState = ECitySampleLocomotionState::Idle;
				}
			}
		}

		if((LastLocomotionState != ECitySampleLocomotionState::Idle && LocomotionState == ECitySampleLocomotionState::Idle) || 
		   (LastLocomotionState != ECitySampleLocomotionState::Walk && LocomotionState == ECitySampleLocomotionState::Walk))
		{
			TimeInLocomotionState = 0.f;
		}
	}

	PlayRate = CalculatePlayRate();
	CycleDirection = CalculateCycleDirection();
	Stride = CalculateStride();
	Gait = GetGait();
	BasePoseMTN = Get_BasePose_MTN();
	BasePoseFTN = Get_BasePose_FTN();
	BasePoseFTU = Get_BasePose_FTU();
	BasePoseFTO = Get_BasePose_FTO();
	BasePoseMTO = Get_BasePose_MTO();
	BasePoseMTU = Get_BasePose_MTU();
}

void UCitySampleAnimInstance_Crowd::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
	// Leaving this here to find crowd's post evals easily in perf captures
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_NativePostEvaluateAnimation);
}

float UCitySampleAnimInstance_Crowd::CalculatePlayRate() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_CalculatePlayRate);

	static const FName CurveName = FName(TEXT("MoveData_Speed"));
	const float CurveValue = FMath::Clamp(GetCurveValue(CurveName), 20.f, 1000.f);
	const float A = (CurveValue != 0.f) ? (Speed / CurveValue) : 0.f;
	const float B = GetOwningComponent()->GetComponentScale().X;
	return FMath::Clamp(((B != 0.f) ? (A / B) : 0.f), 0.8f, 2.0f);
}

float UCitySampleAnimInstance_Crowd::CalculateCycleDirection() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_CalculateCycleDirection);

	return UKismetAnimationLibrary::CalculateDirection(Velocity, CharacterRotation);
}

float UCitySampleAnimInstance_Crowd::CalculateStride() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_CalculateStride);

	return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 100.f), FVector2D(0.f, 1.f), Speed);
}

void UCitySampleAnimInstance_Crowd::GetMassMoveState(EMassMovementAction& OutCurrentMovementAction, EMassMovementAction& OutPreviousMovementAction) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_GetMassMoveState);

	OutCurrentMovementAction = EMassMovementAction::Stand;
	OutPreviousMovementAction = EMassMovementAction::Stand;

	if(Character && Character->IsPlayerControlled())
	{
		if(Speed > WalkSpeedThreshold)
		{
			OutCurrentMovementAction = EMassMovementAction::Move;
			OutPreviousMovementAction = EMassMovementAction::Move;
		}
	}
}

float UCitySampleAnimInstance_Crowd::GetGait() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_GetGait);

	static const FName CurveName = FName(TEXT("MoveData_Gait"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}

float UCitySampleAnimInstance_Crowd::Get_BasePose_MTN() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Get_BasePose_MTN);

	static const FName CurveName = FName(TEXT("LayerData_BasePose_MTN"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}

float UCitySampleAnimInstance_Crowd::Get_BasePose_FTN() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Get_BasePose_FTN);

	static const FName CurveName = FName(TEXT("LayerData_BasePose_FTN"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}

float UCitySampleAnimInstance_Crowd::Get_BasePose_FTU() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Get_BasePose_FTU);

	static const FName CurveName = FName(TEXT("LayerData_BasePose_FTU"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}

float UCitySampleAnimInstance_Crowd::Get_BasePose_FTO() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Get_BasePose_FTO);

	static const FName CurveName = FName(TEXT("LayerData_BasePose_FTO"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}

float UCitySampleAnimInstance_Crowd::Get_BasePose_MTO() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Get_BasePose_MTO);

	static const FName CurveName = FName(TEXT("LayerData_BasePose_MTO"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}

float UCitySampleAnimInstance_Crowd::Get_BasePose_MTU() const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Get_BasePose_MTU);

	static const FName CurveName = FName(TEXT("LayerData_BasePose_MTU"));
	return FMath::Clamp(GetCurveValue(CurveName), 0.f, 1.f);
}
