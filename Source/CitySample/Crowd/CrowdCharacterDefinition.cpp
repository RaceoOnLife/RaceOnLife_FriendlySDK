// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdCharacterDefinition.h"

#include "Engine/AssetManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RandomStream.h"

#include "Crowd/CrowdBlueprintLibrary.h"
#include "Crowd/CrowdCharacterDataAsset.h"

DEFINE_LOG_CATEGORY(LogCitySampleCrowdDefinition);

namespace CitySample {
	namespace Crowd
	{
		static int32 ForcedGender = 0;
		FAutoConsoleVariableRef CVarForcedGender(TEXT("CitySample.Crowd.ForcedGender"), ForcedGender, TEXT("CitySample crowd gender force"), ECVF_Cheat);
		static int32 ForcedBodyType = 0;
		FAutoConsoleVariableRef CVarForcedBodyType(TEXT("CitySample.Crowd.ForcedBodyType"), ForcedBodyType, TEXT("CitySample crowd body type force"), ECVF_Cheat);
		static int32 ForcedHeadIndex = 0;
		FAutoConsoleVariableRef CVarForcedHeadIndex(TEXT("CitySample.Crowd.ForcedHeadIndex"), ForcedHeadIndex, TEXT("CitySample crowd head index force"), ECVF_Cheat);
		static int32 ForcedHairIndex = 0;
		FAutoConsoleVariableRef CVarForcedHairIndex(TEXT("CitySample.Crowd.ForcedHairIndex"), ForcedHairIndex, TEXT("CitySample crowd hair index force"), ECVF_Cheat);
		static int32 ForcedOutfitIndex = 0;
		FAutoConsoleVariableRef CVarForcedOutfitIndex(TEXT("CitySample.Crowd.ForcedOutfitIndex"), ForcedOutfitIndex, TEXT("CitySample crowd outfit index force"), ECVF_Cheat);
		static int32 ForcedOutfitMaterialIndex = 0;
		FAutoConsoleVariableRef CVarForcedOutfitMaterialIndex(TEXT("CitySample.Crowd.ForcedOutfitMaterialIndex"), ForcedOutfitMaterialIndex, TEXT("CitySample crowd outfit material index force"), ECVF_Cheat);
		static int32 ForcedSkinTextureIndex = 0;
		FAutoConsoleVariableRef CVarForcedSkinTextureIndex(TEXT("CitySample.Crowd.ForcedSkinTextureIndex"), ForcedSkinTextureIndex, TEXT("CitySample crowd skin texture index force"), ECVF_Cheat);
		static int32 ForcedPatternColorIndex = 0;
		FAutoConsoleVariableRef CVarForcedPatternColorIndex(TEXT("CitySample.Crowd.ForcedPatternColorIndex"), ForcedPatternColorIndex, TEXT("CitySample crowd pattern color index force"), ECVF_Cheat);
		static int32 ForcedPatternOptionIndex = 0;
		FAutoConsoleVariableRef CVarForcedPatternOptionIndex(TEXT("CitySample.Crowd.ForcedPatternOptionIndex"), ForcedPatternOptionIndex, TEXT("CitySample crowd pattern option index force"), ECVF_Cheat);
	}
}

void FCrowdMaterialColorOverride::ApplyToMaterial(UMaterialInstanceDynamic* MID) const
{
	if (MID)
	{
		switch (ParameterType)
		{
		case ECrowdMaterialParameterType::Color:
			MID->SetVectorParameterValue(ParameterName, Color.ReinterpretAsLinear());
			break;
		case ECrowdMaterialParameterType::Vector:
			MID->SetVectorParameterValue(ParameterName, Vector);
			break;
		case ECrowdMaterialParameterType::Float:
			MID->SetScalarParameterValue(ParameterName, Float);
			break;
		default:
			checkNoEntry();
		}
	}
}

void FCrowdPatternInfo::ApplyToMaterial(UMaterialInstanceDynamic* MID) const
{
	static const FName PatternScaleParameterName = "A_Pattern_Scale";
	static const FName PatternSelectParameterName = "A_Pattern_Select";
	static const FName PatternRoughnessParameterName = "A_Pattern_RoughMult";
	
	if (MID)
	{
		MID->SetScalarParameterValue(PatternScaleParameterName, Scale);
		MID->SetScalarParameterValue(PatternSelectParameterName, Selection);
		MID->SetScalarParameterValue(PatternRoughnessParameterName, RoughnessMultiplier);
	}
}

