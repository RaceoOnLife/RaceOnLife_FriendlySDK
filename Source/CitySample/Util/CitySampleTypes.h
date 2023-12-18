// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Chaos/ChaosEngineInterface.h"
#include "CitySampleTypes.generated.h"

DECLARE_STATS_GROUP(TEXT("CitySample"), STATGROUP_CitySample, STATCAT_Advanced);

#define CitySampleECC_InteractionsTrace			ECollisionChannel::ECC_GameTraceChannel3
#define CitySampleColisionProfile_Interaction	FName(TEXT("CitySampleInteraction"))

/** For runtime switching of sub device profiles to handle large swaths of cvar changes */
UENUM(BlueprintType)
enum class EDeviceProfileOverrideMode : uint8
{
	DroneMode,
	DrivingMode
};

UENUM(BlueprintType)
enum class EPlayerTraversalState : uint8
{
	OnFoot,
	InVehicle,
	Drone
};
