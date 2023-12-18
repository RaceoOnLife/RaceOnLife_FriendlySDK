// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CrowdCharacterDefinition.h"
#include "MassTrafficDrivers.h"
#include "Engine/DataAsset.h"

#include "CrowdPresetDataAsset.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdPreset
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCrowdCharacterOptions CharacterOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSoftObjectPtr<UStaticMesh>> Meshes;
};

UCLASS(Blueprintable, BlueprintType)
class CITYSAMPLE_API UCrowdPresetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "Name"))
	TArray<FCrowdPreset> Presets;
};

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdVATPreset : public FCrowdPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UAnimToTextureDataAsset> AnimToTextureAsset = nullptr;
};

UCLASS(Blueprintable, BlueprintType)
class CITYSAMPLE_API UCrowdVATPresetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "Name"))
	TArray<FCrowdVATPreset> Presets;

	UFUNCTION(BlueprintCallable)
	void CopyFromCrowdPreset(UCrowdPresetDataAsset* CrowdPresetDataAsset, UCrowdCharacterDataAsset* ValidationAsset);

	UFUNCTION(BlueprintCallable)
	void CopyToDataTable(UMassTrafficDriverTypesDataAsset* DriverTypesDataAsset);

	UFUNCTION(BlueprintCallable)
	void Randomize(
		UCrowdCharacterDataAsset* CrowdDataAsset,
		int32 NumPresets,
		const FCrowdCharacterOptions& BaseOptions,
		TArray<ECrowdLineupVariation> FixedOptions);
};