void FCrowdOutfitMaterialDefinition::ApplyToComponent(UMeshComponent* MeshComponent, const uint8 PatternColorIndex, const uint8 PatternOptionIndex) const
{
	static const FName CrowdColorParameterName = "A_CrowdColor_pattern";
	
	if (MeshComponent)
	{
		TArray<FName> MaterialSlots = MeshComponent->GetMaterialSlotNames();

		for (const FName& MaterialSlot : MaterialSlots)
		{
			UMaterialInstanceDynamic* MaterialInstance = UCitySampleCrowdFunctionLibrary::CreateDynamicMaterialInstance(MeshComponent, MaterialSlot);
			const FCrowdMaterialOverride* MaterialOverride = MaterialOverrides.Find(MaterialSlot);
			if (MaterialInstance && MaterialOverride)
			{
				// Apply Parameter Overrides
				for (const FCrowdMaterialColorOverride& ParameterOverride : MaterialOverride->ParameterOverrides)
				{
					ParameterOverride.ApplyToMaterial(MaterialInstance);
				}

				// Apply Patterns
				FColor PatternColor;
				FCrowdPatternInfo PatternInfo;

				if (GetPatternInfoForSlot(MaterialSlot, PatternColorIndex, PatternOptionIndex, PatternColor, PatternInfo))
				{
					MaterialInstance->SetVectorParameterValue(CrowdColorParameterName, PatternColor.ReinterpretAsLinear());
					PatternInfo.ApplyToMaterial(MaterialInstance);
				}
			}
			else
			{
				//UE_LOG(LogCrowdBlueprint, Warning, TEXT("Failed to generate MID for component %s on slot %s"), *MeshComponent->GetName(), *MaterialSlot.ToString());
			}
		}
	}
}

bool FCrowdOutfitMaterialDefinition::GetPatternInfoForSlot(const FName SlotName, const uint8 PatternColorIndex, const uint8 PatternSelectionIndex, FColor& PatternColor, FCrowdPatternInfo& PatternInfo) const
{
	const FCrowdMaterialOverride* MaterialOverride = MaterialOverrides.Find(SlotName);

	// Keep track of the scale multiplier for driven slots
	float DrivenScaleMultiplier = 1.f;
	
	while (true)
	{
		if (MaterialOverride->PatternUsage == ECrowdPatternUsage::None)
		{
			break;
		}
		else if (MaterialOverride->PatternUsage == ECrowdPatternUsage::PatternList)
		{
			if (MaterialOverride->ComplimentaryColors.Num() && MaterialOverride->Patterns.Num())
			{
				PatternColor = MaterialOverride->ComplimentaryColors[PatternColorIndex % MaterialOverride->ComplimentaryColors.Num()];
				PatternInfo = MaterialOverride->Patterns[PatternSelectionIndex % MaterialOverride->Patterns.Num()];

				// Apply any scale multipliers we've accumulated via walking a driven chain
				PatternInfo.Scale *= DrivenScaleMultiplier;
				return true;
			}
			
			break;
		}
		else if (MaterialOverride->PatternUsage == ECrowdPatternUsage::Driven)
		{
			// Walk up to the parent
			const FCrowdMaterialOverride* NewMaterialOverride = MaterialOverrides.Find(MaterialOverride->SourceSlot);

			// Terminate if parent doesn't exist
			if (!NewMaterialOverride)
			{
				break;
			}

			// Update the scale multiplier and then set the current material override to evaluate
			DrivenScaleMultiplier *= MaterialOverride->ScaleMultiplier;
			MaterialOverride = NewMaterialOverride;
		}
		else
		{
			// Unknown enum value
			break;
		}
	}

	return false;
}

int32 FCrowdOutfitMaterialDefinition::GetMaxPatternColors() const
{
	int32 MaxPatternColors = 0;

	for (const TPair<FName, FCrowdMaterialOverride>& MaterialOverride : MaterialOverrides)
	{
		MaxPatternColors = FMath::Max(MaterialOverride.Value.ComplimentaryColors.Num(), MaxPatternColors);
	}

	return MaxPatternColors;
}

