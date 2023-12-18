// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/CitySampleGameMode.h"

#include "CitySample.h"
#include "Game/CitySampleGameState.h"
#include "Game/CitySamplePlayerController.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"

static TAutoConsoleVariable<bool> CVarPerformanceMode(
	TEXT("CitySample.PerformanceMode"),
	false,
	TEXT("When true, certain portions of the sample are adjusted to be more friendly to lower spec hardware"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarLogSyncLoads(
	TEXT("CitySample.LogSyncLoads"),
	true,
	TEXT("Controls whether sync loads are logged out"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarUseSandboxIntro(
	TEXT("CitySample.UseSandboxIntro"),
	true,
	TEXT("Whether or not to use the sandbox intro level sequence upon entering the city sample. 0 = disable, 1 = enable. Completely overriden by the '-DisableSandboxIntro' startup argument"),
	ECVF_Cheat
);

ACitySampleGameMode::ACitySampleGameMode()
{
	GameStateClass = ACitySampleGameState::StaticClass();
	PlayerControllerClass = ACitySamplePlayerController::StaticClass();
}

void ACitySampleGameMode::BeginPlay()
{
	Super::BeginPlay(); 
	
	OnSyncLoadPackageHandle = FCoreDelegates::OnSyncLoadPackage.AddUObject(this, &ACitySampleGameMode::OnSyncLoadPackage);

	// Disable data layers deemed not performance friendly when performance mode is enabled
	if (CVarPerformanceMode.GetValueOnGameThread())
	{
		if (UWorld* World = GetWorld())
		{
			if (UDataLayerSubsystem* DataLayerSubsystem = World->GetSubsystem<UDataLayerSubsystem>())
			{	
				for (const FName& DataLayerToDisable : DataLayersToDisableInPerformanceMode)
				{
					PRAGMA_DISABLE_DEPRECATION_WARNINGS
					DataLayerSubsystem->SetDataLayerRuntimeStateByLabel(DataLayerToDisable, EDataLayerRuntimeState::Unloaded);
					PRAGMA_ENABLE_DEPRECATION_WARNINGS
				}
			}
		}
	}
}

void ACitySampleGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	FCoreDelegates::OnSyncLoadPackage.Remove(OnSyncLoadPackageHandle);
}

void ACitySampleGameMode::OnSyncLoadPackage(const FString& PackageName)
{
	if (CVarLogSyncLoads->GetBool())
	{
		UE_LOG(LogCitySample, Log, TEXT("CitySample sync loading package: %s"), *PackageName);
	}
}

bool ACitySampleGameMode::UseSandboxIntro() const
{
#if !UE_BUILD_SHIPPING
	//We need a start up argument for builds since commands queued by -execcmds gets executed after this is normally called
	//If the game mode is explicitly set not to use the sandbox intro the Cvar is ignored as gameflow would get stuck waiting for a level sequence that might not exist
	if (FParse::Param(FCommandLine::Get(), TEXT("DisableSandboxIntro")) || !bUseSandboxIntro)
	{
		return false;
	}

	return CVarUseSandboxIntro.GetValueOnGameThread();
#endif

	return bUseSandboxIntro;
}
