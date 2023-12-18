// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleBlueprintLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "Camera/CameraComponent.h"
#include "ConvexVolume.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Engine/CoreSettings.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManagerGeneric.h"
#include "IDeviceProfileSelectorModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsInterfaceDeclaresCore.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"

#include "Chaos/ISpatialAccelerationCollection.h"
#include "Chaos/ParticleHandle.h"
#include "ChaosVehicleMovementComponent.h"
#include "ZoneGraphSettings.h"
#include "ZoneGraphSubsystem.h"

#include "Camera/CitySamplePlayerCameraManager.h"
#include "CitySample.h"
#include "Character/CitySampleCharacter.h"
#include "Game/CitySampleGameMode.h"
#include "Game/CitySampleGameState.h"
#include "Game/CitySamplePlayerController.h"
#include "Game/CitySampleWorldInfo.h"
#include "Util/CitySampleInterpolators.h"


FString UCitySampleBlueprintLibrary::GetVersionString()
{
	FString ConfigString = LexToString(FApp::GetBuildConfiguration());

	uint32 ChangelistNumber = FEngineVersion::Current().GetChangelist();
	FString VersionString = FString::Printf(TEXT("CL: %d    Config: %s"), ChangelistNumber, *ConfigString);

	return VersionString;
}

void UCitySampleBlueprintLibrary::LogCitySampleDebugMessage(const FString& Message)
{
	UE_LOG(LogCitySample, Display, TEXT("%s"), *Message);
}

void UCitySampleBlueprintLibrary::LogCSVEvent(const FString EventName)
{
	UE_LOG(LogCitySample, Display, TEXT("Logging Custom Event %s to CSV."), *EventName);
	CSV_EVENT_GLOBAL(TEXT("%s"), *EventName);
}

FString UCitySampleBlueprintLibrary::GetActiveDeviceProfileName()
{
	return UDeviceProfileManager::Get().GetActiveDeviceProfileName();
}

FString UCitySampleBlueprintLibrary::GetBaseDeviceProfileName()
{
	// Set the device name to the platform name as a fallback
	FString DeviceName = FPlatformProperties::PlatformName();

	// Attempt to get the specific device name from the runtime device profile selector
	FString DeviceProfileSelectionModule;
	if (GConfig->GetString(TEXT("DeviceProfileManager"), TEXT("DeviceProfileSelectionModule"), DeviceProfileSelectionModule, GEngineIni))
	{
		if (IDeviceProfileSelectorModule* DPSelectorModule = FModuleManager::LoadModulePtr<IDeviceProfileSelectorModule>(*DeviceProfileSelectionModule))
		{
			DeviceName = DPSelectorModule->GetRuntimeDeviceProfileName();
		}
	}

	return DeviceName;
}

void UCitySampleBlueprintLibrary::OverrideDeviceProfileForMode(EDeviceProfileOverrideMode NewMode)
{
	// Sanitize Enum
	FString EnumString = StaticEnum<EDeviceProfileOverrideMode>()->GetValueAsString(NewMode);
	int32 ChopLoc = 0;
	EnumString.FindLastChar(TEXT(':'), ChopLoc);
	EnumString = EnumString.RightChop(ChopLoc + 1);

	// Test if currently in this mode, if so, do nothing
	if (GetActiveDeviceProfileName().EndsWith(EnumString))
	{
		UE_LOG(LogCitySample, Display, TEXT("%s is already active"), *GetActiveDeviceProfileName());
		return;
	}

	// Create Profile String
	FString DeviceProfileName = GetBaseDeviceProfileName();
	DeviceProfileName.Append(TEXT("_"));
	DeviceProfileName.Append(EnumString);

	if (UDeviceProfile* OverrideDeviceProfile = UDeviceProfileManager::Get().FindProfile(DeviceProfileName, true))
	{
		UDeviceProfileManager::Get().SetOverrideDeviceProfile(OverrideDeviceProfile);
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("Cannot Find Device Profile Named %s"), *DeviceProfileName);
	}
}

void UCitySampleBlueprintLibrary::RestoreDefaultDeviceProfile()
{
	UDeviceProfileManager::Get().RestoreDefaultDeviceProfile();
}

static int32 GDefaultLoadingRangeMainGrid = 12800;
static FAutoConsoleVariableRef CVarGDefaultLoadingRangeMainGrid(
	TEXT("CitySample.DefaultLoadingRangeMainGrid"),
	GDefaultLoadingRangeMainGrid,
	TEXT("CitySample's default loading range for the main grid."),
	ECVF_Default
);