int32 FCrowdOutfitMaterialDefinition::GetMaxPatternOptions() const
{
	int32 MaxPatternOptions = 0;

	for (const TPair<FName, FCrowdMaterialOverride>& MaterialOverride : MaterialOverrides)
	{
		MaxPatternOptions = FMath::Max(MaterialOverride.Value.Patterns.Num(), MaxPatternOptions);
	}

	return MaxPatternOptions;
}

void FCrowdHairDefinition::UpdateGroomBinding(const FSoftObjectPath& PathToHeadMesh, const ECitySampleCrowdGender Skeleton)
{
	// Base paths stored per gender so they can be indexed via an ECitySampleCrowdGender
	static const TArray<FString> BasePaths = {
		"/Game/Crowd/Character/Male/GroomBindings",
		"/Game/Crowd/Character/Female/GroomBindings"
	};

	// Early out if there's no valid groom
	if (Groom.IsNull())
	{
		GroomBinding = nullptr;
		return;
	}

	if (PathToHeadMesh.IsNull())
	{
		GroomBinding = nullptr;
		return;
	}

	// Remove any `_Strands` suffix if it exists
	FString SanitizedGroomName = Groom.GetAssetName();
	SanitizedGroomName.RemoveFromEnd(TEXT("_Strands"), ESearchCase::IgnoreCase);
	
	// AssetName is constructed as GB_<HeadAssetName>_<GroomAssetName>
	const FString AssetName = FString::Printf(TEXT("GB_%s_%s"), *PathToHeadMesh.GetAssetName(), *SanitizedGroomName);

	// Paths are construced as <BasePath>/<AssetName>.<AssetName>
	const FSoftObjectPath NewGroomBindingPath(FString::Printf(TEXT("%s/%s.%s"), *BasePaths[static_cast<uint8>(Skeleton)], *AssetName, *AssetName));

	// Check that the soft object path constructed points at an actual asset
	FAssetData AssetData;
	UAssetManager::Get().GetAssetDataForPath(NewGroomBindingPath, AssetData);

	if (AssetData.IsValid())
	{
		GroomBinding = NewGroomBindingPath;
	}
	else
	{
		UE_LOG(LogCitySampleCrowdDefinition, Warning, TEXT("Unable to find binding asset for: %s\nGroom binding may be incorrect."), *NewGroomBindingPath.ToString());
	}
}

TArray<FSoftObjectPath> FCrowdCharacterDefinition::GetSoftPathsToLoad() const
{
	// Initialize with the direct SoftObjectPtrs
	TArray<FSoftObjectPath> ObjectsToLoad = {
		BodyDefinition.Base.ToSoftObjectPath(),
		Head.ToSoftObjectPath(),
		AccessoryDefinition.Mesh.ToSoftObjectPath(),
		AccessoryDefinition.AccessoryAnimSet.ToSoftObjectPath(),
		SkinTextureDefinition.BodyColor.ToSoftObjectPath(),	
		SkinTextureDefinition.ChestColor.ToSoftObjectPath(),	
		SkinTextureDefinition.FaceCavity.ToSoftObjectPath(),
		SkinTextureDefinition.FaceColor.ToSoftObjectPath(),
		SkinTextureDefinition.FaceColorCM1.ToSoftObjectPath(),
		SkinTextureDefinition.FaceColorCM2.ToSoftObjectPath(),
		SkinTextureDefinition.FaceColorCM3.ToSoftObjectPath(),
		SkinTextureDefinition.FaceNormal.ToSoftObjectPath(),
		SkinTextureDefinition.FaceNormalWM1.ToSoftObjectPath(),
		SkinTextureDefinition.FaceNormalWM2.ToSoftObjectPath(),
		SkinTextureDefinition.FaceNormalWM3.ToSoftObjectPath(),
		SkinTextureDefinition.FaceRoughness.ToSoftObjectPath(),
		SkinTextureDefinition.ChestRoughness.ToSoftObjectPath()
	};

	// Add all the hair definitions
	for (const FCrowdHairDefinition& HairDefinition : HairDefinitions)
	{
		ObjectsToLoad.Add(HairDefinition.Groom.ToSoftObjectPath());
		ObjectsToLoad.Add(HairDefinition.GroomBinding.ToSoftObjectPath());
		ObjectsToLoad.Add(HairDefinition.PhysicsAsset.ToSoftObjectPath());
		ObjectsToLoad.Add(HairDefinition.BakedGroomMap.ToSoftObjectPath());
		ObjectsToLoad.Add(HairDefinition.FollicleMask.ToSoftObjectPath());
	}

	TArray<TSoftObjectPtr<UAnimToTextureDataAsset>> AnimToTextureDataAssets = {
		BodyDefinition.BodyData,
		HeadData,
		OutfitDefinition.TopData,
		OutfitDefinition.BottomData,
		OutfitDefinition.ShoesData
	};

	for (TSoftObjectPtr<UAnimToTextureDataAsset> SoftAnimToTextureDataAsset : AnimToTextureDataAssets)
	{
		// Assume all the AnimToTextureDataAssets are already loaded
		if (UAnimToTextureDataAsset* AnimToTextureDataAsset = SoftAnimToTextureDataAsset.LoadSynchronous())
		{
			ObjectsToLoad.Add(AnimToTextureDataAsset->SkeletalMesh.ToSoftObjectPath());
		}
	}

	// Animation data assets:
	if (!LocomotionAnimSet.IsNull())
	{
		ObjectsToLoad.Add(LocomotionAnimSet.ToSoftObjectPath());
	}

	if (!ContextualAnimDataAsset.IsNull())
	{
		ObjectsToLoad.Add(ContextualAnimDataAsset.ToSoftObjectPath());
	}

	// Strip any null assets from the list
	int i = 0;
	while (i < ObjectsToLoad.Num())
	{
		if (ObjectsToLoad[i].IsNull())
		{
			ObjectsToLoad.RemoveAt(i);
		}
		else
		{
			i += 1;
		}
	}

	return ObjectsToLoad;
}

