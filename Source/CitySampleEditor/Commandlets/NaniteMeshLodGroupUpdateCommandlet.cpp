// Copyright Epic Games, Inc. All Rights Reserved.

#include "NaniteMeshLodGroupUpdateCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "FileHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogNaniteMeshLodGroupUpdate, Log, All);

static const FName CitySampleNaniteMeshLODGroupName(TEXT("CitySampleNaniteMeshLODGroup"));

int32 UNaniteMeshLodGroupUpdateCommandlet::Main(const FString& FullCommandLine)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Params;

	ParseCommandLine(*FullCommandLine, Tokens, Switches, Params);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Update Registry Module
	UE_LOG(LogNaniteMeshLodGroupUpdate, Display, TEXT("Searching Asset Registry..."));
	AssetRegistryModule.Get().SearchAllAssets(true);

	TArray<FAssetData> Assets;
	TArray<UObject*> ModifiedObjects;

	FARFilter Filter;
	Filter.bIncludeOnlyOnDiskAssets = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game/")));
	Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"),TEXT("StaticMesh")));
	Filter.bRecursivePaths = true;

	AssetRegistry.GetAssets(Filter, Assets);

	int32 i = 0;
	
	for (FAssetData& Asset : Assets)
	{
		UE_LOG(LogNaniteMeshLodGroupUpdate, Display, TEXT("Checking StaticMesh %s"), *Asset.GetObjectPathString());
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
		{
			if (Mesh->NaniteSettings.bEnabled)
			{
				UE_LOG(LogNaniteMeshLodGroupUpdate, Display, TEXT("Updating mesh %s to '%s' from '%s'"),
					*Asset.GetObjectPathString(),
					*CitySampleNaniteMeshLODGroupName.ToString(),
					*Mesh->LODGroup.ToString());

				Mesh->Modify();
				Mesh->LODGroup = CitySampleNaniteMeshLODGroupName;
				ModifiedObjects.Add(Mesh);
			}
		}

		if ((++i % 300) == 0)
		{
			UEditorLoadingAndSavingUtils::SaveDirtyPackages(/*bSaveMaps=*/false, /*bSaveAssets=*/true);
			CollectGarbage(RF_NoFlags, true);
		}
	}
	
	return 0;
}