static int32 GDefaultLoadingRangeHLOD0 = 76800;
static FAutoConsoleVariableRef CVarGDefaultLoadingRangeHLOD0(
	TEXT("CitySample.DefaultLoadingRangeHLOD0"),
	GDefaultLoadingRangeHLOD0,
	TEXT("CitySample's default loading range for the HLOD0 grid."),
	ECVF_Default
);

static int32 GDefaultHLODWarmupEnabled = 1;
static FAutoConsoleVariableRef CVarGDefaultHLODWarmupEnabled(
	TEXT("CitySample.DefaultHLODWarmupEnabled"),
	GDefaultHLODWarmupEnabled,
	TEXT("Enable HLOD warmup."),
	ECVF_Default
);

static void ApplyDefaultLoadingSettings(UWorld* World)
{
	if (World)
	{
		GLevelStreamingContinuouslyIncrementalGCWhileLevelsPendingPurge = 64;
		GEngine->Exec(World, *FString::Printf(TEXT("wp.Runtime.OverrideRuntimeSpatialHashLoadingRange -grid=0 -range=%d"), GDefaultLoadingRangeMainGrid));
		GEngine->Exec(World, *FString::Printf(TEXT("wp.Runtime.OverrideRuntimeSpatialHashLoadingRange -grid=1 -range=%d"), GDefaultLoadingRangeHLOD0));
		GEngine->Exec(World, *FString::Printf(TEXT("wp.Runtime.HLOD.WarmupEnabled %d"), GDefaultHLODWarmupEnabled));
	}
}

static FAutoConsoleCommandWithWorld CmdApplyDefaultLoadingSettings(
	TEXT("CitySample.ApplyDefaultLoadingSettings"),
	TEXT(""),
	FConsoleCommandWithWorldDelegate::CreateStatic(ApplyDefaultLoadingSettings));

void UCitySampleBlueprintLibrary::ApplyDefaultLoadingSettings(UObject* WorldContextObject)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		::ApplyDefaultLoadingSettings(World);
	}
}

ACitySamplePlayerController* UCitySampleBlueprintLibrary::GetCitySamplePlayerController(const UObject* WorldContextObject)
{
	return Cast<ACitySamplePlayerController>(UGameplayStatics::GetPlayerController(WorldContextObject, 0));
}

ACitySampleCharacter* UCitySampleBlueprintLibrary::GetCitySamplePlayerCharacter(const UObject* WorldContextObject)
{
	return Cast<ACitySampleCharacter>(UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0));
}

ACitySampleGameMode* UCitySampleBlueprintLibrary::GetCitySampleGameMode(const UObject* WorldContextObject)
{
	return Cast<ACitySampleGameMode>(UGameplayStatics::GetGameMode(WorldContextObject));
}

ACitySampleGameState* UCitySampleBlueprintLibrary::GetCitySampleGameState(const UObject* WorldContextObject)
{
	return Cast<ACitySampleGameState>(UGameplayStatics::GetGameState(WorldContextObject));
}

ACitySamplePlayerController* UCitySampleBlueprintLibrary::IsPlayerOfTraversalType(const AActor* InActor, const EPlayerTraversalState State)
{
	if (const APawn* Pawn = Cast<APawn>(InActor))
	{
		if (ACitySamplePlayerController* CitySampleController = Cast<ACitySamplePlayerController>(Pawn->GetController()))
		{
			return CitySampleController->CurrentTraversalState == State ? CitySampleController : nullptr;
		}
	}
	return nullptr;
}