void FCrowdCharacterOptions::GenerateCharacterDefinition(const UCrowdCharacterDataAsset* DataAsset, FCrowdCharacterDefinition& CharacterDefinition) const
{
	if (!DataAsset)
	{
		return;
	}

	// Reset the Definition to prevent any state carrying over
	CharacterDefinition = FCrowdCharacterDefinition();

	const FCrowdGenderDefinition& GenderDefinition = Skeleton == ECitySampleCrowdGender::A ? DataAsset->SkeletonA : DataAsset->SkeletonB;
	const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = BodyType == ECitySampleCrowdBodyType::NormalWeight ?
		GenderDefinition.NormalWeight : BodyType == ECitySampleCrowdBodyType::OverWeight ? GenderDefinition.OverWeight : GenderDefinition.UnderWeight;

	CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

	if (BodyOutfitDefinition.HeadsData.Num())
	{
		CharacterDefinition.HeadData = BodyOutfitDefinition.HeadsData[HeadIndex % BodyOutfitDefinition.HeadsData.Num()];
		CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(CharacterDefinition.HeadData);
	}

	if (BodyOutfitDefinition.Outfits.Num())
	{
		CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[OutfitIndex % BodyOutfitDefinition.Outfits.Num()];
	}

	{
		TArray<int> HairIndices = {
			HairIndex,
			EyebrowsIndex,
			FuzzIndex,
			EyelashesIndex,
			MustacheIndex,
			BeardIndex,
		};

		if (ensure(HairIndices.Num() <= CharacterDefinition.HairDefinitions.Num()))
		{
			for (int SlotIdx = 0; SlotIdx < HairIndices.Num(); ++SlotIdx)
			{
				const TArray<FCrowdHairDefinition>& HairDefinitions = GenderDefinition.HairSlots[SlotIdx].HairDefinitions;
				int HairIdx = HairIndices[SlotIdx];
				if (HairDefinitions.Num())
				{
					CharacterDefinition.HairDefinitions[SlotIdx] = HairDefinitions[HairIdx % HairDefinitions.Num()];

					// Try to resolve a groom binding 
					if (SlotIdx != static_cast<uint8>(ECrowdHairSlots::Eyebrows) && SlotIdx != static_cast<uint8>(ECrowdHairSlots::Eyelashes))
					{
						CharacterDefinition.HairDefinitions[SlotIdx].UpdateGroomBinding(CharacterDefinition.Head.ToSoftObjectPath(), Skeleton);
					}
				}
			}
		}
	}

	if (BodyOutfitDefinition.Accessories.Num())
	{
		CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[AccessoryIndex % BodyOutfitDefinition.Accessories.Num()];
	}
	
	if (BodyOutfitDefinition.LocomotionAnimSets.Num())
	{
		CharacterDefinition.LocomotionAnimSet = BodyOutfitDefinition.LocomotionAnimSets[AnimSetIndex % BodyOutfitDefinition.LocomotionAnimSets.Num()];
	}

	CharacterDefinition.ContextualAnimDataAsset = BodyOutfitDefinition.ContextualAnimData;
	
	if (DataAsset->HairColors.Num())
	{
		CharacterDefinition.HairColorDefinition = DataAsset->HairColors[HairColorIndex % DataAsset->HairColors.Num()];
	}

	if (GenderDefinition.OutfitMaterials.Num())
	{
		CharacterDefinition.OutfitMaterialDefinition = GenderDefinition.OutfitMaterials[OutfitMaterialIndex % GenderDefinition.OutfitMaterials.Num()];
	}

	if (GenderDefinition.SkinMaterials.Num())
	{
		const FCrowdSkinMaterialDefinition& SkinMaterialDefinition = GenderDefinition.SkinMaterials[SkinTextureIndex % GenderDefinition.SkinMaterials.Num()];
		CharacterDefinition.SkinTextureDefinition = SkinMaterialDefinition.Texture;

		if (SkinMaterialDefinition.TextureModifiers.Num())
		{
			CharacterDefinition.SkinTextureModifierDefinition = SkinMaterialDefinition.TextureModifiers[SkinTextureModifierIndex % SkinMaterialDefinition.TextureModifiers.Num()];
		}
	}

	if (BodyOutfitDefinition.ScaleFactors.Num())
	{
		CharacterDefinition.ScaleFactor = BodyOutfitDefinition.ScaleFactors[ScaleFactorIndex % BodyOutfitDefinition.ScaleFactors.Num()];
	}

	CharacterDefinition.PatternColorIndex = PatternColorIndex;
	CharacterDefinition.PatternOptionIndex = PatternOptionIndex;
	CharacterDefinition.RayTracingMinLOD = DataAsset->RayTracingMinLOD;
}

