// Copyright Epic Games, Inc. All Rights Reserved.
#include "CrowdCharacterDataAsset.h"

#include "CrowdBlueprintLibrary.h"

FCrowdCharacterDefinition UCrowdCharacterDataAsset::GetCharacterDefinition(
	const ECitySampleCrowdGender Skeleton, const ECitySampleCrowdBodyType BodyType, 
	const int32 HeadIndex, const int32 OutfitIndex, const int32 OutfitMaterialIndex, const int32 HairIndex, const int32 HairColorIndex,
	const int32 AccessoryIndex, const int32 SkinTextureIndex, const int32 SkinTextureModifierIndex, 
	const int32 ScaleFactorIndex) const
{
	FCrowdCharacterDefinition CharacterDefinition;

	if (Skeleton == ECitySampleCrowdGender::A)
	{	
		if (BodyType == ECitySampleCrowdBodyType::NormalWeight)
		{
			const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = this->SkeletonA.NormalWeight;
			
			// Get Body Definition
			CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

			// Get Outfit Definition
			if (!BodyOutfitDefinition.Outfits.IsEmpty())
			{
				const int32 Index = OutfitIndex % BodyOutfitDefinition.Outfits.Num();
				if (BodyOutfitDefinition.Outfits.IsValidIndex(Index))
				{
					CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[Index];
				}
			}

			// Get Scale Factor
			const TArray<float>& ScaleFactors = BodyOutfitDefinition.ScaleFactors;
			if (!ScaleFactors.IsEmpty())
			{
				const int32 Index = ScaleFactorIndex % ScaleFactors.Num();
				if (ScaleFactors.IsValidIndex(Index))
				{
					CharacterDefinition.ScaleFactor = ScaleFactors[Index];
				}
			}

			// Get Head Definition
			CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(BodyOutfitDefinition.HeadsData, HeadIndex);

			// Get Accessory Definition
			if (!BodyOutfitDefinition.Accessories.IsEmpty())
			{
				const int32 Index = AccessoryIndex % BodyOutfitDefinition.Accessories.Num();
				if (BodyOutfitDefinition.Accessories.IsValidIndex(Index))
				{
					CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[Index];
				}
			}
		}
		else if (BodyType == ECitySampleCrowdBodyType::OverWeight)
		{
			const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = this->SkeletonA.OverWeight;

			// Get Body Definition
			CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

			// Get Outfit Definition
			if (!BodyOutfitDefinition.Outfits.IsEmpty())
			{
				const int32 Index = OutfitIndex % BodyOutfitDefinition.Outfits.Num();
				if (BodyOutfitDefinition.Outfits.IsValidIndex(Index))
				{
					CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[Index];
				}
			}

			// Get Scale Factor
			const TArray<float>& ScaleFactors = BodyOutfitDefinition.ScaleFactors;
			if (!ScaleFactors.IsEmpty())
			{
				const int32 Index = ScaleFactorIndex % ScaleFactors.Num();
				if (ScaleFactors.IsValidIndex(Index))
				{
					CharacterDefinition.ScaleFactor = ScaleFactors[Index];
				}
			}

			// Get Head Definition
			CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(BodyOutfitDefinition.HeadsData, HeadIndex);

			// Get Accessory Definition
			if (!BodyOutfitDefinition.Accessories.IsEmpty())
			{
				const int32 Index = AccessoryIndex % BodyOutfitDefinition.Accessories.Num();
				if (BodyOutfitDefinition.Accessories.IsValidIndex(Index))
				{
					CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[Index];
				}
			}
		}
		else if (BodyType == ECitySampleCrowdBodyType::UnderWeight)
		{
			const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = this->SkeletonA.UnderWeight;

			// Get Body Definition
			CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

			// Get Outfit Definition
			if (!BodyOutfitDefinition.Outfits.IsEmpty())
			{
				const int32 Index = OutfitIndex % BodyOutfitDefinition.Outfits.Num();
				if (BodyOutfitDefinition.Outfits.IsValidIndex(Index))
				{
					CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[Index];
				}
			}

			// Get Scale Factor
			const TArray<float>& ScaleFactors = BodyOutfitDefinition.ScaleFactors;
			if (!ScaleFactors.IsEmpty())
			{
				const int32 Index = ScaleFactorIndex % ScaleFactors.Num();
				if (ScaleFactors.IsValidIndex(Index))
				{
					CharacterDefinition.ScaleFactor = ScaleFactors[Index];
				}
			}

			// Get Head Definition
			CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(BodyOutfitDefinition.HeadsData, HeadIndex);

			// Get Accessory Definition
			if (!BodyOutfitDefinition.Accessories.IsEmpty())
			{
				const int32 Index = AccessoryIndex % BodyOutfitDefinition.Accessories.Num();
				if (BodyOutfitDefinition.Accessories.IsValidIndex(Index))
				{
					CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[Index];
				}
			}
		}

		// Get Hair Definition
		// For now we just have a single hair index and we're indexing each slot by the same value
		// however in the future this will move to more individual control
		for (int SlotIdx = 0; SlotIdx < this->SkeletonA.HairSlots.Num(); ++SlotIdx)
		{
			const FCrowdHairSlot& HairSlot = this->SkeletonA.HairSlots[SlotIdx];

			if (!HairSlot.HairDefinitions.IsEmpty())
			{
				CharacterDefinition.HairDefinitions[SlotIdx] = HairSlot.HairDefinitions[HairIndex % HairSlot.HairDefinitions.Num()];
			}
		}

		// Get Outfit Material
		if (!this->SkeletonA.OutfitMaterials.IsEmpty())
		{
			const int32 Index = OutfitMaterialIndex % this->SkeletonA.OutfitMaterials.Num();
			if (this->SkeletonA.OutfitMaterials.IsValidIndex(Index))
			{
				CharacterDefinition.OutfitMaterialDefinition = this->SkeletonA.OutfitMaterials[Index];
			}
		}

		// Get Head Texture 
		if (!this->SkeletonA.SkinMaterials.IsEmpty())
		{
			const int32 Index = SkinTextureIndex % this->SkeletonA.SkinMaterials.Num();
			if (this->SkeletonA.SkinMaterials.IsValidIndex(Index))
			{
				CharacterDefinition.SkinTextureDefinition = this->SkeletonA.SkinMaterials[Index].Texture;

				// Get Texture Definition
				if (!this->SkeletonA.SkinMaterials[Index].TextureModifiers.IsEmpty())
				{
					const int32 IndexTM = SkinTextureModifierIndex % this->SkeletonA.SkinMaterials[Index].TextureModifiers.Num();
					if (this->SkeletonA.SkinMaterials[Index].TextureModifiers.IsValidIndex(IndexTM))
					{
						CharacterDefinition.SkinTextureModifierDefinition = this->SkeletonA.SkinMaterials[Index].TextureModifiers[IndexTM];
					}
				}
			}
		}
	}

	else if (Skeleton == ECitySampleCrowdGender::B)
	{
		if (BodyType == ECitySampleCrowdBodyType::NormalWeight)
		{
			const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = this->SkeletonB.NormalWeight;

			// Get Body Definition
			CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

			// Get Outfit Definition
			if (!BodyOutfitDefinition.Outfits.IsEmpty())
			{
				const int32 Index = OutfitIndex % BodyOutfitDefinition.Outfits.Num();
				CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[Index];
			}

			// Get Scale Factor
			const TArray<float>& ScaleFactors = BodyOutfitDefinition.ScaleFactors;
			if (!ScaleFactors.IsEmpty())
			{
				const int32 Index = ScaleFactorIndex % ScaleFactors.Num();
				if (ScaleFactors.IsValidIndex(Index))
				{
					CharacterDefinition.ScaleFactor = ScaleFactors[Index];
				}
			}

			// Get Head Definition
			CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(BodyOutfitDefinition.HeadsData, HeadIndex);

			// Get Accessory Definition
			if (!BodyOutfitDefinition.Accessories.IsEmpty())
			{
				const int32 Index = AccessoryIndex % BodyOutfitDefinition.Accessories.Num();
				if (BodyOutfitDefinition.Accessories.IsValidIndex(Index))
				{
					CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[Index];
				}
			}
		}
		else if (BodyType == ECitySampleCrowdBodyType::OverWeight)
		{
			const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = this->SkeletonB.OverWeight;

			// Get Body Definition
			CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

			// Get Outfit Definition
			if (!BodyOutfitDefinition.Outfits.IsEmpty())
			{
				const int32 Index = OutfitIndex % BodyOutfitDefinition.Outfits.Num();
				CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[Index];
			}

			// Get Scale Factor
			const TArray<float>& ScaleFactors = BodyOutfitDefinition.ScaleFactors;
			if (!ScaleFactors.IsEmpty())
			{
				const int32 Index = ScaleFactorIndex % ScaleFactors.Num();
				if (ScaleFactors.IsValidIndex(Index))
				{
					CharacterDefinition.ScaleFactor = ScaleFactors[Index];
				}
			}

			// Get Head Definition
			CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(BodyOutfitDefinition.HeadsData, HeadIndex);

			// Get Accessory Definition
			if (!BodyOutfitDefinition.Accessories.IsEmpty())
			{
				const int32 Index = AccessoryIndex % BodyOutfitDefinition.Accessories.Num();
				if (BodyOutfitDefinition.Accessories.IsValidIndex(Index))
				{
					CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[Index];
				}
			}
		}
		else if (BodyType == ECitySampleCrowdBodyType::UnderWeight)
		{
			const FCrowdBodyOutfitDefinition& BodyOutfitDefinition = this->SkeletonB.UnderWeight;

			// Get Body Definition
			CharacterDefinition.BodyDefinition = BodyOutfitDefinition.Body;

			// Get Outfit Definition
			if (!BodyOutfitDefinition.Outfits.IsEmpty())
			{
				const int32 Index = OutfitIndex % BodyOutfitDefinition.Outfits.Num();
				CharacterDefinition.OutfitDefinition = BodyOutfitDefinition.Outfits[Index];
			}

			// Get Scale Factor
			const TArray<float>& ScaleFactors = BodyOutfitDefinition.ScaleFactors;
			if (!ScaleFactors.IsEmpty())
			{
				const int32 Index = ScaleFactorIndex % ScaleFactors.Num();
				if (ScaleFactors.IsValidIndex(Index))
				{
					CharacterDefinition.ScaleFactor = ScaleFactors[Index];
				}
			}

			// Get Head Definition
			CharacterDefinition.Head = UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(BodyOutfitDefinition.HeadsData, HeadIndex);

			// Get Accessory Definition
			if (!BodyOutfitDefinition.Accessories.IsEmpty())
			{
				const int32 Index = AccessoryIndex % BodyOutfitDefinition.Accessories.Num();
				if (BodyOutfitDefinition.Accessories.IsValidIndex(Index))
				{
					CharacterDefinition.AccessoryDefinition = BodyOutfitDefinition.Accessories[Index];
				}
			}
		}

		// Get Hair Definition
		// For now we just have a single hair index and we're indexing each slot by the same value
		// however in the future this will move to more individual control
		for (int SlotIdx = 0; SlotIdx < this->SkeletonB.HairSlots.Num(); ++SlotIdx)
		{
			const FCrowdHairSlot& HairSlot = this->SkeletonB.HairSlots[SlotIdx];

			if (!HairSlot.HairDefinitions.IsEmpty())
			{
				CharacterDefinition.HairDefinitions[SlotIdx] = HairSlot.HairDefinitions[HairIndex % HairSlot.HairDefinitions.Num()];
			}
		}

		// Get Outfit Material
		if (!this->SkeletonB.OutfitMaterials.IsEmpty())
		{
			const int32 Index = OutfitMaterialIndex % this->SkeletonB.OutfitMaterials.Num();
			if (this->SkeletonB.OutfitMaterials.IsValidIndex(Index))
			{
				CharacterDefinition.OutfitMaterialDefinition = this->SkeletonB.OutfitMaterials[Index];
			}
		}

		// Get Texture 
		if (!this->SkeletonB.SkinMaterials.IsEmpty())
		{
			const int32 Index = SkinTextureIndex % this->SkeletonB.SkinMaterials.Num();
			if (this->SkeletonB.SkinMaterials.IsValidIndex(Index))
			{
				CharacterDefinition.SkinTextureDefinition = this->SkeletonB.SkinMaterials[Index].Texture;

				// Get Texture Definition
				if (!this->SkeletonB.SkinMaterials[Index].TextureModifiers.IsEmpty())
				{
					const int32 IndexTM = SkinTextureModifierIndex % this->SkeletonB.SkinMaterials[Index].TextureModifiers.Num();
					if (this->SkeletonB.SkinMaterials[Index].TextureModifiers.IsValidIndex(IndexTM))
					{
						CharacterDefinition.SkinTextureModifierDefinition = this->SkeletonB.SkinMaterials[Index].TextureModifiers[IndexTM];
					}
				}
			}
		}
	}

	// Get Hair Color Definition
	if (!this->HairColors.IsEmpty())
	{
		const int32 Index = HairColorIndex % this->HairColors.Num();
		if (this->HairColors.IsValidIndex(Index))
		{
			CharacterDefinition.HairColorDefinition = this->HairColors[Index];
		}
	}

	return CharacterDefinition;
}


