// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CitySampleVisualizationToggleType.generated.h"

/** Different Visualization Toggle Types we support*/
UENUM(BlueprintType)
enum class ECitySampleVisualizationToggleType : uint8
{
	None,
	Nanite,
	Mass,
	DayNight,
	TSR
};