void FCrowdCharacterOptions::Randomize(const UCrowdCharacterDataAsset& DataAsset)
{
	Randomize(DataAsset, TSet<ECrowdLineupVariation>());
}

void FCrowdCharacterOptions::Randomize(const UCrowdCharacterDataAsset& DataAsset, const FRandomStream& RandomStream)
{
	Randomize(DataAsset, TSet<ECrowdLineupVariation>(), RandomStream);
}

// Utility function for conditionally setting a random enum value in the options structure
template<typename T>
void SetOptionEnumFromRange(const ECrowdLineupVariation OptionType, T& EnumValue, const int MinimumValue, const int MaximumValue, const TSet<ECrowdLineupVariation> FixedOptions, const FRandomStream& RandomStream)
{
	if (!FixedOptions.Contains(OptionType))
	{
		EnumValue = static_cast<T>(RandomStream.RandRange(MinimumValue, MaximumValue));
	}
}

// Utility function for conditionally setting a random value in the options structure
template<typename T>
void SetOptionIndexFromArray(const ECrowdLineupVariation OptionType, uint8& OptionIndex, const TArray<T>& OptionsArray, const TSet<ECrowdLineupVariation> FixedOptions, const FRandomStream& RandomStream)
{
	if (!FixedOptions.Contains(OptionType))
	{
		OptionIndex = OptionsArray.Num() ? RandomStream.RandRange(0, OptionsArray.Num() - 1) : 0;	
	}	
}

// Utility function for conditionally setting a random value in the options structure
// Assumes the Array is an Array of structs where the struct has a property called "RandomWeight"
template<typename T>
void SetWeightedOptionIndexFromArray(const ECrowdLineupVariation OptionType, uint8& OptionIndex, const TArray<T>& OptionsArrayOfStructs, const TSet<ECrowdLineupVariation> FixedOptions, const FRandomStream& RandomStream)
{
	// Do nothing if the option is fixed
	if (!FixedOptions.Contains(OptionType))
	{
		// Early out if the array is empty
		if (OptionsArrayOfStructs.IsEmpty())
		{
			OptionIndex = 0;
			return;
		}

		// Generates weighted random by generating a number in the range [0, sum(Weights)]
		// and then looks for the index such that the sum of the weights below that index is <= the random number
		// and the sum of the weights up to an including itself are > the random number
		
		TArray<int32> RandomThresholds;
		RandomThresholds.Reserve(OptionsArrayOfStructs.Num());
		
		int32 TotalWeight = 0;

		for (const T& StructElement : OptionsArrayOfStructs)
		{
			// Ensure we don't go negative
			TotalWeight += FMath::Max(0, StructElement.RandomWeight);
			RandomThresholds.Add(TotalWeight);
		}

		// Need to subtract 1 as we go from 0 and not 1
		int32 RandomValue = RandomStream.RandRange(0, TotalWeight-1);

		// Find the first entry in the thresholds array that is encloses our random value from above
		OptionIndex = RandomThresholds.IndexOfByPredicate([RandomValue](const int32 RandomThreshold)
		{
			return RandomThreshold > RandomValue;
		});
	}	
}