FCrowdCharacterDefinition  UCrowdCharacterDataAsset::GetCharacterDefinitionFromIndex(const int32 GlobalIndex) const
{
	const TArray<ECitySampleCrowdGender> Genders = { ECitySampleCrowdGender::A, ECitySampleCrowdGender::B };
	const TArray<ECitySampleCrowdBodyType> BodyTypes = { ECitySampleCrowdBodyType::NormalWeight }; // , ECitySampleCrowdBodyType::OverWeight, ECitySampleCrowdBodyType::UnderWeight };

	const int32 GenderIndex   = GlobalIndex % Genders.Num();
	const int32 BodyTypeIndex = GlobalIndex % BodyTypes.Num();

	return GetCharacterDefinition(Genders[GenderIndex], BodyTypes[BodyTypeIndex],
		GlobalIndex, GlobalIndex, GlobalIndex, GlobalIndex, GlobalIndex, GlobalIndex, GlobalIndex, GlobalIndex, GlobalIndex);
}

// ----------------------------------------------------------------------------------------------------------------

TArray<UAnimToTextureDataAsset*> UCrowdCharacterDataAsset::FindOutfitDataAssets(const bool bMale, const bool bFemale) const
{	
	// Get All Outfits For all Genders and BodyTypes
	TArray<FCrowdOutfitDefinition> OutfitDefinitions;
	if (bMale)
	{
		OutfitDefinitions.Append(this->SkeletonA.NormalWeight.Outfits);
		OutfitDefinitions.Append(this->SkeletonA.OverWeight.Outfits);
		OutfitDefinitions.Append(this->SkeletonA.UnderWeight.Outfits);
	}

	if (bFemale)
	{
		OutfitDefinitions.Append(this->SkeletonB.NormalWeight.Outfits);
		OutfitDefinitions.Append(this->SkeletonB.OverWeight.Outfits);
		OutfitDefinitions.Append(this->SkeletonB.UnderWeight.Outfits);
	}

	// 
	TArray<UAnimToTextureDataAsset*> DataAssets;
	for (const FCrowdOutfitDefinition& OutfitDefinition: OutfitDefinitions)
	{
		UAnimToTextureDataAsset* TopData = OutfitDefinition.TopData.LoadSynchronous();
		UAnimToTextureDataAsset* BottomData = OutfitDefinition.BottomData.LoadSynchronous();
		UAnimToTextureDataAsset* ShoesData = OutfitDefinition.ShoesData.LoadSynchronous();

		if (TopData)
		{
			DataAssets.AddUnique(TopData);
		}

		if (BottomData)
		{
			DataAssets.AddUnique(BottomData);
		}

		if (ShoesData)
		{
			DataAssets.AddUnique(ShoesData);
		}
	};

	return DataAssets;
}

