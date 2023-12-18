// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "CitySampleAnimInstance_Crowd_Head.generated.h"

class ACitySampleCrowdCharacter;
class USkeletalMeshComponent;

UCLASS()
class UCitySampleAnimInstance_Crowd_Head : public UAnimInstance
{
	GENERATED_BODY()

public:

	UCitySampleAnimInstance_Crowd_Head(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:

	UPROPERTY(Transient, BlueprintReadWrite, Category = Defaults)
	TObjectPtr<ACitySampleCrowdCharacter> CrowdCharacter;

	UPROPERTY(Transient, BlueprintReadWrite, Category = Defaults)
	TObjectPtr<USkeletalMeshComponent> SourceMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	FName CurveName_Overweight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	FName CurveName_Underweight;
};

