// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "CitySampleAnimSet_Locomotion.generated.h"

class UAnimSequence;

UCLASS()
class UCitySampleAnimSet_Locomotion : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UCitySampleAnimSet_Locomotion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core Locomotion")
	TObjectPtr<UAnimSequence> WalkFwdCycle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core Locomotion")
	TObjectPtr<UAnimSequence> WalkFwdQuicklyCycle;
};