TArray<UAnimToTextureDataAsset*> UCrowdCharacterDataAsset::FindBodyDataAssets(const bool bMale, const bool bFemale) const
{	
	// Get All Bodies For all Genders and BodyTypes
	TArray<TSoftObjectPtr<UAnimToTextureDataAsset>> DataPtrArray;
	if (bMale)
	{
		DataPtrArray.Add(this->SkeletonA.NormalWeight.Body.BodyData);
		DataPtrArray.Add(this->SkeletonA.OverWeight.Body.BodyData);
		DataPtrArray.Add(this->SkeletonA.UnderWeight.Body.BodyData);
	}
	if (bFemale)
	{
		DataPtrArray.Add(this->SkeletonB.NormalWeight.Body.BodyData);
		DataPtrArray.Add(this->SkeletonB.OverWeight.Body.BodyData);
		DataPtrArray.Add(this->SkeletonB.UnderWeight.Body.BodyData);
	}

	TArray<UAnimToTextureDataAsset*> DataAssets;
	for (TSoftObjectPtr<UAnimToTextureDataAsset>& DataPtr : DataPtrArray)
	{
		UAnimToTextureDataAsset* Data = DataPtr.LoadSynchronous();
		if (Data)
		{
			DataAssets.AddUnique(Data);
		}
	}

	return DataAssets;
}

