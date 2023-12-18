// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdCharacterLineupActor.h"

#include "CrowdBlueprintLibrary.h"
#include "Components/TextRenderComponent.h"
#include "CrowdCharacterDataAsset.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/ScopedSlowTask.h"


ACrowdCharacterLineup::ACrowdCharacterLineup()
	:	Spacing(500, 70, 180)
	,   LocationOffset(0, 0, 117)
	,   RotationOffset(0, 180, 0)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;

#if WITH_EDITOR
	if (RootComponent)
	{
		RootComponent->bVisualizeComponent = true;
	}
#endif
}

void ACrowdCharacterLineup::Destroyed()
{
	ClearLineup();

	Super::Destroyed();
}

#if WITH_EDITOR
void ACrowdCharacterLineup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty)
	{
		FName ModifiedPropertyName = PropertyChangedEvent.MemberProperty->GetFName();

		if (ModifiedPropertyName == GET_MEMBER_NAME_CHECKED(ACrowdCharacterLineup, Spacing) ||
			ModifiedPropertyName == GET_MEMBER_NAME_CHECKED(ACrowdCharacterLineup, LocationOffset) ||
			ModifiedPropertyName == GET_MEMBER_NAME_CHECKED(ACrowdCharacterLineup, RotationOffset))
		{
			UpdateInstanceTransforms();
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ACrowdCharacterLineup::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	UpdateInstanceTransforms();
}

#endif

void ACrowdCharacterLineup::ClearLineup()
{
	for (FCrowdLineupInstance LineupInstance : LineupInstances)
	{
		if (LineupInstance.LineupActor)
		{
			LineupInstance.LineupActor->Destroy();
		}
	}
	LineupInstances.Empty();

	for (UTextRenderComponent* RowLabel : RowLabels)
	{
		if (RowLabel)
		{
			RowLabel->DestroyComponent();
		}
	}
	RowLabels.Empty();
}

void ACrowdCharacterLineup::UpdateLineup()
{
	for (FCrowdLineupInstance& LineupInstance : LineupInstances)
	{
		if (LineupInstance.LineupActor)
		{
			FCrowdCharacterDefinition CharacterDefinition;
			PopulateCharacterDefinition(LineupInstance.InstanceOptions, CharacterDefinition);

			{
				FEditorScriptExecutionGuard ScriptGuard;

				UpdateLineupActor(LineupInstance.LineupActor, LineupInstance.LineupCoordinates, CharacterDefinition);
			}
		}
	}	
	UpdateInstanceTransforms();
}

void ACrowdCharacterLineup::SpawnCharacterFromOptions(const FString VariantLabel, const FIntVector LineupCoordinates, const FCrowdCharacterOptions& CharacterOptions)
{
	if (UWorld* TargetWorld = GetWorld())
	{
		FCrowdCharacterDefinition CharacterDefinition;
		PopulateCharacterDefinition(CharacterOptions, CharacterDefinition);

		FVector SpawnLocation = LocationOffset + (FVector(LineupCoordinates) * Spacing);
		AActor* NewActor = SpawnLineupActor(LineupCoordinates, VariantLabel, SpawnLocation, RotationOffset, CharacterDefinition, CharacterOptions);
		if (NewActor)
		{
			LineupInstances.Emplace(NewActor, LineupCoordinates, CharacterOptions);

			FAttachmentTransformRules AttachmentTransformRules(EAttachmentRule::KeepRelative, false);
			NewActor->AttachToActor(this, AttachmentTransformRules);


		}
	}
}

void ACrowdCharacterLineup::ResolveVariationDependencies(FCrowdVariationSpecifier& FirstVariation, FCrowdVariationSpecifier& SecondVariation, FCrowdVariationSpecifier& ThirdVariation)
{
	const bool& bX = VariationOptions.bX;
	const bool& bY = VariationOptions.bY;
	const bool& bZ = VariationOptions.bZ;
	const ECrowdLineupVariation& X_Variation = VariationOptions.X_Variation;
	const ECrowdLineupVariation& Y_Variation = VariationOptions.Y_Variation;
	const ECrowdLineupVariation& Z_Variation = VariationOptions.Z_Variation;

	// Only 6 permutations so easier to compute explicitly
	if (IsDependant(X_Variation, Y_Variation))
	{
		// Y > X
		if (IsDependant(Y_Variation, Z_Variation))
		{
			// Z > Y > X
			FirstVariation = { bZ, EAxis::Z, Z_Variation };
			SecondVariation = { bY, EAxis::Y, Y_Variation };
			ThirdVariation = { bX, EAxis::X, X_Variation };
		}
		else
		{
			// Y > X && Y >= Z
			if (IsDependant(X_Variation, Z_Variation))
			{
				// Y >= Z > X
				FirstVariation = { bY, EAxis::Y, Y_Variation };
				SecondVariation = { bZ, EAxis::Z, Z_Variation };
				ThirdVariation = { bX, EAxis::X, X_Variation };
			}
			else
			{
				// Y > X >= Z
				FirstVariation = { bY, EAxis::Y, Y_Variation };
				SecondVariation = { bX, EAxis::X, X_Variation };
				ThirdVariation = { bZ, EAxis::Z, Z_Variation };
			}
		}
	}
	else
	{
		// X >= Y
		if (IsDependant(X_Variation, Z_Variation))
		{
			// Z > X >= Y
			FirstVariation = { bZ, EAxis::Z, Z_Variation };
			SecondVariation = { bX, EAxis::X, X_Variation };
			ThirdVariation = { bY, EAxis::Y, Y_Variation };
		}
		else
		{
			// X >= Y && X >= Z
			if (IsDependant(Y_Variation, Z_Variation))
			{
				// X >= Z > Y
				FirstVariation = { bX, EAxis::X, X_Variation };
				SecondVariation = { bZ, EAxis::Z, Z_Variation };
				ThirdVariation = { bY, EAxis::Y, Y_Variation };
			}
			else
			{
				// X >= Y >= Z
				FirstVariation = { bX, EAxis::X, X_Variation };
				SecondVariation = { bY, EAxis::Y, Y_Variation }; 
				ThirdVariation = { bZ, EAxis::Z, Z_Variation };
			}
		}
	}
}

void ACrowdCharacterLineup::BuildLineup()
{
	if (!CharacterDataAsset)
	{
		return;
	}

	FScopedSlowTask SlowTask(1.f, FText::FromString(TEXT("Building Lineup...")));

	ClearLineup();

	if (LineupType == ECrowdLineupType::Variation)
	{
		// The properties we are vary could be dependent on each other such as the number of outfits changing based on gender
		// as such we change the order we vary the properties such that all dependent properties are evaluated after the property they depend upon
		FCrowdVariationSpecifier FirstVariation, SecondVariation, ThirdVariation;
		ResolveVariationDependencies(FirstVariation, SecondVariation, ThirdVariation);

		// Generate the base character definition
		FCrowdCharacterOptions CurrentCharacterOptions = BaseOptions;

		TArray<FString> LabelParts;

		int MaxFirst = FirstVariation.bEnabled ? GetNumberOfVariants(CurrentCharacterOptions, FirstVariation.Variation) : 1;
		for (int FirstIndex = 0; FirstIndex < MaxFirst; ++FirstIndex)
		{
			if (FirstVariation.bEnabled)
			{
				SetCharacterOptionsEntry(CurrentCharacterOptions, FirstVariation.Variation, FirstIndex);

				uint32 VariationIndex = static_cast<uint32>(FirstVariation.Variation);
				FString VariationName = StaticEnum<ECrowdLineupVariation>()->GetNameStringByIndex(VariationIndex);
				LabelParts.Add(FString::Printf(TEXT("%s: %d"), *VariationName, FirstIndex));
			}

			int MaxSecond = SecondVariation.bEnabled ? GetNumberOfVariants(CurrentCharacterOptions, SecondVariation.Variation) : 1;
			for (int SecondIndex = 0; SecondIndex < MaxSecond; ++SecondIndex)
			{
				if (SecondVariation.bEnabled)
				{
					SetCharacterOptionsEntry(CurrentCharacterOptions, SecondVariation.Variation, SecondIndex);

					uint32 VariationIndex = static_cast<uint32>(SecondVariation.Variation);
					FString VariationName = StaticEnum<ECrowdLineupVariation>()->GetNameStringByIndex(VariationIndex);
					LabelParts.Add(FString::Printf(TEXT("%s: %d"), *VariationName, SecondIndex));
				}

				int MaxThird = ThirdVariation.bEnabled ? GetNumberOfVariants(CurrentCharacterOptions, ThirdVariation.Variation) : 1;
				for (int ThirdIndex = 0; ThirdIndex < MaxThird; ++ThirdIndex)
				{
					if (ThirdVariation.bEnabled)
					{
						SetCharacterOptionsEntry(CurrentCharacterOptions, ThirdVariation.Variation, ThirdIndex);

						uint32 VariationIndex = static_cast<uint32>(SecondVariation.Variation);
						FString VariationName = StaticEnum<ECrowdLineupVariation>()->GetNameStringByIndex(VariationIndex);
						LabelParts.Add(FString::Printf(TEXT("%s: %d"), *VariationName, SecondIndex));
					}

					FString VariantLabel = FString::Join(LabelParts, TEXT("\n"));

					FIntVector LineupCoordinates =
						FirstVariation.GetOffsetDirection(FirstIndex) +
						SecondVariation.GetOffsetDirection(SecondIndex) +
						ThirdVariation.GetOffsetDirection(ThirdIndex);

					SpawnCharacterFromOptions(VariantLabel, LineupCoordinates, CurrentCharacterOptions);
				}
			}
		}
	}
	else if (LineupType == ECrowdLineupType::Random)
	{
		// Convert the array to a set
		TSet<ECrowdLineupVariation> FixedOptionsSet(RandomOptions.FixedOptions);
		
		for (int X_Index = 0; X_Index < RandomOptions.LineupSize.X; ++X_Index)
		{
			for (int Y_Index = 0; Y_Index < RandomOptions.LineupSize.Y; ++Y_Index)
			{
				for (int Z_Index = 0; Z_Index < RandomOptions.LineupSize.Z; ++Z_Index)
				{
					FCrowdCharacterOptions RandomCharacterOptions = BaseOptions;
					RandomCharacterOptions.Randomize(*CharacterDataAsset, FixedOptionsSet);

					FIntVector LineupCoordinates(X_Index, Y_Index, Z_Index);
					FString VariantLabel = LineupCoordinates.ToString();

					SpawnCharacterFromOptions(VariantLabel, LineupCoordinates, RandomCharacterOptions);
				}
			}
		}
	}
}

bool ACrowdCharacterLineup::GetParentVariation(ECrowdLineupVariation ChildVariation, ECrowdLineupVariation& ParentVariation)
{
	switch (ChildVariation)
	{
	case ECrowdLineupVariation::Skeleton:
	case ECrowdLineupVariation::HairColor:
		return false;
	case ECrowdLineupVariation::BodyType:
	case ECrowdLineupVariation::Head:
	case ECrowdLineupVariation::OutfitMaterial:
	case ECrowdLineupVariation::Hair:
	case ECrowdLineupVariation::Eyebrows:
	case ECrowdLineupVariation::Fuzz:
	case ECrowdLineupVariation::Eyelashes:
	case ECrowdLineupVariation::Mustache:
	case ECrowdLineupVariation::Beard:
	case ECrowdLineupVariation::SkinTexture:
	case ECrowdLineupVariation::Accessory:
	{
		ParentVariation = ECrowdLineupVariation::Skeleton;
		return true;
	}
	case ECrowdLineupVariation::Outfit:
	case ECrowdLineupVariation::ScaleFactor:
	{
		ParentVariation = ECrowdLineupVariation::BodyType;
		return true;
	}
	case ECrowdLineupVariation::SkinTextureModifier:
	{
		ParentVariation = ECrowdLineupVariation::SkinTexture;
		return true;
	}
	case ECrowdLineupVariation::PatternColor:
	case ECrowdLineupVariation::PatternOption:
	{
		ParentVariation = ECrowdLineupVariation::OutfitMaterial;
		return true;
	}	
	default:
	{
		checkNoEntry();
		return false;
	}
	}
}

bool ACrowdCharacterLineup::IsDependant(ECrowdLineupVariation FirstVariation, ECrowdLineupVariation SecondVariation)
{
	ECrowdLineupVariation ParentVariation;
	bool bHasParent = GetParentVariation(FirstVariation, ParentVariation);
	while (bHasParent)
	{
		if (ParentVariation == SecondVariation)
		{
			return true;
		}

		bHasParent = GetParentVariation(ParentVariation, ParentVariation);
	}

	return false;
}

int ACrowdCharacterLineup::GetNumberOfVariants(const FCrowdCharacterOptions& CharacterOptions, ECrowdLineupVariation VariantType) const
{
	switch (VariantType)
	{
	case ECrowdLineupVariation::Skeleton:
		return 2;
	case ECrowdLineupVariation::BodyType:
		return 3;
	case ECrowdLineupVariation::Head:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		FCrowdBodyOutfitDefinition& BodyOutfitDefinition = CharacterOptions.BodyType == ECitySampleCrowdBodyType::NormalWeight ?
			GenderDefinition.NormalWeight : CharacterOptions.BodyType == ECitySampleCrowdBodyType::OverWeight ? GenderDefinition.OverWeight : GenderDefinition.UnderWeight;

		return BodyOutfitDefinition.HeadsData.Num();
	}
	case ECrowdLineupVariation::Outfit:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		FCrowdBodyOutfitDefinition& BodyOutfitDefinition = CharacterOptions.BodyType == ECitySampleCrowdBodyType::NormalWeight ?
			GenderDefinition.NormalWeight : CharacterOptions.BodyType == ECitySampleCrowdBodyType::OverWeight ? GenderDefinition.OverWeight : GenderDefinition.UnderWeight;

		return BodyOutfitDefinition.Outfits.Num();
	}
	case ECrowdLineupVariation::OutfitMaterial:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		return GenderDefinition.OutfitMaterials.Num();
	}
	case ECrowdLineupVariation::Hair:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		uint8 SlotIdx = static_cast<uint8>(ECrowdHairSlots::Hair);
		return GenderDefinition.HairSlots.IsValidIndex(SlotIdx) ? GenderDefinition.HairSlots[SlotIdx].HairDefinitions.Num() : 0;
	}
	case ECrowdLineupVariation::Eyebrows:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		uint8 SlotIdx = static_cast<uint8>(ECrowdHairSlots::Eyebrows);
		return GenderDefinition.HairSlots.IsValidIndex(SlotIdx) ? GenderDefinition.HairSlots[SlotIdx].HairDefinitions.Num() : 0;
	}
	case ECrowdLineupVariation::Fuzz:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		uint8 SlotIdx = static_cast<uint8>(ECrowdHairSlots::Fuzz);
		return GenderDefinition.HairSlots.IsValidIndex(SlotIdx) ? GenderDefinition.HairSlots[SlotIdx].HairDefinitions.Num() : 0;
	}
	case ECrowdLineupVariation::Eyelashes:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		uint8 SlotIdx = static_cast<uint8>(ECrowdHairSlots::Eyelashes);
		return GenderDefinition.HairSlots.IsValidIndex(SlotIdx) ? GenderDefinition.HairSlots[SlotIdx].HairDefinitions.Num() : 0;
	}
	case ECrowdLineupVariation::Mustache:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		uint8 SlotIdx = static_cast<uint8>(ECrowdHairSlots::Mustache);
		return GenderDefinition.HairSlots.IsValidIndex(SlotIdx) ? GenderDefinition.HairSlots[SlotIdx].HairDefinitions.Num() : 0;
	}
	case ECrowdLineupVariation::Beard:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		uint8 SlotIdx = static_cast<uint8>(ECrowdHairSlots::Beard);
		return GenderDefinition.HairSlots.IsValidIndex(SlotIdx) ? GenderDefinition.HairSlots[SlotIdx].HairDefinitions.Num() : 0;
	}
	case ECrowdLineupVariation::HairColor:
	{
		return CharacterDataAsset->HairColors.Num();
	}
	case ECrowdLineupVariation::SkinTexture:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		return GenderDefinition.SkinMaterials.Num();
	}
	case ECrowdLineupVariation::SkinTextureModifier:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;

		if (GenderDefinition.SkinMaterials.IsValidIndex(CharacterOptions.SkinTextureIndex))
		{
			return GenderDefinition.SkinMaterials[CharacterOptions.SkinTextureIndex].TextureModifiers.Num();
		}
		else
		{
			return 0;
		}
	}
	case ECrowdLineupVariation::Accessory:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		FCrowdBodyOutfitDefinition& BodyOutfitDefinition = CharacterOptions.BodyType == ECitySampleCrowdBodyType::NormalWeight ?
			GenderDefinition.NormalWeight : CharacterOptions.BodyType == ECitySampleCrowdBodyType::OverWeight ? GenderDefinition.OverWeight : GenderDefinition.UnderWeight;
		return BodyOutfitDefinition.Accessories.Num();
	}
	case ECrowdLineupVariation::ScaleFactor:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		FCrowdBodyOutfitDefinition& BodyOutfitDefinition = CharacterOptions.BodyType == ECitySampleCrowdBodyType::NormalWeight ?
			GenderDefinition.NormalWeight : CharacterOptions.BodyType == ECitySampleCrowdBodyType::OverWeight ? GenderDefinition.OverWeight : GenderDefinition.UnderWeight;
		return BodyOutfitDefinition.ScaleFactors.Num();
	}
	case ECrowdLineupVariation::PatternColor:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		if (GenderDefinition.OutfitMaterials.IsValidIndex(CharacterOptions.OutfitMaterialIndex))
		{
			// We'll use the max value from all the possible overrides
			const TMap<FName, FCrowdMaterialOverride>& MaterialOverrides = GenderDefinition.OutfitMaterials[CharacterOptions.OutfitMaterialIndex].MaterialOverrides;
			
			int MaxPatternColors = 1;
			for (const TPair<FName, FCrowdMaterialOverride>& MaterialOverride : MaterialOverrides)
			{
				if (MaterialOverride.Value.PatternUsage == ECrowdPatternUsage::PatternList)
				{
					MaxPatternColors = FMath::Max(MaterialOverride.Value.ComplimentaryColors.Num(), MaxPatternColors);
				}
			}
			return MaxPatternColors;
		}
		else
		{
			return 1;
		}
	}
	case ECrowdLineupVariation::PatternOption:
	{
		FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CharacterDataAsset->SkeletonA : CharacterDataAsset->SkeletonB;
		if (GenderDefinition.OutfitMaterials.IsValidIndex(CharacterOptions.OutfitMaterialIndex))
		{
			// We'll use the max value from all the possible overrides
			const TMap<FName, FCrowdMaterialOverride>& MaterialOverrides = GenderDefinition.OutfitMaterials[CharacterOptions.OutfitMaterialIndex].MaterialOverrides;

			// We use 1 as the base case so at least 1 character is rendered
			int MaxPatternOptions = 1;
			for (const TPair<FName, FCrowdMaterialOverride>& MaterialOverride : MaterialOverrides)
			{
				if (MaterialOverride.Value.PatternUsage == ECrowdPatternUsage::PatternList)
				{
					MaxPatternOptions = FMath::Max(MaterialOverride.Value.Patterns.Num(), MaxPatternOptions);
				}
			}
			return MaxPatternOptions;
		}
		else
		{
			return 1;
		}
	}
	default:
	{
		checkNoEntry();
		return 0;
	}
	}
}

