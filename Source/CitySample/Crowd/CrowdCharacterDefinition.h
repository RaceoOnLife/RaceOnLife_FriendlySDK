// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimToTextureDataAsset.h"
#include "Containers/Map.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Math/Color.h"
#include "UObject/SoftObjectPtr.h"

#include "GroomAsset.h"

#include "Crowd/CrowdCharacterEnums.h"
#include "Components/LODSyncComponent.h"

#include "CrowdCharacterDefinition.generated.h"

class UCrowdCharacterDataAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogCitySampleCrowdDefinition, Log, All);

// ----------------------------------------------------------------------------

/**
	Hair Color Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdHairColorDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HairMelanin = 0.161905f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HairRedness = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HairRoughness = 0.37f;
};

// ----------------------------------------------------------------------------
//						OUTFIT MATERIAL DEFINITION
// ----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdMaterialColorOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ParameterName = "Color";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECrowdMaterialParameterType ParameterType = ECrowdMaterialParameterType::Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (HideAlphaChannel, EditCondition="ParameterType==ECrowdMaterialParameterType::Color", EditConditionHides))
	FColor Color = FColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition="ParameterType==ECrowdMaterialParameterType::Vector", EditConditionHides))
	FVector Vector = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition="ParameterType==ECrowdMaterialParameterType::Float", EditConditionHides))
	float Float = 0.f;

	void ApplyToMaterial(UMaterialInstanceDynamic* MID) const;
};

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdPatternInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Scale = 6.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="31"))
	int Selection = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="2"))
	float RoughnessMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ISMBlendAmount = 1.f;

	void ApplyToMaterial(UMaterialInstanceDynamic* MID) const;
};

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdMaterialOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCrowdMaterialColorOverride> ParameterOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECrowdPatternUsage PatternUsage = ECrowdPatternUsage::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (HideAlphaChannel, EditCondition="PatternUsage==ECrowdPatternUsage::PatternList", EditConditionHides))
	TArray<FColor> ComplimentaryColors;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition="PatternUsage==ECrowdPatternUsage::PatternList", EditConditionHides))
	TArray<FCrowdPatternInfo> Patterns;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition="PatternUsage==ECrowdPatternUsage::Driven", EditConditionHides))
	FName SourceSlot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition="PatternUsage==ECrowdPatternUsage::Driven", EditConditionHides))
	float ScaleMultiplier = 1.f;
};

/**
	Character Part OutfitColors
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdOutfitMaterialDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FCrowdMaterialOverride> MaterialOverrides;

	void ApplyToComponent(UMeshComponent* MeshComponent, const uint8 PatternColorIndex, const uint8 PatternOptionIndex) const;
	bool GetPatternInfoForSlot(const FName SlotName, const uint8 PatternColorIndex, const uint8 PatternSelectionIndex, FColor& PatternColor, FCrowdPatternInfo& PatternInfo) const;

	int32 GetMaxPatternColors() const;
	int32 GetMaxPatternOptions() const;
};

// ----------------------------------------------------------------------------

/**
	Accessory Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdAccessoryDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocalPosition = FVector::ZeroVector;

	/** #fixme jf: I assume this should be an FRotator and other code fixed up to deal? Current usage of this treats it like a forward vector instead of a rotation, which is maybe ok? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocalRotation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TSoftObjectPtr<UDataAsset> AccessoryAnimSet;
	
	// Assigns a weight to the accessory for use in randomization
	// Weights less than 0 will be treated as if they were 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RandomWeight = 1;
};

// ----------------------------------------------------------------------------

/**
	Hair Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdHairDefinition
{
	GENERATED_BODY()

	FCrowdHairDefinition() = default;
	FCrowdHairDefinition(TSoftObjectPtr<UGroomAsset> InGroom, TSoftObjectPtr<UGroomBindingAsset> InGroomBinding)
		: Groom(InGroom)
		, GroomBinding(InGroomBinding)
	{

	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UGroomAsset> Groom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UGroomBindingAsset> GroomBinding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UPhysicsAsset> PhysicsAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> BakedGroomMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "7", UIMin = "0", UIMax = "7"))
	int BakedGroomMinimumLOD = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TSoftObjectPtr<UStaticMesh> GroomStaticMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TSoftObjectPtr<UTexture2D> FollicleMask;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLocalSimulation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString LocalBone;

	void UpdateGroomBinding(const FSoftObjectPath& PathToHeadMesh, const ECitySampleCrowdGender Skeleton);	
	
	UStaticMesh* GetGroomStaticMesh() const
	{
		UStaticMesh* ReturnVal = nullptr;
		if (GroomStaticMesh.ToSoftObjectPath().IsValid())
		{
			ReturnVal = GroomStaticMesh.Get();
			if (!ReturnVal)
			{
				UStaticMesh* LoadedAsset = Cast<UStaticMesh>(GroomStaticMesh.ToSoftObjectPath().TryLoad());
				if (ensureMsgf(LoadedAsset, TEXT("Failed to load asset pointer %s"), *GroomStaticMesh.ToString()))
				{
					ReturnVal = LoadedAsset;
				}
			}
		}
		return ReturnVal;
	}
};

// ----------------------------------------------------------------------------

/**
	Hair Slot Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdHairSlot
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere)
	FName SlotName;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TArray<FCrowdHairDefinition> HairDefinitions;
};

// ----------------------------------------------------------------------------

/**
* Body Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdBodyDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USkeletalMesh> Base;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimToTextureDataAsset> BodyData;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
		//TSoftObjectPtr<USkeletalMesh> Head;
};

// ----------------------------------------------------------------------------

/**
	Outfit Color Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdOutfitDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimToTextureDataAsset> TopData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimToTextureDataAsset> BottomData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimToTextureDataAsset> ShoesData;
};

// ----------------------------------------------------------------------------

/**
* Body / Outfit Definition
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdBodyOutfitDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdBodyDefinition Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UAnimToTextureDataAsset>> HeadsData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCrowdOutfitDefinition> Outfits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<float> ScaleFactors = { 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TArray<TSoftObjectPtr<UDataAsset>> LocomotionAnimSets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TSoftObjectPtr<class UMassCrowdContextualAnimationDataAsset> ContextualAnimData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TArray<FCrowdAccessoryDefinition> Accessories;
};

// ----------------------------------------------------------------------------
//								SKIN TEXTURE MODIFIER
// ----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdSkinTextureModifierDefinition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OffsetU = 0.5f; // MelaninRandomSeed

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OffsetV = 0.5f; // RednessRandomSeed
};

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdSkinTextureDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "15", UIMin = "0", UIMax = "15"))
	int TextureAtlasIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> BodyColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ChestColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceCavity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceColorCM1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceColorCM2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceColorCM3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceNormal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceNormalWM1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceNormalWM2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceNormalWM3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FaceRoughness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ChestRoughness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SourceU = 0.5f; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SourceV = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.9", ClampMax = "1.1", UIMin = "0.9", UIMax = "1.1"))
	float AlbedoMultiplier = 1.f;
};

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdSkinMaterialDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdSkinTextureDefinition Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCrowdSkinTextureModifierDefinition> TextureModifiers;
};

/**
	Gender Definition
	Struct for Masculine or Feminine
*/
USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdGenderDefinition
{
	GENERATED_BODY()

	FCrowdGenderDefinition()
	{
		uint8 NumSlots = static_cast<uint8>(ECrowdHairSlots::MAX);
		HairSlots.SetNum(NumSlots);

#if WITH_EDITORONLY_DATA
		UEnum* StaticSlotEnum = StaticEnum<ECrowdHairSlots>();
		check(StaticSlotEnum);

		for (int SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx)
		{
			HairSlots[SlotIdx].SlotName = FName(StaticSlotEnum->GetNameStringByValue(SlotIdx));
		}
#endif
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	FCrowdBodyOutfitDefinition NormalWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	FCrowdBodyOutfitDefinition OverWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	FCrowdBodyOutfitDefinition UnderWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, meta=(AssetBundles = "Client", TitleProperty = "SlotName"))
	TArray<FCrowdHairSlot> HairSlots;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCrowdSkinMaterialDefinition> SkinMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCrowdOutfitMaterialDefinition> OutfitMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimToTextureDataAsset> FarLodMeshData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Client"))
	TArray<TSoftObjectPtr<UGroomBindingAsset>> GroomBindings;
};