TArray<UAnimToTextureDataAsset*> UCrowdCharacterDataAsset::FindHeadDataAssets(const bool bMale, const bool bFemale) const
{
	// Get All Bodies For all Genders and BodyTypes
	TArray<TSoftObjectPtr<UAnimToTextureDataAsset>> DataPtrArray;

	if (bMale)
	{
		for (const FCrowdBodyOutfitDefinition& BodyOutfitDefinition : { this->SkeletonA.UnderWeight, this->SkeletonA.NormalWeight, this->SkeletonA.OverWeight })
		{
			DataPtrArray.Append(BodyOutfitDefinition.HeadsData);
		}
	}
	if (bFemale)
	{
		for (const FCrowdBodyOutfitDefinition& BodyOutfitDefinition : { this->SkeletonB.UnderWeight, this->SkeletonB.NormalWeight, this->SkeletonB.OverWeight })
		{
			DataPtrArray.Append(BodyOutfitDefinition.HeadsData);
		}
	}
	
	TArray<UAnimToTextureDataAsset*> DataAssets;
	for (TSoftObjectPtr<UAnimToTextureDataAsset>& DataPtr : DataPtrArray)
	{
		UAnimToTextureDataAsset* Data = DataPtr.LoadSynchronous();
		if (Data)
		{
			DataAssets.AddUnique(Data);
		}
	}

	return DataAssets;
}

