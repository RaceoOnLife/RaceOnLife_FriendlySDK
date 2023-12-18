// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdVisualizationFragment.h"
#include "MassExecutionContext.h"
#include "CrowdCharacterDataAsset.h"
#include "CrowdPresetDataAsset.h"
#include "MassCrowdRepresentationSubsystem.h"
#include "MassRepresentationTypes.h"
#include "MassRepresentationFragments.h"
#include "MassCrowdAnimationTypes.h"
#include "CitySampleCrowdSettings.h"
#include "CrowdCharacterActor.h"

static TAutoConsoleVariable<bool> CVarUseMetahumanPresets(
    TEXT("Crowd.UseMetahumanPresets"),
    false,
    TEXT("Controls whether the Visualization fragment will attempt to use a data asset of presets to control the randomization"),
	ECVF_Cheat);

static TAutoConsoleVariable<FString> CVarMetahumanPresetsPath(
    TEXT("Crowd.MetahumanPresetsPath"),
    "/Game/Crowd/Character/Shared/CrowdPresets.CrowdPresets",
    TEXT("Path to the data asset containing the Metahuman Presets"),
	ECVF_Cheat);

static TAutoConsoleVariable<bool> CVarUseISMFarLod(
	TEXT("Crowd.UseISMFarLOD"),
	true,
	TEXT("Controls whether we'll use an alternate user-defined (in CrowdCharacterDataAsset) simplified ISM for furthest crowds"),
	ECVF_Cheat);

static TAutoConsoleVariable<bool> CVarUseISMFarLodMeshOverride(
	TEXT("Crowd.UseISMFarLodMeshOverride"),
	false,
	TEXT("Controls whether we'll use the static mesh override for Far LODs defined in UCitySampleCrowdSettings"),
	ECVF_Cheat);

static TAutoConsoleVariable<bool> CVarUseMetahumanISMParts(
	TEXT("Crowd.UseMetahumanISMParts"),
	true,
	TEXT("Controls whether the Visualization fragment will use parts"),
	ECVF_Cheat);