// ----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCrowdCharacterDefinition
{
	GENERATED_BODY()

	FCrowdCharacterDefinition()
	{
		HairDefinitions.SetNum(static_cast<uint8>(ECrowdHairSlots::MAX));
	}

	const FCrowdHairDefinition& GetHairDefinitionForSlot(const ECrowdHairSlots HairSlot) const
	{
		uint8 SlotIdx = static_cast<uint8>(HairSlot);

		// Should always succeed as the array is sized to the enum
		check(HairDefinitions.IsValidIndex(SlotIdx));

		return HairDefinitions[SlotIdx];
	}

	// Meshes

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdBodyDefinition BodyDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USkeletalMesh> Head;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UAnimToTextureDataAsset> HeadData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdOutfitDefinition OutfitDefinition;

	// Fixed size array where the index in the array corresponds to a value in ECrowdHairSlots
	// i.e. HairDefinitions[0] is the hair definition for ECrowdHairSlots::Hair and so on
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize)
	TArray<FCrowdHairDefinition> HairDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdAccessoryDefinition AccessoryDefinition;

	// Material & Textures

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdHairColorDefinition HairColorDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdOutfitMaterialDefinition OutfitMaterialDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdSkinTextureDefinition SkinTextureDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCrowdSkinTextureModifierDefinition SkinTextureModifierDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ScaleFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 PatternColorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 PatternOptionIndex = 0;

	// Optionnally override the min. ray tracing LOD set on the skeleton mesh. Default: -1, use the skeleton mesh value
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RayTracingMinLOD = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDataAsset> LocomotionAnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UMassCrowdContextualAnimationDataAsset> ContextualAnimDataAsset;

	TArray<FSoftObjectPath> GetSoftPathsToLoad() const;
};