TArray<UAnimToTextureDataAsset*> UCrowdCharacterDataAsset::FindDataAssets(const bool bMale, const bool bFemale) const
{
	TArray<UAnimToTextureDataAsset*> DataAssets;
	DataAssets.Append(FindOutfitDataAssets(bMale, bFemale));
	DataAssets.Append(FindBodyDataAssets(bMale, bFemale));
	DataAssets.Append(FindHeadDataAssets(bMale, bFemale));

	return DataAssets;
}

// ----------------------------------------------------------------------------------------------------------------

TArray<USkeletalMesh*> UCrowdCharacterDataAsset::FindOutfitSkeletalMeshes(const bool bMale, const bool bFemale) const
{
	// Get All Bodies For all Genders and BodyTypes
	TArray<UAnimToTextureDataAsset*> DataAssets = FindOutfitDataAssets(bMale, bFemale);

	// 
	TArray<USkeletalMesh*> SkeletalMeshes;
	for (UAnimToTextureDataAsset* DataAsset : DataAssets)
	{
		USkeletalMesh* SkeletalMesh = DataAsset->GetSkeletalMesh();
		if (SkeletalMesh)
		{
			SkeletalMeshes.AddUnique(SkeletalMesh);
		}
	};

	return SkeletalMeshes;
}