void FCrowdCharacterOptions::Randomize(const UCrowdCharacterDataAsset& DataAsset,
	TSet<ECrowdLineupVariation> FixedProperties)
{
	FRandomStream RandomStream;
	RandomStream.GenerateNewSeed();
	
	Randomize(DataAsset, FixedProperties, RandomStream);
}

void FCrowdCharacterOptions::Randomize(const UCrowdCharacterDataAsset& DataAsset,
	TSet<ECrowdLineupVariation> FixedProperties, const FRandomStream& RandomStream)
{
	// First generate all the options which don't rely on other options
	SetOptionEnumFromRange(ECrowdLineupVariation::Skeleton, Skeleton, 0, 1, FixedProperties, RandomStream);
	SetOptionEnumFromRange(ECrowdLineupVariation::BodyType, BodyType, 0, 2, FixedProperties, RandomStream);
	SetOptionIndexFromArray(ECrowdLineupVariation::HairColor, HairColorIndex, DataAsset.HairColors, FixedProperties, RandomStream);

	// Grab the Gender Definition
	const FCrowdGenderDefinition& GenderDefinition = Skeleton == ECitySampleCrowdGender::A ? DataAsset.SkeletonA : DataAsset.SkeletonB;

	// Generate all the options which rely on Gender directly
	SetOptionIndexFromArray(ECrowdLineupVariation::OutfitMaterial, OutfitMaterialIndex, GenderDefinition.OutfitMaterials, FixedProperties, RandomStream);
	SetOptionIndexFromArray(ECrowdLineupVariation::SkinTexture, SkinTextureIndex, GenderDefinition.SkinMaterials, FixedProperties, RandomStream);

	// Hair slots can be done in a loop. The Options array should match the size of HairSlots in the GenderDefinition
	TArray<uint8*> HairOptions = { &HairIndex, &EyebrowsIndex, &FuzzIndex, &EyelashesIndex, &MustacheIndex, &BeardIndex };
	if (ensure(HairOptions.Num() <= GenderDefinition.HairSlots.Num()))
	{
		for (int HairSlotIdx = 0; HairSlotIdx < HairOptions.Num(); ++HairSlotIdx)
		{
			// Find the starting point in the OptionType enum and then offset it by the slot index to find the correct enum value
			const int OptionIndex = static_cast<int>(ECrowdLineupVariation::Hair) + HairSlotIdx;
			SetOptionIndexFromArray(ECrowdLineupVariation(OptionIndex), *HairOptions[HairSlotIdx], GenderDefinition.HairSlots[HairSlotIdx].HairDefinitions, FixedProperties, RandomStream);
		}
	}

	// Grab the BodyOutfit Definition from Body Type
	const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = BodyType == ECitySampleCrowdBodyType::NormalWeight ?
		GenderDefinition.NormalWeight : BodyType == ECitySampleCrowdBodyType::OverWeight ? GenderDefinition.OverWeight : GenderDefinition.UnderWeight;

	// Now Generate all the options which rely on Body Type
	SetOptionIndexFromArray(ECrowdLineupVariation::Head, HeadIndex, BodyOutfitDefinition.HeadsData, FixedProperties, RandomStream);
	SetOptionIndexFromArray(ECrowdLineupVariation::Outfit, OutfitIndex, BodyOutfitDefinition.Outfits, FixedProperties, RandomStream);
	SetOptionIndexFromArray(ECrowdLineupVariation::ScaleFactor, ScaleFactorIndex, BodyOutfitDefinition.ScaleFactors, FixedProperties, RandomStream);
	AnimSetIndex = BodyOutfitDefinition.LocomotionAnimSets.Num() ? RandomStream.RandRange(0, BodyOutfitDefinition.LocomotionAnimSets.Num() - 1) : 0;
	SetWeightedOptionIndexFromArray(ECrowdLineupVariation::Accessory, AccessoryIndex, BodyOutfitDefinition.Accessories, FixedProperties, RandomStream);

	// Grab the Outfit Material definition if possible

	if (GenderDefinition.OutfitMaterials.Num())
	{
		const FCrowdOutfitMaterialDefinition& OutfitMaterialDefinition = GenderDefinition.OutfitMaterials[OutfitMaterialIndex % GenderDefinition.OutfitMaterials.Num()];

		if (!FixedProperties.Contains(ECrowdLineupVariation::PatternColor))
		{
			PatternColorIndex = RandomStream.RandRange(0, OutfitMaterialDefinition.GetMaxPatternColors() - 1);
		}

		if (!FixedProperties.Contains(ECrowdLineupVariation::PatternOption))
		{
			PatternOptionIndex = RandomStream.RandRange(0, OutfitMaterialDefinition.GetMaxPatternOptions() - 1);
		}
	}

	// Grab the SkinTexture definition if possible
	if (GenderDefinition.SkinMaterials.Num())
	{
		// If the array has entries then our randomization will have generated a valid index within it so no need to do an IsValidIndex first
		const FCrowdSkinMaterialDefinition& SkinMaterialDefinition = GenderDefinition.SkinMaterials[SkinTextureIndex];

		// Calculate a random modifier index
		SetOptionIndexFromArray(ECrowdLineupVariation::SkinTextureModifier, SkinTextureModifierIndex, SkinMaterialDefinition.TextureModifiers, FixedProperties, RandomStream);
	}

	if (CitySample::Crowd::ForcedGender > 0)
	{
		Skeleton = ECitySampleCrowdGender(CitySample::Crowd::ForcedGender - 1);
	}
	if (CitySample::Crowd::ForcedBodyType > 0)
	{
		BodyType = ECitySampleCrowdBodyType(CitySample::Crowd::ForcedBodyType - 1);
	}
	if (CitySample::Crowd::ForcedHeadIndex > 0)
	{
		HeadIndex = CitySample::Crowd::ForcedHeadIndex - 1;
	}
	if (CitySample::Crowd::ForcedHairIndex > 0)
	{
		HairIndex = CitySample::Crowd::ForcedHairIndex - 1;
	}
	if (CitySample::Crowd::ForcedOutfitIndex > 0)
	{
		OutfitIndex = CitySample::Crowd::ForcedOutfitIndex - 1;
	}
	if (CitySample::Crowd::ForcedOutfitMaterialIndex > 0)
	{
		OutfitMaterialIndex = CitySample::Crowd::ForcedOutfitMaterialIndex - 1;
	}
	if (CitySample::Crowd::ForcedSkinTextureIndex > 0)
	{
		SkinTextureIndex = CitySample::Crowd::ForcedSkinTextureIndex - 1;
	}
	if (CitySample::Crowd::ForcedPatternColorIndex > 0)
	{
		PatternColorIndex = CitySample::Crowd::ForcedPatternColorIndex - 1;
	}
	if (CitySample::Crowd::ForcedPatternOptionIndex > 0)
	{
		PatternOptionIndex = CitySample::Crowd::ForcedPatternOptionIndex - 1;
	}
}

