// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "CitySampleAnimInstance_Accessory.generated.h"

UCLASS()
class UCitySampleAnimInstance_Accessory : public UAnimInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FName GaitCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	FName BlockReactCurveName;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	float Gait;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	float GaitLayerOverride;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	float BlockReact;

	UCitySampleAnimInstance_Accessory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};