TArray<USkeletalMesh*> UCrowdCharacterDataAsset::FindBodySkeletalMeshes(const bool bMale, const bool bFemale) const
{
	// Get All Bodies For all Genders and BodyTypes
	TArray<UAnimToTextureDataAsset*> DataAssets = FindBodyDataAssets(bMale, bFemale);

	TArray<USkeletalMesh*> SkeletalMeshes;
	for (UAnimToTextureDataAsset* DataAsset : DataAssets)
	{
		USkeletalMesh* SkeletalMesh = DataAsset->GetSkeletalMesh();
		if (SkeletalMesh)
		{
			SkeletalMeshes.AddUnique(SkeletalMesh);
		}
	}

	return SkeletalMeshes;
}

TArray<USkeletalMesh*> UCrowdCharacterDataAsset::FindHeadSkeletalMeshes(const bool bMale, const bool bFemale) const
{
	TArray<UAnimToTextureDataAsset*> DataAssets = FindHeadDataAssets(bMale, bFemale);

	TArray<USkeletalMesh*> SkeletalMeshes;
	for (UAnimToTextureDataAsset* DataAsset : DataAssets)
	{
		USkeletalMesh* SkeletalMesh = DataAsset->GetSkeletalMesh();
		if (SkeletalMesh)
		{
			SkeletalMeshes.AddUnique(SkeletalMesh);
		}
	}

	return SkeletalMeshes;
}

TArray<USkeletalMesh*> UCrowdCharacterDataAsset::FindSkeletalMeshes(const bool bMale, const bool bFemale) const
{
	TArray<USkeletalMesh*> SkeletalMeshes;
	SkeletalMeshes.Append(FindOutfitSkeletalMeshes(bMale, bFemale));
	SkeletalMeshes.Append(FindBodySkeletalMeshes(bMale, bFemale));
	SkeletalMeshes.Append(FindHeadSkeletalMeshes(bMale, bFemale));

	return SkeletalMeshes;
}

// ----------------------------------------------------------------------------------------------------------------

TArray<UGroomAsset*> UCrowdCharacterDataAsset::FindGrooms(const bool bMale, const bool bFemale, 
	const bool bHair, const bool bEyebrows, const bool bFuzz, const bool bEyelashes, const bool bMustache, const bool bBeard) const
{
	TArray<FCrowdHairSlot> HairSlots;
	if (bMale)
	{ 
		HairSlots.Append(this->SkeletonA.HairSlots);
	}
	if (bFemale)
	{
		HairSlots.Append(this->SkeletonB.HairSlots);
	}

	TArray<UGroomAsset*> GroomArray;
	for (const FCrowdHairSlot& HairSlot : HairSlots)
	{
		#if WITH_EDITORONLY_DATA
			// Filters
			if ((HairSlot.SlotName == "Hair" && !bHair) || 
				(HairSlot.SlotName == "Eyebrows" && !bEyebrows) ||
				(HairSlot.SlotName == "Fuzz" && !bFuzz) ||
				(HairSlot.SlotName == "Eyelashes" && !bEyelashes) ||
				(HairSlot.SlotName == "Mustache" && !bMustache) ||
				(HairSlot.SlotName == "Beard" && !bBeard))
			{
				continue;
			}
		#endif

		for (const FCrowdHairDefinition& HairDefinition : HairSlot.HairDefinitions)
		{
			GroomArray.AddUnique(HairDefinition.Groom.LoadSynchronous());
		}
	}

	return GroomArray;
}

