// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "CitySampleConvertedISMActor.generated.h"

UCLASS(notplaceable, notblueprintable)
class CITYSAMPLE_API ACitySampleConvertedISMActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = ConvertedFrom)
	TSoftObjectPtr<UBlueprint> BlueprintAsset;

	UPROPERTY(VisibleAnywhere, Category = ConvertedFrom)
	TSoftObjectPtr<UWorld> WorldAsset;
#endif
};