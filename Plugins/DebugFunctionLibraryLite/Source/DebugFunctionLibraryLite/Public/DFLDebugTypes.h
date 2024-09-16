// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "DebugFunctionLibrarySettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DFLDebugTypes.generated.h"

UENUM(BlueprintType)
enum class EDFLOutputMethod : uint8
{
	PrintOutputLog UMETA(DisplayName = "Print & Output Log"),
	Print,
	OutputLog,
	None
};