USTRUCT(BlueprintType)
struct FCrowdCharacterOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	ECitySampleCrowdGender Skeleton = ECitySampleCrowdGender::A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp, meta = (DisplayName = "Body Shape"))
	ECitySampleCrowdBodyType BodyType = ECitySampleCrowdBodyType::NormalWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	uint8 HeadIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Outfit", Interp)
	uint8 OutfitIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Outfit", Interp)
	uint8 OutfitMaterialIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 HairIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 EyebrowsIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 FuzzIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 EyelashesIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 MustacheIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 BeardIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Hair", Interp)
	uint8 HairColorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Skin", Interp)
	uint8 SkinTextureIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base|Skin", Interp)
	uint8 SkinTextureModifierIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	uint8 AccessoryIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	uint8 ScaleFactorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	uint8 AnimSetIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	uint8 PatternColorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup|Base", Interp)
	uint8 PatternOptionIndex = 0;

	void GenerateCharacterDefinition(const UCrowdCharacterDataAsset* DataAsset, FCrowdCharacterDefinition& CharacterDefinition) const;
	
	// Use the provided data asset to generate a random set of valid options
	void Randomize(const UCrowdCharacterDataAsset& DataAsset);
	void Randomize(const UCrowdCharacterDataAsset& DataAsset, const FRandomStream& RandomStream);
	void Randomize(const UCrowdCharacterDataAsset& DataAsset, TSet<ECrowdLineupVariation> FixedProperties);
	void Randomize(const UCrowdCharacterDataAsset& DataAsset, TSet<ECrowdLineupVariation> FixedProperties, const FRandomStream& RandomStream);

	bool operator==(const FCrowdCharacterOptions& OtherOptions) const
	{
		return	Skeleton						== OtherOptions.Skeleton
			&&	BodyType					== OtherOptions.BodyType
			&&	HeadIndex					== OtherOptions.HeadIndex
			&&	OutfitIndex					== OtherOptions.OutfitIndex
			&&	OutfitMaterialIndex			== OtherOptions.OutfitMaterialIndex
			&&	HairIndex					== OtherOptions.HairIndex
			&&	EyebrowsIndex				== OtherOptions.EyebrowsIndex
			&&	EyelashesIndex				== OtherOptions.EyelashesIndex
			&&	MustacheIndex				== OtherOptions.MustacheIndex
			&&	BeardIndex					== OtherOptions.BeardIndex
			&&	HairColorIndex				== OtherOptions.HairColorIndex
			&&	SkinTextureIndex			== OtherOptions.SkinTextureIndex
			&&	SkinTextureModifierIndex	== OtherOptions.SkinTextureModifierIndex
			&&	AccessoryIndex				== OtherOptions.AccessoryIndex
			&&	ScaleFactorIndex			== OtherOptions.ScaleFactorIndex
			&&	AnimSetIndex				== OtherOptions.AnimSetIndex
			&&	PatternColorIndex			== OtherOptions.PatternColorIndex
			&&	PatternOptionIndex			== OtherOptions.PatternOptionIndex;
	}

	bool operator!=(const FCrowdCharacterOptions& OtherOptions) const
	{
		return !(*this == OtherOptions);
	}
};

