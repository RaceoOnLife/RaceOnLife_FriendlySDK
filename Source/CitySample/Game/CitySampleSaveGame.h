// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CitySampleSaveGame.generated.h"

/**
 * Save game class for CitySample
 */
UCLASS()
class CITYSAMPLE_API UCitySampleSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool bVerticalAxisInverted = false;
	
	UPROPERTY(BlueprintReadOnly, Category="Settings")
	FVector LookSensivity = { 1.0f, 1.0f, 1.0f };

	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool bForceFeedbackEnabled = true;
};
