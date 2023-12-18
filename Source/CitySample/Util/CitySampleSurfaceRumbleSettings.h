// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CitySampleSurfaceRumbleSettings.generated.h"

class UCurveFloat;

USTRUCT(BlueprintType)
struct FCitySampleSurfaceRumbleSettings
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
	UCurveFloat* SpeedToIntensityCurve = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bUseLargeMotors = false;

	UPROPERTY(BlueprintReadWrite)
	bool bUseSmallMotors = false;

	UPROPERTY(BlueprintReadWrite)
	float PostitiveIntensityDeviation = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float NegativeIntensityDeviation = 0.0f;
};