TArray<UStaticMesh*> UCrowdCharacterDataAsset::FindGroomMeshes(const bool bMale, const bool bFemale, const bool bHair, const bool bEyebrows, const bool bFuzz, const bool bEyelashes, const bool bMustache, const bool bBeard, 
	const int32 GroupIndex) const
{
	TArray<UGroomAsset*> Grooms = this->FindGrooms(bMale, bFemale, bHair, bEyebrows, bFuzz, bEyelashes, bMustache, bBeard);

	TArray<UStaticMesh*> Meshes;
	for (UGroomAsset* Groom: Grooms)
	{
		for (int32 Index = 0; Index < Groom->HairGroupsMeshes.Num(); ++Index)
		{
			if (GroupIndex == -1 || GroupIndex == Index)
			{
				UStaticMesh* Mesh = Groom->HairGroupsMeshes[Index].ImportedMesh;
				if (Mesh)
				{
					Meshes.AddUnique(Mesh);
				}
			}
		}
	}

	return Meshes;
}

TArray<UStaticMesh*> UCrowdCharacterDataAsset::FindAccesoryMeshes(const bool bMale, const bool bFemale) const
{
	TArray<FCrowdAccessoryDefinition> AccessoryDefinitions;
	
	if (bMale)
	{
		AccessoryDefinitions.Append(this->SkeletonA.UnderWeight.Accessories);
		AccessoryDefinitions.Append(this->SkeletonA.NormalWeight.Accessories);
		AccessoryDefinitions.Append(this->SkeletonA.OverWeight.Accessories);
	}
	if (bFemale)
	{
		AccessoryDefinitions.Append(this->SkeletonB.UnderWeight.Accessories);
		AccessoryDefinitions.Append(this->SkeletonB.NormalWeight.Accessories);
		AccessoryDefinitions.Append(this->SkeletonB.OverWeight.Accessories);
	}

	TArray<UStaticMesh*> Meshes;
	for (const FCrowdAccessoryDefinition& AccessoryDefinition : AccessoryDefinitions)
	{
		UStaticMesh* Mesh = AccessoryDefinition.Mesh.LoadSynchronous();
		if (Mesh)
		{
			Meshes.AddUnique(Mesh);
		}
	}

	return Meshes;
}

TArray<USkeletalMesh*> UCrowdCharacterDataAsset::FindBaseSkeletalMeshes(const bool bMale, const bool bFemale) const
{
	// Get All Bodies For all Genders and BodyTypes
	TArray<USkeletalMesh*> SkeletalMeshes;
	if (bMale)
	{
		SkeletalMeshes.AddUnique(this->SkeletonA.NormalWeight.Body.Base.LoadSynchronous());
		SkeletalMeshes.AddUnique(this->SkeletonA.OverWeight.Body.Base.LoadSynchronous());
		SkeletalMeshes.AddUnique(this->SkeletonA.UnderWeight.Body.Base.LoadSynchronous());
	}
	if (bFemale)
	{
		SkeletalMeshes.AddUnique(this->SkeletonB.NormalWeight.Body.Base.LoadSynchronous());
		SkeletalMeshes.AddUnique(this->SkeletonB.OverWeight.Body.Base.LoadSynchronous());
		SkeletalMeshes.AddUnique(this->SkeletonB.UnderWeight.Body.Base.LoadSynchronous());
	}

	return SkeletalMeshes;
}