void ACrowdCharacterLineup::PopulateCharacterDefinition(const FCrowdCharacterOptions& CharacterOptions, FCrowdCharacterDefinition& CharacterDefinition)
{
	CharacterOptions.GenerateCharacterDefinition(CharacterDataAsset, CharacterDefinition);
}

int GetCharacterOptionsEntry(const FCrowdCharacterOptions& CharacterOptions, const ECrowdLineupVariation VariationType)
{
	switch (VariationType)
	{
	case ECrowdLineupVariation::Skeleton:
		return static_cast<int>(CharacterOptions.Skeleton);
	case ECrowdLineupVariation::BodyType:
		return static_cast<int>(CharacterOptions.BodyType);
	case ECrowdLineupVariation::Head:
		return CharacterOptions.HeadIndex;
	case ECrowdLineupVariation::Outfit:
		return CharacterOptions.OutfitIndex;
	case ECrowdLineupVariation::OutfitMaterial:
		return CharacterOptions.OutfitMaterialIndex;
	case ECrowdLineupVariation::Hair:
		return CharacterOptions.HairIndex;
	case ECrowdLineupVariation::Eyebrows:
		return CharacterOptions.EyebrowsIndex;
	case ECrowdLineupVariation::Fuzz:
		return CharacterOptions.FuzzIndex;
	case ECrowdLineupVariation::Eyelashes:
		return CharacterOptions.MustacheIndex;
	case ECrowdLineupVariation::Mustache:
		return CharacterOptions.MustacheIndex;
	case ECrowdLineupVariation::Beard:
		return CharacterOptions.BeardIndex;
	case ECrowdLineupVariation::HairColor:
		return CharacterOptions.HairColorIndex;
	case ECrowdLineupVariation::SkinTexture:
		return CharacterOptions.SkinTextureIndex;
	case ECrowdLineupVariation::SkinTextureModifier:
		return CharacterOptions.SkinTextureModifierIndex;
	case ECrowdLineupVariation::Accessory:
		return CharacterOptions.AccessoryIndex;
	case ECrowdLineupVariation::ScaleFactor:
		return CharacterOptions.ScaleFactorIndex;
	case ECrowdLineupVariation::PatternColor:
		return CharacterOptions.PatternColorIndex;
	case ECrowdLineupVariation::PatternOption:
		return CharacterOptions.PatternOptionIndex;
	default:
	{
		checkNoEntry();
		return 0;
	}
	}
}