static TAutoConsoleVariable<int> CVarMaxCrowdHeads(
	TEXT("Crowd.MaxHeads"),
	0,
	TEXT("Set the maximum number of heads to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdOutfits(
	TEXT("Crowd.MaxOutfits"),
	0,
	TEXT("Set the maximum number of outfits to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdOutfitMaterials(
	TEXT("Crowd.MaxOutfitMaterials"),
	0,
	TEXT("Set the maximum number of outfit materials to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdHairstyles(
	TEXT("Crowd.MaxHairstyles"),
	0,
	TEXT("Set the maximum number of hairstyles to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdEyebrows(
	TEXT("Crowd.MaxEyebrows"),
	0,
	TEXT("Set the maximum number of eyebrows to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdEyelashes(
	TEXT("Crowd.MaxEyelashes"),
	0,
	TEXT("Set the maximum number of eyelashes to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdMustaches(
	TEXT("Crowd.MaxMustaches"),
	0,
	TEXT("Set the maximum number of mustaches to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdBeards(
	TEXT("Crowd.MaxBeards"),
	0,
	TEXT("Set the maximum number of beards to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdHairColors(
	TEXT("Crowd.MaxHairColors"),
	0,
	TEXT("Set the maximum number of hair colors to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdSkinTextures(
	TEXT("Crowd.MaxSkinTextures"),
	0,
	TEXT("Set the maximum number of skin textures to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdSkinTextureModifiers(
	TEXT("Crowd.MaxSkinTextureModifiers"),
	0,
	TEXT("Set the maximum number of skin texture modifiers to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarMaxCrowdAccessories(
	TEXT("Crowd.MaxAccessories"),
	0,
	TEXT("Set the maximum number of accessories to use for crowds. Setting this to 0 or lower will be unlimited"),
	ECVF_Scalability);

static TAutoConsoleVariable<int> CVarCrowdRandomSeed(
	TEXT("Crowd.RandomSeed"),
	376789,
	TEXT("Set's a random seed to ensure crowds are generated in a consistent way for meaningful performance comparisons"),
	ECVF_Cheat);

UCitySampleCrowdVisualizationFragmentInitializer::UCitySampleCrowdVisualizationFragmentInitializer()
	: EntityQuery(*this)
{
	ObservedType = FCitySampleCrowdVisualizationFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;
}

void UCitySampleCrowdVisualizationFragmentInitializer::ConfigureQueries() 
{
	EntityQuery.AddRequirement<FCitySampleCrowdVisualizationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FCrowdAnimationFragment>(EMassFragmentAccess::ReadWrite);
}

uint32 UCitySampleCrowdVisualizationFragmentInitializer::FindColorOverride(FCrowdCharacterDefinition& CharacterDefinition, USkeletalMesh* SkelMesh)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_CitySampleCrowdVisualizationFragmentInitializer_FindColorOverride);

	if (SkelMesh == nullptr)
	{
		return FColor::White.ToPackedRGBA();
	}

	static FName PrioritySlots[] = {
									TEXT("M_Blazer"),
									TEXT("M_Vest"),
									TEXT("M_ButtonDown"),
									TEXT("M_Jeans"),
									TEXT("M_Slacks"),
									TEXT("M_CroppedJacket")
	};

	TArray<FName> MaterialSearchOrder;
	const TArray<FSkeletalMaterial>& SkeletalMeshMaterials = SkelMesh->GetMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMeshMaterials.Num(); ++MaterialIndex)
	{
		const FSkeletalMaterial& SkeletalMaterial = SkeletalMeshMaterials[MaterialIndex];
		
		MaterialSearchOrder.AddUnique(SkeletalMaterial.MaterialSlotName);
	}

	// Reverse through priority slots, if found, move to the top
	for (int i = UE_ARRAY_COUNT(PrioritySlots) - 1; i >= 0; i--)
	{
		for (int32 MaterialIndex = 1; MaterialIndex < MaterialSearchOrder.Num(); ++MaterialIndex)
		{
			if (PrioritySlots[i] == MaterialSearchOrder[MaterialIndex])
			{
				MaterialSearchOrder.RemoveAt(MaterialIndex);
				MaterialSearchOrder.Insert(PrioritySlots[i], 0);
				break;
			}
		}
	}

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialSearchOrder.Num(); ++MaterialIndex)
	{
		const FCrowdMaterialOverride* MaterialOverride = CharacterDefinition.OutfitMaterialDefinition.MaterialOverrides.Find(MaterialSearchOrder[MaterialIndex]);
		if (MaterialOverride)
		{
			for (const FCrowdMaterialColorOverride& ParameterOverride : MaterialOverride->ParameterOverrides)
			{	
				static FName NAME_CrowdColor = TEXT("A_CrowdColor_main");
				if (ParameterOverride.ParameterName == NAME_CrowdColor)
				{
					FColor PatternColor;
					FCrowdPatternInfo PatternInfo;
					const bool bSlotUsesPattern = CharacterDefinition.OutfitMaterialDefinition.GetPatternInfoForSlot(MaterialSearchOrder[MaterialIndex], CharacterDefinition.PatternColorIndex, CharacterDefinition.PatternOptionIndex, PatternColor, PatternInfo);
					if (bSlotUsesPattern)
					{
						FColor BlendedColor;
						BlendedColor.R = FMath::Lerp(ParameterOverride.Color.R, PatternColor.R, PatternInfo.ISMBlendAmount);
						BlendedColor.G = FMath::Lerp(ParameterOverride.Color.G, PatternColor.G, PatternInfo.ISMBlendAmount);
						BlendedColor.B = FMath::Lerp(ParameterOverride.Color.B, PatternColor.B, PatternInfo.ISMBlendAmount);

						return BlendedColor.ToPackedRGBA();
					}

					return ParameterOverride.Color.ToPackedRGBA();
				}
			}
		}
	}

	return FColor::White.ToPackedRGBA();
}

// Use a struct to store the values from all the crowd Scalabilityvariables so we don't need to call GetValue repeatedly
struct FCachedCrowdScalabilityValues
{
	uint8 MaxHeads;
	uint8 MaxOutfits;
	uint8 MaxOutfitMaterials;
	uint8 MaxHairstyles;
	uint8 MaxEyebrows;
	uint8 MaxEyelashes;
	uint8 MaxMustaches;
	uint8 MaxBeards;
	uint8 MaxHairColors;
	uint8 MaxSkinTextures;
	uint8 MaxSkinTextureModifiers;
	uint8 MaxAccessories;
	
	void CacheValues()
	{
		MaxHeads				= FMath::Clamp(CVarMaxCrowdHeads.GetValueOnAnyThread(), 0, 255);
		MaxOutfits				= FMath::Clamp(CVarMaxCrowdOutfits.GetValueOnAnyThread(), 0, 255);
		MaxOutfitMaterials		= FMath::Clamp(CVarMaxCrowdOutfitMaterials.GetValueOnAnyThread(), 0, 255);
		MaxHairstyles			= FMath::Clamp(CVarMaxCrowdHairstyles.GetValueOnAnyThread(), 0, 255);
		MaxEyebrows				= FMath::Clamp(CVarMaxCrowdEyebrows.GetValueOnAnyThread(), 0, 255);
		MaxEyelashes			= FMath::Clamp(CVarMaxCrowdEyelashes.GetValueOnAnyThread(), 0, 255);
		MaxMustaches			= FMath::Clamp(CVarMaxCrowdMustaches.GetValueOnAnyThread(), 0, 255);
		MaxBeards				= FMath::Clamp(CVarMaxCrowdBeards.GetValueOnAnyThread(), 0, 255);
		MaxHairColors			= FMath::Clamp(CVarMaxCrowdHairColors.GetValueOnAnyThread(), 0, 255);
		MaxSkinTextures			= FMath::Clamp(CVarMaxCrowdSkinTextures.GetValueOnAnyThread(), 0, 255);
		MaxSkinTextureModifiers = FMath::Clamp(CVarMaxCrowdSkinTextureModifiers.GetValueOnAnyThread(), 0, 255);
		MaxAccessories			= FMath::Clamp(CVarMaxCrowdAccessories.GetValueOnAnyThread(), 0, 255);
	}
};

// We have to use a macro here due to using bitfields
// We use modulo rather than clamping the value to prevent random values being unequally weighted to the max value
#define ClampOption(Option, MaxValue) \
if (MaxValue > 0) \
{ \
	Option = Option % MaxValue; \
};

// Two Clamp functions as depending on the initialization path we may be initializing from an FCrowdCharacterOptions or a FCrowdVisualizationID

void ClampCharacterOptions(FCrowdCharacterOptions& InCharacterOptions, const FCachedCrowdScalabilityValues& CachedCrowdScalabilityValues)
{
	ClampOption(InCharacterOptions.HeadIndex,					CachedCrowdScalabilityValues.MaxHeads);
	ClampOption(InCharacterOptions.OutfitIndex,					CachedCrowdScalabilityValues.MaxOutfits);
	ClampOption(InCharacterOptions.OutfitMaterialIndex,			CachedCrowdScalabilityValues.MaxOutfitMaterials);
	ClampOption(InCharacterOptions.HairIndex,					CachedCrowdScalabilityValues.MaxHairstyles);
	ClampOption(InCharacterOptions.EyebrowsIndex,				CachedCrowdScalabilityValues.MaxEyebrows);
	ClampOption(InCharacterOptions.EyelashesIndex,				CachedCrowdScalabilityValues.MaxEyelashes);
	ClampOption(InCharacterOptions.MustacheIndex,				CachedCrowdScalabilityValues.MaxMustaches);
	ClampOption(InCharacterOptions.BeardIndex,					CachedCrowdScalabilityValues.MaxBeards);
	ClampOption(InCharacterOptions.HairColorIndex,				CachedCrowdScalabilityValues.MaxHairColors);
	ClampOption(InCharacterOptions.SkinTextureIndex,			CachedCrowdScalabilityValues.MaxSkinTextures);
	ClampOption(InCharacterOptions.SkinTextureModifierIndex,	CachedCrowdScalabilityValues.MaxSkinTextureModifiers);
	ClampOption(InCharacterOptions.AccessoryIndex,				CachedCrowdScalabilityValues.MaxAccessories);
}

void ClampVisualizationID(FCrowdVisualizationID& InCharacterVisualizationID, const FCachedCrowdScalabilityValues& CachedCrowdScalabilityValues)
{
	// We use modulo rather than clamping the value to prevent random values being unequally weighted to the max value

	FCrowdVisualizationBitfield& VisualizationBitfield = InCharacterVisualizationID.AsMutableBitfield();

	ClampOption(VisualizationBitfield.HeadIndex,				CachedCrowdScalabilityValues.MaxHeads);
	ClampOption(VisualizationBitfield.OutfitIndex,				CachedCrowdScalabilityValues.MaxOutfits);
	ClampOption(VisualizationBitfield.OutfitMaterialIndex,		CachedCrowdScalabilityValues.MaxOutfitMaterials);
	ClampOption(VisualizationBitfield.HairIndex,				CachedCrowdScalabilityValues.MaxHairstyles);
	ClampOption(VisualizationBitfield.EyebrowsIndex,			CachedCrowdScalabilityValues.MaxEyebrows);
	ClampOption(VisualizationBitfield.EyelashesIndex,			CachedCrowdScalabilityValues.MaxEyelashes);
	ClampOption(VisualizationBitfield.MustacheIndex,			CachedCrowdScalabilityValues.MaxMustaches);
	ClampOption(VisualizationBitfield.BeardIndex,				CachedCrowdScalabilityValues.MaxBeards);
	ClampOption(VisualizationBitfield.HairColorIndex,			CachedCrowdScalabilityValues.MaxHairColors);
	ClampOption(VisualizationBitfield.SkinTextureIndex,			CachedCrowdScalabilityValues.MaxSkinTextures);
	ClampOption(VisualizationBitfield.SkinTextureModifierIndex, CachedCrowdScalabilityValues.MaxSkinTextureModifiers);
	ClampOption(VisualizationBitfield.AccessoryIndex,			CachedCrowdScalabilityValues.MaxAccessories);
}

#undef ClampOption

void UCitySampleCrowdVisualizationFragmentInitializer::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UMassCrowdRepresentationSubsystem* RepresentationSubsystem = UWorld::GetSubsystem<UMassCrowdRepresentationSubsystem>(EntityManager.GetWorld());

	// Cache the Scalabilityvalues
	FCachedCrowdScalabilityValues CachedCrowdScalabilityValues;
	CachedCrowdScalabilityValues.CacheValues();

	// Build the Random Stream from the seed if one is provided
	FRandomStream RandomStream;

	const int32 CrowdRandomSeed = CVarCrowdRandomSeed.GetValueOnGameThread();
	if (CrowdRandomSeed >= 0)
	{
		RandomStream.Initialize(CrowdRandomSeed);
	}
	else
	{
		RandomStream.GenerateNewSeed();
	}
	
	// Check if we're using presets
	if (CVarUseMetahumanPresets.GetValueOnGameThread() == true)
	{
		// Grab the path to the data asset from the cvar
		const FSoftObjectPath PresetDataAssetPath(CVarMetahumanPresetsPath.GetValueOnGameThread());
		
		// If we have a valid preset asset then we're going to use that to generate the random characters and set up the ISMs
		if (UCrowdPresetDataAsset* PresetDataAsset = Cast<UCrowdPresetDataAsset>(PresetDataAssetPath.TryLoad()))
		{
			const TArray<FCrowdPreset>& Presets = PresetDataAsset->Presets;
		
			// Ensure we have at least one preset
			if (!Presets.IsEmpty() && RepresentationSubsystem)
			{
				EntityQuery.ForEachEntityChunk(EntityManager, Context, [this, Presets, RepresentationSubsystem](FMassExecutionContext& Context)
                {
                    // Get the associated fragment arrays
					const TArrayView<FCitySampleCrowdVisualizationFragment> VisualizationList = Context.GetMutableFragmentView<FCitySampleCrowdVisualizationFragment>();
					const TArrayView<FMassRepresentationFragment> VisualizationDataList = Context.GetMutableFragmentView<FMassRepresentationFragment>();				
				
                    const int32 NumEntities = Context.GetNumEntities();
                    for (int32 i = 0; i < NumEntities; ++i)
                    {
                        const int PresetIndex = FMath::RandRange(0, Presets.Num() - 1);
			
                        VisualizationList[i].VisualizationID = FCrowdVisualizationID(Presets[PresetIndex].CharacterOptions);

                        // Build a mesh description in the expected format for the RepresentationSubsystem from the Preset Meshes
                        FStaticMeshInstanceVisualizationDesc StaticMeshDec;
                        for (TSoftObjectPtr<UStaticMesh> StaticMeshPtr : Presets[PresetIndex].Meshes)
                        {
                            FStaticMeshInstanceVisualizationMeshDesc MeshDesc;
                            MeshDesc.Mesh = StaticMeshPtr.LoadSynchronous();
                            StaticMeshDec.Meshes.Add(MeshDesc);
                        }
					
                        VisualizationDataList[i].StaticMeshDescIndex = RepresentationSubsystem->FindOrAddStaticMeshDesc(StaticMeshDec);
                    }
                });

				// Early return to avoid the fallback
				return;
			}
		}
	}

	// Fallback random if we were unable to set up using the presets
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&, this, RepresentationSubsystem, CachedCrowdScalabilityValues, RandomStream](FMassExecutionContext& Context)
	{
		const TArrayView<FCitySampleCrowdVisualizationFragment> VisualizationList = Context.GetMutableFragmentView<FCitySampleCrowdVisualizationFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();
		const TArrayView<FCrowdAnimationFragment> AnimationDataList = Context.GetMutableFragmentView<FCrowdAnimationFragment>();
		const int32 NumEntities = Context.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			// Try to get the data asset from the crowd character CDO
			ACitySampleCrowdCharacter* CitySampleCrowdCharacterCDO = nullptr;
			UCrowdCharacterDataAsset* CrowdCharacterDataAsset = nullptr;
			TSubclassOf<AActor> TemplateActorClass = RepresentationSubsystem->GetTemplateActorClass(RepresentationList[i].HighResTemplateActorIndex);
			if (TemplateActorClass)
			{
				CitySampleCrowdCharacterCDO = Cast<ACitySampleCrowdCharacter>(TemplateActorClass->GetDefaultObject());
				if (CitySampleCrowdCharacterCDO)
				{
					CrowdCharacterDataAsset = CitySampleCrowdCharacterCDO->CrowdCharacterData.Get();
				}
			}
			
			// Grab the settings class
			const UCitySampleCrowdSettings* CrowdSettings = UCitySampleCrowdSettings::Get();

			// Randomize using the data asset if possible else fallback to a generic randomize
			FCrowdCharacterOptions CharacterOptions;
			if (CrowdSettings && CrowdSettings->bMassCrowdShouldUseActorDefaultOptions && CitySampleCrowdCharacterCDO)
			{
				CharacterOptions = CitySampleCrowdCharacterCDO->CharacterOptions;
				VisualizationList[i].VisualizationID = FCrowdVisualizationID(CharacterOptions);
			}
			else if (CrowdCharacterDataAsset)
			{
				CharacterOptions.Randomize(*CrowdCharacterDataAsset, RandomStream);
				ClampCharacterOptions(CharacterOptions, CachedCrowdScalabilityValues);
				VisualizationList[i].VisualizationID = FCrowdVisualizationID(CharacterOptions);
			}
			else
			{
				VisualizationList[i].VisualizationID.Randomize(RandomStream);
				ClampVisualizationID(VisualizationList[i].VisualizationID, CachedCrowdScalabilityValues);
			}

			if (CVarUseMetahumanISMParts.GetValueOnGameThread() == true && RepresentationSubsystem)
			{
				if (CrowdCharacterDataAsset)
				{
					QUICK_SCOPE_CYCLE_COUNTER(STAT_CitySampleCrowdVisualizationFragmentInitializer_ISMSetup);

					FCrowdCharacterDefinition CharacterDefinition;

					VisualizationList[i].SkinAtlasIndex = CharacterOptions.SkinTextureIndex;

					// If we found the data asset then we should have a valid set of Character Options already
					// from when we randomized
					CharacterOptions.GenerateCharacterDefinition(CrowdCharacterDataAsset, CharacterDefinition);

					FStaticMeshInstanceVisualizationDesc StaticMeshInstanceDesc;
					StaticMeshInstanceDesc.bUseTransformOffset = true;
					StaticMeshInstanceDesc.TransformOffset.SetRotation(FRotator(0, -90.0f, 0).Quaternion());

					const bool bUseFarLod = CVarUseISMFarLod.GetValueOnGameThread() == true;
					const float FarLodSignificanceThreshold = CrowdSettings ? CrowdSettings->ISMFarLodSignificanceThreshold : 4.0f;

					FStaticMeshInstanceVisualizationMeshDesc StaticMeshDesc;
					StaticMeshDesc.MinLODSignificance = 0.0f;
					StaticMeshDesc.MaxLODSignificance = bUseFarLod ? FarLodSignificanceThreshold : 4.0f;

					// Order of the static meshes is important here
					// UMassProcessor_CrowdVisualizationCustomData::UpdateCrowdCustomData assumes
					// 0 - Head
					// 1 - Body
					// 2 - Top
					// 3 - Bottom
					// 4 - Shoes

					if (UAnimToTextureDataAsset* ATTDA = GetAnimToTextureDataAsset(CharacterDefinition.HeadData))
					{
						ensureMsgf(ATTDA->GetStaticMesh(), TEXT("%s is missing static mesh %s"), *ATTDA->GetName(), *ATTDA->StaticMesh.ToString());

						StaticMeshDesc.Mesh = ATTDA->GetStaticMesh();
						StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);
					}

					if (UAnimToTextureDataAsset* ATTDA = GetAnimToTextureDataAsset(CharacterDefinition.BodyDefinition.BodyData))
					{
						ensureMsgf(ATTDA->GetStaticMesh(), TEXT("%s is missing static mesh %s"), *ATTDA->GetName(), *ATTDA->StaticMesh.ToString());

						StaticMeshDesc.Mesh = ATTDA->GetStaticMesh();
						StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);
						AnimationDataList[i].AnimToTextureData = ATTDA;
					}

					if (UAnimToTextureDataAsset* ATTDA = GetAnimToTextureDataAsset(CharacterDefinition.OutfitDefinition.TopData))
					{
						ensureMsgf(ATTDA->GetStaticMesh(), TEXT("%s is missing static mesh %s"), *ATTDA->GetName(), *ATTDA->StaticMesh.ToString());
						ensureMsgf(ATTDA->GetSkeletalMesh(), TEXT("%s is missing skeletal mesh %s"), *ATTDA->GetName(), *ATTDA->SkeletalMesh.ToString());

						StaticMeshDesc.Mesh = ATTDA->GetStaticMesh();
						StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);

						VisualizationList[i].TopColor = FindColorOverride(CharacterDefinition, ATTDA->GetSkeletalMesh());
					}

					if (UAnimToTextureDataAsset* ATTDA = GetAnimToTextureDataAsset(CharacterDefinition.OutfitDefinition.BottomData))
					{
						ensureMsgf(ATTDA->GetStaticMesh(), TEXT("%s is missing static mesh %s"), *ATTDA->GetName(), *ATTDA->StaticMesh.ToString());
						ensureMsgf(ATTDA->GetSkeletalMesh(), TEXT("%s is missing skeletal mesh %s"), *ATTDA->GetName(), *ATTDA->SkeletalMesh.ToString());

						StaticMeshDesc.Mesh = ATTDA->GetStaticMesh();
						StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);

						VisualizationList[i].BottomColor = FindColorOverride(CharacterDefinition, ATTDA->GetSkeletalMesh());
					}

					if (UAnimToTextureDataAsset* ATTDA = GetAnimToTextureDataAsset(CharacterDefinition.OutfitDefinition.ShoesData))
					{
						ensureMsgf(ATTDA->GetStaticMesh(), TEXT("%s is missing static mesh %s"), *ATTDA->GetName(), *ATTDA->StaticMesh.ToString());
						ensureMsgf(ATTDA->GetSkeletalMesh(), TEXT("%s is missing skeletal mesh %s"), *ATTDA->GetName(), *ATTDA->SkeletalMesh.ToString());

						StaticMeshDesc.Mesh = ATTDA->GetStaticMesh();
						StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);

						VisualizationList[i].ShoesColor = FindColorOverride(CharacterDefinition, ATTDA->GetSkeletalMesh());
					}

					const FCrowdHairDefinition& HairDefinition = CharacterDefinition.GetHairDefinitionForSlot(ECrowdHairSlots::Hair);
					if (UStaticMesh* HairStaticMesh = HairDefinition.GetGroomStaticMesh())
					{
						StaticMeshDesc.Mesh = HairStaticMesh;
						StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);
					}

					if (bUseFarLod)
					{
						FCrowdGenderDefinition& GenderDefinition = CharacterOptions.Skeleton == ECitySampleCrowdGender::A ? CrowdCharacterDataAsset->SkeletonA : CrowdCharacterDataAsset->SkeletonB;
						if(UAnimToTextureDataAsset* ATTDA = GetAnimToTextureDataAsset(GenderDefinition.FarLodMeshData))
						{
							const bool bUseFarLodMeshOverride = CVarUseISMFarLodMeshOverride.GetValueOnGameThread() == true;

							StaticMeshDesc.MinLODSignificance = FarLodSignificanceThreshold;
							StaticMeshDesc.MaxLODSignificance = 4.0f;
							
							UStaticMesh* MeshOverride = (bUseFarLodMeshOverride && CrowdSettings) ? CrowdSettings->GetISMFarLodMeshOverride() : nullptr;
							if (MeshOverride)
							{
								StaticMeshDesc.Mesh = MeshOverride;
							}
							else
							{
								StaticMeshDesc.Mesh = ATTDA->GetStaticMesh();
							}

							StaticMeshInstanceDesc.Meshes.Add(StaticMeshDesc);
						}
					}

					RepresentationList[i].StaticMeshDescIndex = RepresentationSubsystem->FindOrAddStaticMeshDesc(StaticMeshInstanceDesc);
				}
			}

#if WITH_EDITORONLY_DATA
			if (AnimationDataList[i].AnimToTextureData == nullptr && CitySampleCrowdCharacterCDO)
			{
				AnimationDataList[i].AnimToTextureData = CitySampleCrowdCharacterCDO->FallbackAnimToTextureDataAsset;
			}
#endif // WITH_EDITORONLY_DATA

			if (CrowdSettings)
			{
				if (CrowdSettings->bForceMassCrowdToAverage)
				{
					// Force body type to average for alpha
					VisualizationList[i].VisualizationID.AsMutableBitfield().BodyType = 0;
				}

				if (CrowdSettings->bHideAccessoriesForMassCrowd)
				{
					// Force off accessories for alpha
					VisualizationList[i].VisualizationID.AsMutableBitfield().AccessoryIndex = 0;
				}
			}
		}
	});

};


UAnimToTextureDataAsset* UCitySampleCrowdVisualizationFragmentInitializer::GetAnimToTextureDataAsset(TSoftObjectPtr<UAnimToTextureDataAsset> SoftPtr)
{
	if (SoftPtr.IsNull())
	{
		return nullptr;
	}

	if (SoftPtr.IsValid())
	{ 
		return SoftPtr.Get();
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_CitySampleCrowdVisualizationFragmentInitializer_GetAnimToTextureDataAsset_LoadSync);
		return SoftPtr.LoadSynchronous();
	}
}