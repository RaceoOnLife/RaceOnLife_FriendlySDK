// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdPresetDataAsset.h"
#include "AnimToTextureDataAsset.h"
#include "Crowd/CrowdBlueprintLibrary.h"

void UCrowdVATPresetDataAsset::CopyFromCrowdPreset(UCrowdPresetDataAsset* CrowdPresetDataAsset, UCrowdCharacterDataAsset* ValidationAsset)
{
	Presets.Reset(CrowdPresetDataAsset->Presets.Num());
	for (const FCrowdPreset& CrowdPreset : CrowdPresetDataAsset->Presets)
	{
		bool bIsValid = false;
		if (ValidationAsset != nullptr)
		{
			bIsValid = true;
			FCrowdCharacterDefinition ValidationDefinition;
			UCitySampleCrowdFunctionLibrary::GenerateCharacterDefinitionFromOptions(CrowdPreset.CharacterOptions, ValidationAsset, ValidationDefinition);
			UAnimToTextureDataAsset* HeadDataAsset = ValidationDefinition.HeadData.LoadSynchronous();
			UAnimToTextureDataAsset* BodyDataAsset = ValidationDefinition.BodyDefinition.BodyData.LoadSynchronous();
			UAnimToTextureDataAsset* TopDataAsset = ValidationDefinition.OutfitDefinition.TopData.LoadSynchronous();
			UAnimToTextureDataAsset* BottomDataAsset = ValidationDefinition.OutfitDefinition.BottomData.LoadSynchronous();
			UAnimToTextureDataAsset* ShoesDataAsset = ValidationDefinition.OutfitDefinition.ShoesData.LoadSynchronous();

			bIsValid = bIsValid && HeadDataAsset && HeadDataAsset->GetStaticMesh();
			bIsValid = bIsValid && BodyDataAsset && BodyDataAsset->GetStaticMesh();
			bIsValid = bIsValid && TopDataAsset && TopDataAsset->GetStaticMesh();
			bIsValid = bIsValid && BottomDataAsset && BottomDataAsset->GetStaticMesh();
			bIsValid = bIsValid && ShoesDataAsset && ShoesDataAsset->GetStaticMesh();
		}

		if (!bIsValid)
		{
			continue;
		}

		FCrowdVATPreset& NewVATPreset = Presets.AddDefaulted_GetRef();
		NewVATPreset.Name = CrowdPreset.Name;
		NewVATPreset.AnimToTextureAsset = nullptr;
		NewVATPreset.CharacterOptions = CrowdPreset.CharacterOptions;
		NewVATPreset.Meshes.Reset(CrowdPreset.Meshes.Num());
		for (const TSoftObjectPtr<UStaticMesh>& PresetMesh : CrowdPreset.Meshes)
		{
			NewVATPreset.Meshes.Add(PresetMesh);
		}
	}
}

void UCrowdVATPresetDataAsset::CopyToDataTable(UMassTrafficDriverTypesDataAsset* DriverTypesDataAsset)
{
	if (DriverTypesDataAsset)
	{
		DriverTypesDataAsset->DriverTypes.Empty();
		
		for (const FCrowdVATPreset& Preset : Presets)
		{
			FMassTrafficDriverTypeData NewPresetData;
			NewPresetData.AnimationData = Preset.AnimToTextureAsset;
			NewPresetData.Meshes.Reserve(Preset.Meshes.Num());
			for (const TSoftObjectPtr<UStaticMesh>& PresetMesh : Preset.Meshes)
			{
				FMassTrafficDriverMesh& DriverMesh = NewPresetData.Meshes.AddDefaulted_GetRef();
				DriverMesh.StaticMesh = PresetMesh.LoadSynchronous();
			}
			DriverTypesDataAsset->DriverTypes.Add(NewPresetData);
		}
	}
}

void UCrowdVATPresetDataAsset::Randomize(
	UCrowdCharacterDataAsset* CrowdDataAsset, 
	int32 NumPresets, 
	const FCrowdCharacterOptions& BaseOptions,
	TArray<ECrowdLineupVariation> FixedOptions)
{
	if (!IsValid(CrowdDataAsset))
	{
		return;
	}

	TSet<ECrowdLineupVariation> FixedOptionsSet(FixedOptions);

	Presets.Reset(NumPresets);
	for (int32 PresetIndex = 0; PresetIndex < NumPresets; ++PresetIndex)
	{
		FCrowdVATPreset& NewPreset = Presets.AddDefaulted_GetRef();
		NewPreset.Name = FString::Format(TEXT("Preset{0}"), { PresetIndex });
		NewPreset.CharacterOptions = BaseOptions;
		NewPreset.CharacterOptions.Randomize(*CrowdDataAsset, FixedOptionsSet);
	}
}
