// Copyright Epic Games, Inc. All Rights Reserved.

#include "TextureDeDupeWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CollectionManagerModule.h"
#include "ICollectionManager.h"
#include "Engine/Texture2D.h"
#include "Misc/FeedbackContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogTextureDeDupe, Log, All);

void UTextureDeDupeWidget::FindDuplicateNamedTextures(bool bMustBeInCook)
{
	const FString CookedFileListCollectionName = TEXT("Audit_InCook");
	TArray<FSoftObjectPath> CookedObjects;
	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	if (CollectionManagerModule.Get().CollectionExists(*CookedFileListCollectionName, ECollectionShareType::CST_All))
	{
		CollectionManagerModule.Get().GetAssetsInCollection(*CookedFileListCollectionName, ECollectionShareType::CST_All, CookedObjects);
	}

	FARFilter Filter;
	Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry"); 
	
	TArray<FAssetData> TextureAssetData;
	AssetRegistryModule.Get().GetAssets(Filter, TextureAssetData);

	TMap<FName, TArray<FAssetData>> NamesToAssetData;
	for (int i = 0; i < TextureAssetData.Num(); i++)
	{
		if (bMustBeInCook && CookedObjects.Num() > 0 && !CookedObjects.Contains(TextureAssetData[i].GetSoftObjectPath()))
		{
			continue;
		}

		TArray<FAssetData>& AssetDatas = NamesToAssetData.FindOrAdd(TextureAssetData[i].AssetName);
		AssetDatas.Add(TextureAssetData[i]);
	}

	for (TMap<FName, TArray<FAssetData>>::TIterator DupIt(NamesToAssetData); DupIt; ++DupIt)
	{
		TArray<FAssetData>& Duplicates = DupIt.Value();
		if (Duplicates.Num() <= 1)
		{
			DupIt.RemoveCurrent();
		}
	}

	for (TMap<FName, TArray<FAssetData>>::TIterator DupIt(NamesToAssetData); DupIt; ++DupIt)
	{
		FName& ObjectName = DupIt.Key();
		TArray<FAssetData>& Duplicates = DupIt.Value();

		UE_LOG(LogTextureDeDupe, Log, TEXT("%s"), *ObjectName.ToString());
		for (int i = 0; i < Duplicates.Num(); i++)
		{
			UE_LOG(LogTextureDeDupe, Log, TEXT("\t%s"), *Duplicates[i].PackageName.ToString());
		}
	}
}

void UTextureDeDupeWidget::FindDuplicateCRCTextures(bool bMustBeInCook)
{
	const FString CookedFileListCollectionName = TEXT("Audit_InCook");
	TArray<FSoftObjectPath> CookedObjects;
	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	if (CollectionManagerModule.Get().CollectionExists(*CookedFileListCollectionName, ECollectionShareType::CST_All))
	{
		CollectionManagerModule.Get().GetAssetsInCollection(*CookedFileListCollectionName, ECollectionShareType::CST_All, CookedObjects);
	}

	FARFilter Filter;
	Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> TextureAssetData;
	AssetRegistryModule.Get().GetAssets(Filter, TextureAssetData);

	GWarn->BeginSlowTask(NSLOCTEXT("TextureDeDupe", "TextureDeDupe_CRC", "Calculating CRCs"), true, true);

	int TotalProgress = TextureAssetData.Num();

	// These packages crash the editor if you try to open them
	TArray<FName> DoNotOpenAssets;
	DoNotOpenAssets.Add(TEXT("/Game/Vehicle/vehCar_vehicle03/Texture/Prototype/Weta/maskGrime/T_vehCar_vehicle03_Grime"));

	TMap<uint32, TArray<FAssetData>> CRCToAssetData;
	for (int i = 0; i < TextureAssetData.Num(); i++)
	{
		if (bMustBeInCook && CookedObjects.Num() > 0 && !CookedObjects.Contains(TextureAssetData[i].GetSoftObjectPath()))
		{
			continue;
		}

		if (DoNotOpenAssets.Contains(TextureAssetData[i].PackageName))
		{
			continue;
		}

		GWarn->UpdateProgress(i, TotalProgress);

		if (UTexture2D* Texture = Cast<UTexture2D>(TextureAssetData[i].GetAsset()))
		{
			uint32 CRC;
			if (Texture->GetSourceArtCRC(CRC))
			{
				TArray<FAssetData>& AssetDatas = CRCToAssetData.FindOrAdd(CRC);
				AssetDatas.Add(TextureAssetData[i]);				
			}
		}
	}

	GWarn->EndSlowTask();

	for (TMap<uint32, TArray<FAssetData>>::TIterator DupIt(CRCToAssetData); DupIt; ++DupIt)
	{
		TArray<FAssetData>& Duplicates = DupIt.Value();
		if (Duplicates.Num() <= 1)
		{
			DupIt.RemoveCurrent();
		}
	}

	for (TMap<uint32, TArray<FAssetData>>::TIterator DupIt(CRCToAssetData); DupIt; ++DupIt)
	{
		uint32& CRC = DupIt.Key();
		TArray<FAssetData>& Duplicates = DupIt.Value();

		UE_LOG(LogTextureDeDupe, Log, TEXT("%X"), CRC);
		for (int i = 0; i < Duplicates.Num(); i++)
		{
			UE_LOG(LogTextureDeDupe, Log, TEXT("\t%s"), *Duplicates[i].PackageName.ToString());
		}
	}
}