// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "CitySampleAnimSet_Accessory.generated.h"

UCLASS()
class UCitySampleAnimSet_Accessory : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	TSubclassOf<UAnimInstance> AccessoryAnimGraphClass;

	UCitySampleAnimSet_Accessory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};

