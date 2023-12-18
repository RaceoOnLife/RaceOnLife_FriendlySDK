// Copyright Epic Games, Inc. All Rights Reserved.

#include "LoadingUtilLibrary.h"
#include "Engine/CoreSettings.h"
#include "GameFramework/WorldSettings.h"
#include "MassSimulationSettings.h"
#include "Engine/Engine.h"

bool ULoadingUtilLibrary::HasCapturedDefaults = false;
float ULoadingUtilLibrary::DefaultLevelStreamingComponentsRegistrationGranularity;
float ULoadingUtilLibrary::DefaultLevelStreamingActorsUpdateTimeLimit;
float ULoadingUtilLibrary::DefaultAsyncLoadingTimeLimit;

double ULoadingUtilLibrary::DefaultMassActorDestructionTimeLimit;
double ULoadingUtilLibrary::DefaultMassActorSpawnTimeLimit;

void ULoadingUtilLibrary::ApplyDefaultPriorityLoading(const UObject* WorldContextObject)
{
	// Call first, just in case defaults have not been captured yet
	CaptureDefaultLoadingSettings();
	ApplyCustomPriorityLoading(WorldContextObject, false, DefaultAsyncLoadingTimeLimit, DefaultLevelStreamingActorsUpdateTimeLimit, DefaultMassActorDestructionTimeLimit, DefaultMassActorSpawnTimeLimit);
}

void ULoadingUtilLibrary::ApplyStreamingPriorityLoading(const UObject* WorldContextObject)
{
	CaptureDefaultLoadingSettings();
	ApplyCustomPriorityLoading(WorldContextObject, false, 10.0f, 10.0f, DefaultMassActorDestructionTimeLimit, DefaultMassActorSpawnTimeLimit);
}

void ULoadingUtilLibrary::ApplyHighestPriorityLoading(const UObject* WorldContextObject)
{
	CaptureDefaultLoadingSettings();
	ApplyCustomPriorityLoading(WorldContextObject, true, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
}

void ULoadingUtilLibrary::ApplyCustomPriorityLoading(const UObject* WorldContextObject, bool UseHighPriorityLoading, float MaxAsyncLoadingMilliSeconds, float MaxActorUpdateMilliSeconds, double MaxMassActorDestructionTimeLimit, double MaxMassActorSpawnTimeLimit)
{
	CaptureDefaultLoadingSettings();

	if (!ensure(WorldContextObject != nullptr))
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();

	if (!ensure(World != nullptr))
	{
		return;
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();

	if (!ensure(WorldSettings != nullptr))
	{
		return;
	}

	WorldSettings->bHighPriorityLoadingLocal = UseHighPriorityLoading;
	GLevelStreamingActorsUpdateTimeLimit = MaxActorUpdateMilliSeconds;
	GLevelStreamingComponentsRegistrationGranularity = DefaultLevelStreamingComponentsRegistrationGranularity;
	GAsyncLoadingUseFullTimeLimit = UseHighPriorityLoading;
	GAsyncLoadingTimeLimit = MaxAsyncLoadingMilliSeconds;

	GET_MASSSIMULATION_CONFIG_VALUE(DesiredActorDestructionTimeSlicePerTick) = MaxMassActorDestructionTimeLimit;
	GET_MASSSIMULATION_CONFIG_VALUE(DesiredActorSpawningTimeSlicePerTick) = MaxMassActorSpawnTimeLimit;
}

void ULoadingUtilLibrary::FlushLevelStreaming(const UObject* WorldContextObject)
{
	if (!ensure(WorldContextObject != nullptr))
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();

	if (!ensure(World != nullptr))
	{
		return;
	}

	GEngine->BlockTillLevelStreamingCompleted(World);
}

void ULoadingUtilLibrary::ForceGarbageCollection()
{
#if WITH_EDITOR
	GEngine->ForceGarbageCollection(false);
#else
	GEngine->ForceGarbageCollection(true);
#endif
}

void ULoadingUtilLibrary::CaptureDefaultLoadingSettings()
{
	if (!HasCapturedDefaults)
	{
		DefaultLevelStreamingComponentsRegistrationGranularity = GLevelStreamingComponentsRegistrationGranularity;
		DefaultLevelStreamingActorsUpdateTimeLimit = GLevelStreamingActorsUpdateTimeLimit;
		DefaultAsyncLoadingTimeLimit = GAsyncLoadingTimeLimit;

		DefaultMassActorDestructionTimeLimit = GET_MASSSIMULATION_CONFIG_VALUE(DesiredActorDestructionTimeSlicePerTick);
		DefaultMassActorSpawnTimeLimit = GET_MASSSIMULATION_CONFIG_VALUE(DesiredActorSpawningTimeSlicePerTick);
		HasCapturedDefaults = true;
	}
}