void ACrowdCharacterLineup::SetCharacterOptionsEntry(FCrowdCharacterOptions& CharacterOptions, const ECrowdLineupVariation VariationType, const int VariantValue)
{
	if (VariationType == ECrowdLineupVariation::Skeleton)
	{
		CharacterOptions.Skeleton = ECitySampleCrowdGender(VariantValue);
	}
	else if (VariationType == ECrowdLineupVariation::BodyType)
	{
		CharacterOptions.BodyType = ECitySampleCrowdBodyType(VariantValue);
	}
	else if (VariationType == ECrowdLineupVariation::Head)
	{
		CharacterOptions.HeadIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Outfit)
	{
		CharacterOptions.OutfitIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::OutfitMaterial)
	{
		CharacterOptions.OutfitMaterialIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Hair)
	{
		CharacterOptions.HairIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Eyebrows)
	{
		CharacterOptions.EyebrowsIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Fuzz)
	{
		CharacterOptions.FuzzIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Eyelashes)
	{
		CharacterOptions.EyelashesIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Mustache)
	{
		CharacterOptions.MustacheIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Beard)
	{
		CharacterOptions.BeardIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::HairColor)
	{
		CharacterOptions.HairColorIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::SkinTexture)
	{
		CharacterOptions.SkinTextureIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::SkinTextureModifier)
	{
		CharacterOptions.SkinTextureModifierIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::Accessory)
	{
		CharacterOptions.AccessoryIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::ScaleFactor)
	{
		CharacterOptions.ScaleFactorIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::PatternColor)
	{
		CharacterOptions.PatternColorIndex = VariantValue;
	}
	else if (VariationType == ECrowdLineupVariation::PatternOption)
	{
		CharacterOptions.PatternOptionIndex = VariantValue;
	}
	else
	{
		checkNoEntry();
	}
}

void ACrowdCharacterLineup::UpdateInstanceTransforms()
{
	for (FCrowdLineupInstance& LineupInstance : LineupInstances)
	{
		AActor* LineupActor = LineupInstance.LineupActor;
		if (IsValid(LineupActor))
		{
			FVector CurrentScale = LineupActor->GetActorRelativeScale3D();

			FTransform NewTransform(
				RotationOffset,
				LocationOffset + (Spacing * FVector(LineupInstance.LineupCoordinates)),
				CurrentScale
			);

			LineupActor->SetActorRelativeTransform(NewTransform);
		}
	}
}