ACitySampleWorldInfo* UCitySampleBlueprintLibrary::GetWorldInfo(const UObject* const WorldContextObject)
{
	if (UWorld* const World = WorldContextObject->GetWorld())
	{
		for (TActorIterator<ACitySampleWorldInfo> It(World, ACitySampleWorldInfo::StaticClass()); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}

bool UCitySampleBlueprintLibrary::IsMassVisualizationEnabled()
{
	const IConsoleVariable* const CVarDisplayEntities = IConsoleManager::Get().FindConsoleVariable(TEXT("CitySample.DisplayEntities"));
	return CVarDisplayEntities && CVarDisplayEntities->GetBool();
}

static const FZoneGraphTag* GetZoneGraphTagForName(FName Name, const UZoneGraphSettings& Settings)
{
	if (Name != NAME_None)
	{
		for (const FZoneGraphTagInfo& TagInfo : Settings.GetTagInfos())
		{
			if (TagInfo.Name == Name)
			{
				return &TagInfo.Tag;
			}
		}
	}

	return nullptr;
}

static TArray<FZoneGraphTag> GetZoneGraphTagsForNames(const TArray<FName>& Names)
{
	TArray<FZoneGraphTag> OutTags;
	if (const UZoneGraphSettings* ZoneGraphSettings = GetDefault<UZoneGraphSettings>())
	{
		OutTags.Reserve(Names.Num());
		for (FName Name : Names)
		{
			if (FZoneGraphTag const * const FoundTag = GetZoneGraphTagForName(Name, *ZoneGraphSettings))
			{
				OutTags.Add(*FoundTag);
			}
		}
	}

	return OutTags;
}

bool UCitySampleBlueprintLibrary::FindNearestLaneLocationByName(UObject* WorldContextObject, FTransform InPoint, FName LaneName, FTransform& OutPoint, float Radius)
{
	TArray<FZoneGraphTag> Tags;
	if (FZoneGraphTag const * const Tag = GetZoneGraphTagForName(LaneName, *GetDefault<UZoneGraphSettings>()))
	{
		Tags.Add(*Tag);
	}

	return FindNearestLaneLocationByTags(WorldContextObject, InPoint, Tags, OutPoint, Radius);
}

bool UCitySampleBlueprintLibrary::FindNearestLaneLocationByTag(UObject* WorldContextObject, FTransform InPoint, FZoneGraphTag Tag, FTransform& OutPoint, float Radius)
{
	TArray<FZoneGraphTag> Tags;
	if (FZoneGraphTag::None != Tag)
	{
		Tags.Add(Tag);
	}

	return FindNearestLaneLocationByTags(WorldContextObject, InPoint, Tags, OutPoint, Radius);
}

bool UCitySampleBlueprintLibrary::FindNearestLaneLocationByNames(UObject* WorldContextObject, FTransform InPoint, const TArray<FName>& LaneNames, FTransform& OutPoint, float Radius)
{
	return FindNearestLaneLocationByTags(WorldContextObject, InPoint, GetZoneGraphTagsForNames(LaneNames), OutPoint, Radius);
}

bool UCitySampleBlueprintLibrary::FindNearestLaneLocationByTags(UObject* WorldContextObject, FTransform InPoint, const TArray<FZoneGraphTag>& Tags, FTransform& OutPoint, float Radius)
{
	Radius = FMath::Max<>(Radius, 150.f);
	
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (UZoneGraphSubsystem* ZoneGraphSubsystem = UWorld::GetSubsystem<UZoneGraphSubsystem>(World))
	{
		const FVector MinBounds = InPoint.GetLocation() + FVector(-Radius);
		const FVector MaxBounds = InPoint.GetLocation() + FVector(Radius);

		FZoneGraphTagFilter Filter;
		if (Tags.Num() > 0)
		{
			Filter.AnyTags = FZoneGraphTagMask::None;
			for (FZoneGraphTag IncludeTag : Tags)
			{
				Filter.AnyTags.Add(IncludeTag);
			}
		}

		const FBox QueryBounds(MinBounds, MaxBounds);
		FZoneGraphLaneLocation LaneLocation;
		float DistanceSqr;
		
		if (ZoneGraphSubsystem->FindNearestLane(QueryBounds, Filter, LaneLocation, DistanceSqr))
		{
			const FQuat Rotation = FRotationMatrix::MakeFromXZ(LaneLocation.Direction, LaneLocation.Up).ToQuat();
			OutPoint = FTransform(Rotation, LaneLocation.Position);
			return true;
		}
	}

	return false;
}

float UCitySampleBlueprintLibrary::EvalAccelInterpolatorFloat(FAccelerationInterpolatorFloat& Interpolator, float NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

FVector UCitySampleBlueprintLibrary::EvalAccelInterpolatorVector(FAccelerationInterpolatorVector& Interpolator, FVector NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

FRotator UCitySampleBlueprintLibrary::EvalAccelInterpolatorRotator(FAccelerationInterpolatorRotator& Interpolator, FRotator NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

float UCitySampleBlueprintLibrary::EvalIIRInterpolatorFloat(FIIRInterpolatorFloat& Interpolator, float NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

FVector UCitySampleBlueprintLibrary::EvalIIRInterpolatorVector(FIIRInterpolatorVector& Interpolator, FVector NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

FRotator UCitySampleBlueprintLibrary::EvaIIRInterpolatorRotator(FIIRInterpolatorRotator& Interpolator, FRotator NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

FVector UCitySampleBlueprintLibrary::EvalDoubleIIRInterpolatorVector(FDoubleIIRInterpolatorVector& Interpolator, FVector NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}

FRotator UCitySampleBlueprintLibrary::EvalDoubleIIRInterpolatorRotator(FIIRInterpolatorRotator& Interpolator, FRotator NewGoal, float DeltaTime)
{
	return Interpolator.Eval(NewGoal, DeltaTime);
}