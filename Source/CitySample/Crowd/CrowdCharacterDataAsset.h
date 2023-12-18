// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
//#include "Dom/JsonObject.h"
#include "CrowdCharacterDefinition.h"
#include "CrowdCharacterEnums.h"
#include "GroomBindingAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/LODSyncComponent.h"

#include "CrowdCharacterDataAsset.generated.h"

class UGroomBindingAsset;

/*
 [Data Structure]

- Male (GenderDefinition)
	- NormalWeight (BodyOutfitDefinition)
		- Body (BodyDefinition)
			- Body (SkeletalMesh*)
			- Base (SkeletalMesh*)
		- Outfits (TArray<OutfitDefinition>)
			- Top (SkeletalMesh*)
			- Bottom (SkeletalMesh*)
			- Shoes (SkeletalMesh*)
		- ScaleFactors (TArray<float>)
	- OverWeight
	- UnderWeight
	- Heads (TArray<SkeletalMesh*>)
	- Hairs (TArray<HairDefinition>)
	- Accessories (TArray<AccessoryDefinition>)
	- Textures (TArray<UTexture2D*>)

- Female (GenderDefinition)
	- 

*/

UCLASS(Blueprintable, BlueprintType)
class CITYSAMPLE_API UCrowdCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual void PostLoad()
	{
		Super::PostLoad();

		// TArray<FCrowdGenderDefinition*> Genders = { &Male, &Female };
		//
		// for (FCrowdGenderDefinition* GenderDefinition : Genders)
		// {
		// 	for (auto& OutfitOverride : GenderDefinition->OutfitMaterials)
		// 	{
		// 		for (const auto& ColorEntry : OutfitOverride.Colors)
		// 		{
		// 			auto& MapEntry = OutfitOverride.MaterialOverrides.FindOrAdd(ColorEntry.SlotName);
		// 			auto& NewParameterEntry = MapEntry.ParameterOverrides.AddDefaulted_GetRef();
		// 			NewParameterEntry.ParameterName = ColorEntry.ParameterName;
		// 			NewParameterEntry.ParameterType = ECrowdMaterialParameterType::Color;
		// 			NewParameterEntry.Color = ColorEntry.Color;
		// 		}
		// 	}
		// }
		
	};

	/* Male BodyTypes, Outift & Hair Definition */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	FCrowdGenderDefinition SkeletonA;

	/* Female BodyTypes, Outift & Hair Definition */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	FCrowdGenderDefinition SkeletonB;

	/* Array of Hair Color Definitions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCrowdHairColorDefinition> HairColors;

	//-- LOD Related settings. Mirrors LOD Sync component settings and then passes them through in SetupLODSync
	
	// if -1, it's default and it will calculate the max number of LODs from all sub components
	// if not, it is a number of LODs (not the max index of LODs)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Sync")
	int32 NumLODs = 8;

	// if -1, it's automatically switching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Sync")
	int32 ForcedLOD = -1;

	// Optionnally override the min. ray tracing LOD set on the skeleton mesh. Default: -1, use the skeleton mesh value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LOD Sync")
	int32 RayTracingMinLOD = -1;

	/** 
	*	Array of components whose LOD may drive or be driven by this component.
	*  Components that are flagged as 'Drive' are treated as being in priority order, with the last component having highest priority. The highest priority
	*  visible component will set the LOD for all other components. If no components are visible, then the highest priority non-visible component will set LOD.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Sync")
	TArray<FCitySampleComponentSync> ComponentsToSync;

	// by default, the mapping will be one to one
	// but if you want custom, add here. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD Sync")
	TMap<FCitySampleCharacterComponentIdentifier, FLODMappingData> CustomLODMapping;

	//-- End LOD settings

	/* Array of Outfit Color Definitions */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
		//TArray<FCrowdOutfitColorDefinition> OutfitColors;

	/*  */

	// --------------------------------------------------------------------------------------------

	/* Returns Character Definition from given indices */
	UFUNCTION(BlueprintCallable)
	FCrowdCharacterDefinition GetCharacterDefinition(
		const ECitySampleCrowdGender Skeleton, const ECitySampleCrowdBodyType BodyType,
		const int32 HeadIndex, const int32 OutfitIndex, const int32 OutfitMaterialIndex, const int32 HairIndex, const int32 HairColorIndex,
		const int32 AccessoryIndex, const int32 SkinTextureIndex, const int32 SkinTextureModifierIndex,
		const int32 ScaleFactorIndex) const;


	/* Returns Random CharacterDefinition */
	//UFUNCTION(BlueprintCallable)
