// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAssetManager.h"
#include "Misc/CommandLine.h"

// We don't allow preloading in the Editor.
static constexpr int32 PreloadInEditor_Off = 0;

// Preload in the Editor during startup.
static constexpr int32 PreloadInEditor_Startup = 1;
static constexpr int32 PreloadInEditor_PIE = 2;
static TAutoConsoleVariable<int32> CVarPreloadInEditor(
	TEXT("CitySample.PreloadInEditor"),
	PreloadInEditor_PIE,
	TEXT("Whether to preload CitySample assets in editor. 0 = Off, 1 = During Startup, 2 = Before PIE"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarPreloadCrowdCharacterData(
	TEXT("CitySample.PreloadCrowdCharacterData"),
	true,
	TEXT("Whether to preload crowd character data assets"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarPreloadAnimToTextureData(
	TEXT("CitySample.PreloadAnimToTextureData"),
	true,
	TEXT("Whether to preload anim to texture data assets"),
	ECVF_Default);

const FName UCitySampleAssetManager::LoadStateClient = FName(TEXT("Client"));

UCitySampleAssetManager::UCitySampleAssetManager()
{

}

void UCitySampleAssetManager::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		PlatformBundleState.Reset();
		PlatformBundleState.Add(LoadStateClient);

		DefaultBundleState = PlatformBundleState;
	}
}

TSharedPtr<FStreamableHandle> UCitySampleAssetManager::PreloadItemDefinitions()
{
#if WITH_EDITOR
	switch (CVarPreloadInEditor->GetInt())
	{
	case PreloadInEditor_Off:
		if (GIsEditor && !IsRunningCommandlet())
		{
			return nullptr;
		}
		break;

	case PreloadInEditor_PIE:
		if (bInPreBeginPIE)
		{
			if (bAlreadyLoadedForPIE)
			{
				return nullptr;
			}

			bAlreadyLoadedForPIE = true;
		}
		// We could rearrange the switch and fall through, but this seems cleaner
		// and less error prone.
		else if (GIsEditor && !IsRunningCommandlet())
		{
			return nullptr;
		}
		break;

	case PreloadInEditor_Startup:
	default:
		break;
	}
#endif

	TArray<FPrimaryAssetId> AssetIdListWithBundle;
	TArray<FPrimaryAssetId> AssetIdListWithoutBundle;
	
	if (CVarPreloadCrowdCharacterData->GetBool())
	{
		GetPrimaryAssetIdList(FName(TEXT("CrowdCharacterDataAsset")), AssetIdListWithBundle, EAssetManagerFilter::UnloadedOnly);
	}
	else
	{
		GetPrimaryAssetIdList(FName(TEXT("CrowdCharacterDataAsset")), AssetIdListWithoutBundle, EAssetManagerFilter::UnloadedOnly);
	}

	if (CVarPreloadAnimToTextureData->GetBool())
	{
		GetPrimaryAssetIdList(FName(TEXT("AnimToTextureDataAsset")), AssetIdListWithBundle, EAssetManagerFilter::UnloadedOnly);
	}
	else
	{
		GetPrimaryAssetIdList(FName(TEXT("AnimToTextureDataAsset")), AssetIdListWithoutBundle, EAssetManagerFilter::UnloadedOnly);
	}
	
	// Loading with no bundle is significantly smaller
	LoadPrimaryAssets(AssetIdListWithoutBundle);

	return LoadPrimaryAssets(AssetIdListWithBundle, DefaultBundleState);
}

void UCitySampleAssetManager::UnloadItemDefinitions()
{
	TArray<FPrimaryAssetId> AssetIdList;
	GetPrimaryAssetIdList(FName(TEXT("CrowdCharacterDataAsset")), AssetIdList);
	GetPrimaryAssetIdList(FName(TEXT("AnimToTextureDataAsset")), AssetIdList);

	UnloadPrimaryAssets(AssetIdList);
}

void UCitySampleAssetManager::PostInitialAssetScan()
{
	Super::PostInitialAssetScan();

	// Asset registry isn't always complete at editor time	
#if WITH_EDITOR
	PreloadItemDefinitions();
#endif //WITH_EDITOR
}

bool UCitySampleAssetManager::ShouldScanPrimaryAssetType(FPrimaryAssetTypeInfo& TypeInfo) const
{
	FName BaseClassName = TypeInfo.PrimaryAssetType;
	if (BaseClassName == FName(TEXT("CrowdCharacterDataAsset")) || BaseClassName == FName(TEXT("AnimToTextureDataAsset")))
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("NoDefaultMaps")))
		{
			return false;
		}
	}

	return Super::ShouldScanPrimaryAssetType(TypeInfo);
}

#if WITH_EDITOR
void UCitySampleAssetManager::PreBeginPIE(bool bStartSimulate)
{
	TGuardValue<bool> PreBeginPIEGuard(bInPreBeginPIE, true);
	TGuardValue<bool> AlreadyLoadedForPIEGuard(bAlreadyLoadedForPIE, false);
	Super::PreBeginPIE(bStartSimulate);

	if (PreloadInEditor_PIE == CVarPreloadInEditor->GetInt())
	{
		PreloadItemDefinitions();
	}
}

void UCitySampleAssetManager::EndPIE(bool bStartSimulate)
{
	Super::EndPIE(bStartSimulate);

	if (PreloadInEditor_PIE == CVarPreloadInEditor->GetInt())
	{
		UnloadItemDefinitions();
	}
}
#endif