// This struct represents the packed character options for use with FCrowdVisualizationID
// The actual struct will be stored as an int64 in the VisualizationID to work around
// the property system not supporting bitfields
struct FCrowdVisualizationBitfield
{
	uint64 Skeleton					: 1;
	uint64 BodyType					: 2;
	uint64 HeadIndex				: 3;
	uint64 OutfitIndex				: 5;
	uint64 OutfitMaterialIndex		: 5;
	uint64 HairIndex				: 3;
	uint64 EyebrowsIndex			: 3;
	uint64 EyelashesIndex			: 3;
	uint64 MustacheIndex			: 3;
	uint64 BeardIndex				: 3;
	uint64 HairColorIndex			: 5;
	uint64 SkinTextureIndex			: 4;
	uint64 SkinTextureModifierIndex : 6;
	uint64 AccessoryIndex			: 4;
	uint64 ScaleFactorIndex			: 4;
	uint64 AnimSetIndex				: 3;
	uint64 PatternColorIndex		: 2;
	uint64 PatternOptionIndex		: 5;
};

USTRUCT(BlueprintType)
struct FCrowdVisualizationID
{
	GENERATED_BODY()

	// All the data is packed inside of an int64 and can be accessed by reinterpreting as
	// a FCrowdVisualizationBitfield. See FCrowdVisualizationID::AsMutableBitfield.
	UPROPERTY()
	int64 PackedData = 0;

	FCrowdVisualizationID() = default;
	FCrowdVisualizationID(const FCrowdCharacterOptions InOptions);

	// Convert the ID to a full options struct
	FCrowdCharacterOptions ToCharacterOptions() const;

	// Reinterprets the packed data as a bitfield struct for easier use
	// provides both a mutable and an immutable version;
	FCrowdVisualizationBitfield& AsMutableBitfield();
	const FCrowdVisualizationBitfield& AsBitfield() const;

	// Randomization Functions
	void Randomize();
	void Randomize(int32 InSeed);
	void Randomize(const FRandomStream& RandomStream);

};

UENUM()
enum class ECitySampleCharacterComponentType : uint8
{
	Mesh,
	Groom
};

USTRUCT(BlueprintType)
struct FCitySampleCharacterComponentIdentifier
{
	GENERATED_BODY()

	FCitySampleCharacterComponentIdentifier() = default;

	FCitySampleCharacterComponentIdentifier(const ECrowdMeshSlots InMeshSlot)
	{
		ComponentType = ECitySampleCharacterComponentType::Mesh;
		MeshSlot = InMeshSlot;
	};

	FCitySampleCharacterComponentIdentifier(const ECrowdHairSlots InGroomSlot)
	{
		ComponentType = ECitySampleCharacterComponentType::Groom;
		GroomSlot = InGroomSlot;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FCitySampleCharacterComponentIdentifier)
	ECitySampleCharacterComponentType ComponentType = ECitySampleCharacterComponentType::Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FCitySampleCharacterComponentIdentifier,
		meta=(EditCondition="ComponentType==ECitySampleCharacterComponentType::Mesh", EditConditionHides))
	ECrowdMeshSlots MeshSlot = ECrowdMeshSlots::Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FCitySampleCharacterComponentIdentifier,
		meta=(EditCondition="ComponentType==ECitySampleCharacterComponentType::Groom", EditConditionHides))
	ECrowdHairSlots GroomSlot = ECrowdHairSlots::Hair;

	bool operator==(const FCitySampleCharacterComponentIdentifier& OtherIdentifier) const
	{
		if (this->ComponentType == OtherIdentifier.ComponentType)
		{
			switch(ComponentType)
			{
			case ECitySampleCharacterComponentType::Mesh:
				return this->MeshSlot == OtherIdentifier.MeshSlot;
			case ECitySampleCharacterComponentType::Groom:
				return this->GroomSlot == OtherIdentifier.GroomSlot;
			default:
				checkNoEntry();
			}
		}

		return false;
	}
};

inline uint32 GetTypeHash(const FCitySampleCharacterComponentIdentifier& Identifier)
{
	// For the sake of comparison only the slot for the current component type is relevant
	const uint8 SlotIndex = Identifier.ComponentType == ECitySampleCharacterComponentType::Mesh ?
		static_cast<uint8>(Identifier.MeshSlot) : static_cast<uint8>(Identifier.GroomSlot);

	// Hash is just a composite of the type and the current slot value
	const uint32 Hash = (static_cast<uint8>(Identifier.ComponentType) << 8) + SlotIndex;
	return Hash;
}

USTRUCT(BlueprintType)
struct FCitySampleComponentSync
{
	GENERATED_BODY()
	
	FCitySampleComponentSync() = default;

	FCitySampleComponentSync(const FCitySampleCharacterComponentIdentifier InComponentIdentifier, const ESyncOption InSyncOption)
		: ComponentIdentifier(InComponentIdentifier)
		, SyncOption(InSyncOption)
	{ };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FCitySampleComponentSync)
	FCitySampleCharacterComponentIdentifier ComponentIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FCitySampleComponentSync)
	ESyncOption SyncOption = ESyncOption::Disabled;
};