FCrowdVisualizationID::FCrowdVisualizationID(const FCrowdCharacterOptions InOptions)
{
	FCrowdVisualizationBitfield& VisualizationBitfield = AsMutableBitfield();

	VisualizationBitfield.Skeleton					= static_cast<uint8>(InOptions.Skeleton);
	VisualizationBitfield.BodyType					= static_cast<uint8>(InOptions.BodyType);

	VisualizationBitfield.HeadIndex					= InOptions.HeadIndex;
	VisualizationBitfield.OutfitIndex				= InOptions.OutfitIndex;
	VisualizationBitfield.OutfitMaterialIndex		= InOptions.OutfitMaterialIndex;
	VisualizationBitfield.HairIndex					= InOptions.HairIndex;
	VisualizationBitfield.EyebrowsIndex				= InOptions.EyebrowsIndex;
	VisualizationBitfield.EyelashesIndex			= InOptions.EyelashesIndex;
	VisualizationBitfield.MustacheIndex				= InOptions.MustacheIndex;
	
	VisualizationBitfield.BeardIndex				= InOptions.BeardIndex;
	VisualizationBitfield.HairColorIndex			= InOptions.HairColorIndex;
	VisualizationBitfield.SkinTextureIndex			= InOptions.SkinTextureIndex;
	VisualizationBitfield.SkinTextureModifierIndex	= InOptions.SkinTextureModifierIndex;
	VisualizationBitfield.AccessoryIndex			= InOptions.AccessoryIndex;
	VisualizationBitfield.ScaleFactorIndex			= InOptions.ScaleFactorIndex;

	VisualizationBitfield.PatternColorIndex			= InOptions.PatternColorIndex;
	VisualizationBitfield.PatternOptionIndex		= InOptions.PatternOptionIndex;

	VisualizationBitfield.AnimSetIndex				= InOptions.AnimSetIndex;
}