//		FCrowdCharacterDefinition GetRandomCharacterDefinition() const;

	/* Returns Definition Indices from a Global Index */
	UFUNCTION(BlueprintCallable)
	FCrowdCharacterDefinition GetCharacterDefinitionFromIndex(const int32 GlobalIndex) const;

	// --------------------------------------------------------------------------------------------
	// These Utilities are only for creating the LineUp. 
	// They do not return all possible variations, only Gender/BodyWeight/Outfit.
	// Hair, Texture, Accessories etc are hardcoded to first index.

	//UFUNCTION(BlueprintCallable)
		//int32 GetNumCharacterDefinitions() const;

	/* Returns Gender/BodyType/Outfits Indices from a Index */
	//UFUNCTION(BlueprintCallable)
		//void GetCharacterDefinitionIndices(const int32 Index,
			//ECitySampleCrowdGender& Gender, ECitySampleCrowdBodyType& BodyType, int32& OutfitIndex) const;

	// --------------------------------------------------------------------------------------------

	/* Finds All Unique Outfit DataAsset used in all OutfitDefinitions */
	UFUNCTION(BlueprintCallable)
	TArray< UAnimToTextureDataAsset*> FindOutfitDataAssets(const bool bMale=true, const bool bFemale=true) const;

	/* Finds All Unique Body DataAsset used in all BodyDefinitions */
	UFUNCTION(BlueprintCallable)
	TArray< UAnimToTextureDataAsset*> FindBodyDataAssets(const bool bMale = true, const bool bFemale = true) const;

	/* Finds All Unique Head DataAsset used in all Definitions */
	UFUNCTION(BlueprintCallable)
	TArray< UAnimToTextureDataAsset*> FindHeadDataAssets(const bool bMale = true, const bool bFemale = true) const;

	/* Finds Unique (Outfit/Body/Head) DataAsset used in all Definitions. */
	UFUNCTION(BlueprintCallable)
	TArray< UAnimToTextureDataAsset*> FindDataAssets(const bool bMale = true, const bool bFemale = true) const;

	/* Finds All Unique Outfit SkeletalMeshes used in all OutfitDefinitions */
	UFUNCTION(BlueprintCallable)
	TArray< USkeletalMesh*> FindOutfitSkeletalMeshes(const bool bMale = true, const bool bFemale = true) const;

	/* Finds All Unique Base SkeletalMeshes used in all BodyDefinitions */
	UFUNCTION(BlueprintCallable)
	TArray< USkeletalMesh*> FindBaseSkeletalMeshes(const bool bMale = true, const bool bFemale = true) const;

	/* Finds All Unique Body SkeletalMeshes used in all BodyDefinitions */
	UFUNCTION(BlueprintCallable)
	TArray< USkeletalMesh*> FindBodySkeletalMeshes(const bool bMale = true, const bool bFemale = true) const;

	/* Finds All Unique Head SkeletalMeshes used in all Definitions */
	UFUNCTION(BlueprintCallable)
	TArray< USkeletalMesh*> FindHeadSkeletalMeshes(const bool bMale = true, const bool bFemale = true) const;

	/* Finds Unique (Outfit/Body/Head) SkeletalMeshes used in all Definitions. */
	UFUNCTION(BlueprintCallable)
	TArray< USkeletalMesh*> FindSkeletalMeshes(const bool bMale = true, const bool bFemale = true) const;

	/* Finds All Unique Grooms used in all Definitions */
	UFUNCTION(BlueprintCallable)
	TArray< UGroomAsset* > FindGrooms(const bool bMale = true, const bool bFemale = true,
		const bool bHair = true, const bool bEyebrows = true, const bool bFuzz = true, const bool bEyelashes = true, const bool bMustache = true, const bool bBeard = true) const;

	/* Finds All Unique Groom Meshes used in all Definitions. 
	* If GroupIndex is INDEX_NONE all LOD meshes will be returned.
	*/
	UFUNCTION(BlueprintCallable)
	TArray< UStaticMesh* > FindGroomMeshes(const bool bMale = true, const bool bFemale = true,
		const bool bHair = true, const bool bEyebrows = true, const bool bFuzz = true, const bool bEyelashes = true, const bool bMustache = true, const bool bBeard = true, 
		const int32 GroupIndex = -1 /*INDEX_NONE*/) const;

	/* Finds All Unique Accessory Meshes used in all Definitions.*/
	UFUNCTION(BlueprintCallable)
	TArray< UStaticMesh* > FindAccesoryMeshes(const bool bMale = true, const bool bFemale = true) const;

	// --------------------------------------------------------------------------------------------

	/* Loads all Definitions from a JSON file.
	   All existing definitions will be replaced */
	//UFUNCTION(BlueprintCallable)
		//bool LoadFromJSON(const FString& FilePath);

	/* Save Definitions to a JSON file.
	   The JSON file with be stored in the Project Saved directory. */
	//UFUNCTION(BlueprintCallable)
		//bool SaveToJSON();

	// --------------------------------------------------------------------------------------------

private:

	/* Returns an Array with all Body Definitions, for all Genders and BodyTypes */
	//TArray<FCrowdBodyDefinition> GetBodyDefinitions() const;

	/* Returns an Array with all Outfit Definitions, for all Genders and BodyTypes */
	//TArray<FCrowdOutfitDefinition> GetOutfitDefinitions() const;

	/* Returns Serialized object */
	//TSharedPtr<FJsonObject> ToJSON() const;
};
