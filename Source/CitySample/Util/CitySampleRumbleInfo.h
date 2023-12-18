// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CitySampleRumbleInfo.generated.h"

/**
 * Struct for storing generic rumble info and what specific motors in the gamepad will be affected
 */
USTRUCT(BlueprintType)
struct FCitySampleRumbleInfo
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
	float Duration = -1.0f;

	UPROPERTY(BlueprintReadWrite)
	float Intensity = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bUseRightLarge = false;

	UPROPERTY(BlueprintReadWrite)
	bool bUseRightSmall = false;

	UPROPERTY(BlueprintReadWrite)
	bool bUseLeftLarge = false;

	UPROPERTY(BlueprintReadWrite)
	bool bUseLeftSmall = false;
};