FCrowdCharacterOptions FCrowdVisualizationID::ToCharacterOptions() const
{
	FCrowdCharacterOptions OutOptions;

	const FCrowdVisualizationBitfield& VisualizationBitfield = AsBitfield();

	OutOptions.Skeleton					= static_cast<ECitySampleCrowdGender>(VisualizationBitfield.Skeleton);
	// Mod here to prevent an invalid body type
	OutOptions.BodyType					= static_cast<ECitySampleCrowdBodyType>(VisualizationBitfield.BodyType % 3);
	
	OutOptions.HeadIndex				= VisualizationBitfield.HeadIndex;
	OutOptions.OutfitIndex				= VisualizationBitfield.OutfitIndex;
	OutOptions.OutfitMaterialIndex		= VisualizationBitfield.OutfitMaterialIndex;
	OutOptions.HairIndex				= VisualizationBitfield.HairIndex;
	OutOptions.EyebrowsIndex			= VisualizationBitfield.EyebrowsIndex;
	OutOptions.EyelashesIndex			= VisualizationBitfield.EyelashesIndex;
	OutOptions.MustacheIndex			= VisualizationBitfield.MustacheIndex;
	
	OutOptions.BeardIndex				= VisualizationBitfield.BeardIndex;
	OutOptions.HairColorIndex			= VisualizationBitfield.HairColorIndex;
	OutOptions.SkinTextureIndex			= VisualizationBitfield.SkinTextureIndex;
	OutOptions.SkinTextureModifierIndex	= VisualizationBitfield.SkinTextureModifierIndex;
	OutOptions.AccessoryIndex			= VisualizationBitfield.AccessoryIndex;
	OutOptions.ScaleFactorIndex			= VisualizationBitfield.ScaleFactorIndex;

	OutOptions.AnimSetIndex				= VisualizationBitfield.AnimSetIndex;

	OutOptions.PatternColorIndex		= VisualizationBitfield.PatternColorIndex;
	OutOptions.PatternOptionIndex		= VisualizationBitfield.PatternOptionIndex;

	// No fuzz index information is stored in the ID
	OutOptions.FuzzIndex = 0;

	return OutOptions;
}

FCrowdVisualizationBitfield& FCrowdVisualizationID::AsMutableBitfield()
{
	// Ensure the bitfield struct we're using fits inside an int64
	static_assert(sizeof(FCrowdVisualizationBitfield) <= sizeof(int64));
	
	return reinterpret_cast<FCrowdVisualizationBitfield&>(this->PackedData);
}

const FCrowdVisualizationBitfield& FCrowdVisualizationID::AsBitfield() const
{
	// Ensure the bitfield struct we're using fits inside an int64
	static_assert(sizeof(FCrowdVisualizationBitfield) <= sizeof(int64));
	
	return reinterpret_cast<const FCrowdVisualizationBitfield&>(this->PackedData);
}

void FCrowdVisualizationID::Randomize()
{
	FRandomStream RandomStream;
	RandomStream.GenerateNewSeed();
	Randomize(RandomStream);
}

void FCrowdVisualizationID::Randomize(int32 InSeed)
{
	FRandomStream RandomStream;
	RandomStream.Initialize(InSeed);
	Randomize(RandomStream);
}

void FCrowdVisualizationID::Randomize(const FRandomStream& RandomStream)
{
	// RandomStream only generates uint32's so we'll just compose two
	// random uint32s to make a random int64
	uint32* PackedDataAddress = reinterpret_cast<uint32*>(&PackedData);
	*PackedDataAddress	= RandomStream.GetUnsignedInt();
	*(PackedDataAddress + 1) = RandomStream.GetUnsignedInt();
	
}