// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdBlueprintLibrary.h"
#include "SkeletalMeshMerge.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimInstance.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "CitySample.h"
#include "Game/CitySampleGameInstanceBase.h"

// Editor only includes
#if WITH_EDITOR
#include "MeshUtilities.h"
#endif

DECLARE_LOG_CATEGORY_CLASS(LogCrowdBlueprint, Log, All);

void UCitySampleCrowdFunctionLibrary::SetMeshComponents(const FCrowdCharacterDefinition& CharacterDefinition,
	USkeletalMeshComponent* BaseComponent, USkeletalMeshComponent* BodyComponent, USkeletalMeshComponent* HeadComponent, 
	USkeletalMeshComponent* TopComponent, USkeletalMeshComponent* BottomComponent, USkeletalMeshComponent* ShoesComponent, 
	UStaticMeshComponent* AccessoryComponent)
{	
	SetMeshComponent(BaseComponent, CharacterDefinition.BodyDefinition.Base);
	SetMeshComponent(BodyComponent, ResolveSkeletalMesh(CharacterDefinition.BodyDefinition.BodyData));
	SetMeshComponent(HeadComponent, CharacterDefinition.Head);
	SetMeshComponent(TopComponent, ResolveSkeletalMesh(CharacterDefinition.OutfitDefinition.TopData));
	SetMeshComponent(BottomComponent, ResolveSkeletalMesh(CharacterDefinition.OutfitDefinition.BottomData));
	SetMeshComponent(ShoesComponent, ResolveSkeletalMesh(CharacterDefinition.OutfitDefinition.ShoesData));
	SetMeshComponent(AccessoryComponent, CharacterDefinition.AccessoryDefinition.Mesh);
}

void UCitySampleCrowdFunctionLibrary::SetGroomComponents(const UCrowdCharacterDataAsset* CrowdCharacterDataAsset, const FCrowdCharacterDefinition& CharacterDefinition,
	UGroomComponent* HairComponent, UGroomComponent* EyebrowsComponent,
	UGroomComponent* FuzzComponent, UGroomComponent* EyelashesComponent,
	UGroomComponent* MustacheComponent, UGroomComponent* BeardComponent, bool bUseCards, bool bEnableGroomBinding)
{
	// Create an array where the order matches the hair slots enum order
	TArray<UGroomComponent*> GroomComponents = {
		HairComponent,
		EyebrowsComponent,
		FuzzComponent,
		EyelashesComponent,
		MustacheComponent,
		BeardComponent
	};

	for (int SlotIdx = 0; SlotIdx < GroomComponents.Num(); ++SlotIdx)
	{
		const FCrowdHairDefinition& HairDefinition = CharacterDefinition.HairDefinitions[SlotIdx];

		SetGroomComponent(GroomComponents[SlotIdx], HairDefinition.Groom, bEnableGroomBinding ? HairDefinition.GroomBinding.LoadSynchronous() : nullptr, bUseCards);
	}
}

void UCitySampleCrowdFunctionLibrary::SetBodyMaterials(const FCrowdCharacterDefinition& CharacterDefinition, USkeletalMeshComponent* BodyComponent, const FName MaterialSlotName, 
	const FName BodyParameterName, USkeletalMeshComponent* BottomComponent /*=nullptr*/)
{
	if (!BodyComponent)
	{
		return;
	}

	// Load Texture
	UTexture2D* BodyTexture = ResolveSoftObjectPtr(CharacterDefinition.SkinTextureDefinition.BodyColor);
	if (!BodyTexture)
	{
		return;
	}

	// Create Material Instance and Set Parameters
	UMaterialInstanceDynamic* MaterialInstance = CreateDynamicMaterialInstance(BodyComponent, MaterialSlotName);
	if (!MaterialInstance)
	{
		return;
	}

	MaterialInstance->SetTextureParameterValue(BodyParameterName, BodyTexture);
	MaterialInstance->SetScalarParameterValue("AtlasSelector", CharacterDefinition.SkinTextureDefinition.TextureAtlasIndex);
	MaterialInstance->SetScalarParameterValue("MelaninRandomSeed", CharacterDefinition.SkinTextureModifierDefinition.OffsetU);
	MaterialInstance->SetScalarParameterValue("RednessRandomSeed", CharacterDefinition.SkinTextureModifierDefinition.OffsetV);
	MaterialInstance->SetScalarParameterValue("SourceU", CharacterDefinition.SkinTextureDefinition.SourceU);
	MaterialInstance->SetScalarParameterValue("SourceV", CharacterDefinition.SkinTextureDefinition.SourceV);
	MaterialInstance->SetScalarParameterValue("AlbedoDataMult", CharacterDefinition.SkinTextureDefinition.AlbedoMultiplier);

	//MaterialInstance->SetScalarParameterValue(SaturationParameterName, CharacterDefinition.TextureModifierDefinition.Saturation);
	//MaterialInstance->SetScalarParameterValue(PowerParameterName, CharacterDefinition.TextureModifierDefinition.Power);
	//MaterialInstance->SetScalarParameterValue(MultiplierParameterName, CharacterDefinition.TextureModifierDefinition.Multiplier);


	// Some outfits have exposed skin on the bottom half, if this is the case then we'll have it share the skin material from the upper body to prevent a mismatch
	static const FName BottomComponentBodySlot("M_Body");
	if (BottomComponent)
	{
		int32 BodySlotMaterialIndex = BottomComponent->GetMaterialIndex(BottomComponentBodySlot);
		if (BodySlotMaterialIndex != INDEX_NONE)
		{
			BottomComponent->SetMaterial(BodySlotMaterialIndex, MaterialInstance);
		}
	}

}

void UCitySampleCrowdFunctionLibrary::SetHairMaterials(const FCrowdCharacterDefinition& CharacterDefinition, UGroomComponent* HairComponent,
	const FName MelaninParameterName, const FName RednessParameterName, const FName RoughnessParameterName)
{
	if (!HairComponent)
	{
		return;
	}

	const TArray<FName> MaterialSlotNames = HairComponent->GetMaterialSlotNames();
	for (const FName& MaterialSlotName : MaterialSlotNames)
	{
		UMaterialInstanceDynamic* MaterialInstance = CreateDynamicMaterialInstance(HairComponent, MaterialSlotName);
		if (MaterialInstance)
		{
			//MaterialInstance->SetVectorParameterValue(ParameterName, CharacterDefinition.HairColorDefinition.Hair.ReinterpretAsLinear());
			MaterialInstance->SetScalarParameterValue(MelaninParameterName, CharacterDefinition.HairColorDefinition.HairMelanin);
			MaterialInstance->SetScalarParameterValue(RednessParameterName, CharacterDefinition.HairColorDefinition.HairRedness);
			MaterialInstance->SetScalarParameterValue(RoughnessParameterName, CharacterDefinition.HairColorDefinition.HairRoughness);
		}
	}
}

void UCitySampleCrowdFunctionLibrary::SetHeadMaterials(const FCrowdCharacterDefinition& CharacterDefinition, USkeletalMeshComponent* HeadComponent, const FName MaterialSlotName,
                                                   const FName FaceParameterName, const FName ChestParameterName, const int LODIndex)
{
	if (!HeadComponent)
	{
		return;
	}

	// Create Material Instance and Set Parameters
	UMaterialInstanceDynamic* MaterialInstance = CreateDynamicMaterialInstance(HeadComponent, MaterialSlotName);
	if (!MaterialInstance)
	{
		return;
	};

	// Load Face Texture
	UTexture2D* FaceTexture = ResolveSoftObjectPtr(CharacterDefinition.SkinTextureDefinition.FaceColor);
	if (FaceTexture)
	{
		MaterialInstance->SetTextureParameterValue(FaceParameterName, FaceTexture);
	}

	// Load Chest Texture
	UTexture2D* ChestTexture = ResolveSoftObjectPtr(CharacterDefinition.SkinTextureDefinition.ChestColor);
	if (ChestTexture)
	{
		MaterialInstance->SetTextureParameterValue(ChestParameterName, ChestTexture);
	}

	MaterialInstance->SetScalarParameterValue("AtlasSelector", CharacterDefinition.SkinTextureDefinition.TextureAtlasIndex);
	MaterialInstance->SetScalarParameterValue("MelaninRandomSeed", CharacterDefinition.SkinTextureModifierDefinition.OffsetU);
	MaterialInstance->SetScalarParameterValue("RednessRandomSeed", CharacterDefinition.SkinTextureModifierDefinition.OffsetV);
	MaterialInstance->SetScalarParameterValue("SourceU", CharacterDefinition.SkinTextureDefinition.SourceU);
	MaterialInstance->SetScalarParameterValue("SourceV", CharacterDefinition.SkinTextureDefinition.SourceV);
	MaterialInstance->SetScalarParameterValue("AlbedoDataMult", CharacterDefinition.SkinTextureDefinition.AlbedoMultiplier);

	// -- Update baked groom maps --

	// An array storing info about the different kinds of baked maps available and what the name of their texture parameter is
	// SlotIdx, AttributeMapName, FollicleMaskName, MelaninParameter, RednessParameter
	using FHairMapInfo = TTuple<uint8, FName, FName, FName, FName>;
	static const TArray<FHairMapInfo> HairMaps = {
		FHairMapInfo(static_cast<uint8>(ECrowdHairSlots::Beard),	FName("BeardAttributeMap"), FName("FollicleMaskBeard"), FName("BeardMelanin"), FName("BeardRedness")),
		FHairMapInfo(static_cast<uint8>(ECrowdHairSlots::Eyebrows), FName("EyebrowsAttributeMap"), NAME_None, FName("EyebrowsMelanin"), FName("EyebrowsRedness")),
		FHairMapInfo(static_cast<uint8>(ECrowdHairSlots::Hair),		FName("HairAttributeMap"), FName("FollicleMaskHair"), FName("HairMelanin"), FName("HairRedness")),
		FHairMapInfo(static_cast<uint8>(ECrowdHairSlots::Mustache), FName("MustacheAttributeMap"), FName("FollicleMaskMustache"), FName("MustacheMelanin"), FName("MustacheRedness"))
	};

	// Validate that we have enough HairDefinitions available to cover the hair slots
	// This should always be true hence the ensure
	uint8 NumHairSlots = static_cast<uint8>(ECrowdHairSlots::MAX);
	if (ensure(NumHairSlots <= CharacterDefinition.HairDefinitions.Num()))
	{
		for (const FHairMapInfo& HairMapInfo : HairMaps)
		{
			const uint8& HairSlotIdx = HairMapInfo.Get<0>();
			const FName& AttributeMapName = HairMapInfo.Get<1>();
			const FName& FollicleMaskName = HairMapInfo.Get<2>();
			const FName& MelaninParameterName = HairMapInfo.Get<3>();
			const FName& RednessParameterName = HairMapInfo.Get<4>();

			// Ensure Melanin and Redness matches hair color
			MaterialInstance->SetScalarParameterValue(MelaninParameterName, CharacterDefinition.HairColorDefinition.HairMelanin);
			MaterialInstance->SetScalarParameterValue(RednessParameterName, CharacterDefinition.HairColorDefinition.HairRedness);
			
			const FCrowdHairDefinition& HairDefinition = CharacterDefinition.HairDefinitions[HairSlotIdx];

			UCitySampleGameInstanceBase* GameInstance = nullptr;
			auto LazyGetGameInstance = [&GameInstance, HeadComponent]()
			{
				if (!GameInstance)
				{
					UWorld* World = HeadComponent->GetWorld();
					GameInstance = Cast<UCitySampleGameInstanceBase>(World ? World->GetGameInstance() : nullptr);
					if (!GameInstance)
					{
						GameInstance = GetMutableDefault<UCitySampleGameInstanceBase>();
					}
				}

				return GameInstance;
			};

			const bool bShouldUseBakedGroomMap = LODIndex >= HairDefinition.BakedGroomMinimumLOD;
			if ((bShouldUseBakedGroomMap || HairDefinition.Groom.IsNull()) && !HairDefinition.BakedGroomMap.IsNull())
			{
				// Grab the chosen baked groom map from the definition for use
				UTexture2D* BakedGroomMap = ResolveSoftObjectPtr(HairDefinition.BakedGroomMap);
				// ensure(BakedGroomMap);
				if (!BakedGroomMap)
				{
					UE_LOG(LogCitySample, Warning, TEXT("HairDefinition.BakedGroomMap ResolveSoftObjectPtr failed. BakedGroomMap asset name: '%s'."),
						*HairDefinition.BakedGroomMap.GetAssetName());
				}

				MaterialInstance->SetTextureParameterValue(AttributeMapName, BakedGroomMap);
			}
			else
			{
				// For various reasons we either don't have or don't want a baked groom map so we use the blank texture declared above
				MaterialInstance->SetTextureParameterValue(AttributeMapName, LazyGetGameInstance()->GetBaseGroomTexture());
			}

			if (!FollicleMaskName.IsNone())
			{
				if (!HairDefinition.FollicleMask.IsNull())
				{
					MaterialInstance->SetTextureParameterValue(FollicleMaskName, HairDefinition.FollicleMask.LoadSynchronous());
				}
				else
				{
					MaterialInstance->SetTextureParameterValue(FollicleMaskName, LazyGetGameInstance()->GetBaseFollicleMaskTexture());
				}
			}
		}
	}
}

void UCitySampleCrowdFunctionLibrary::SetOutfitMaterials(const FCrowdCharacterDefinition& CharacterDefinition, 
	USkeletalMeshComponent* TopComponent, USkeletalMeshComponent* BottomComponent, USkeletalMeshComponent* ShoesComponent, UStaticMeshComponent* AccessoryComponent)
{
	for (UMeshComponent* MeshComponent : TArray<UMeshComponent*>({ TopComponent, BottomComponent, ShoesComponent, AccessoryComponent }))
	{
		CharacterDefinition.OutfitMaterialDefinition.ApplyToComponent(MeshComponent, CharacterDefinition.PatternColorIndex, CharacterDefinition.PatternOptionIndex);
	}
}

void UCitySampleCrowdFunctionLibrary::AttachAccessory(const FCrowdCharacterDefinition& CharacterDefinition, USkeletalMeshComponent* BaseComponent, UStaticMeshComponent* AccessoryComponent)
{
	if (!BaseComponent || !BaseComponent->GetSkeletalMeshAsset() || !AccessoryComponent)
	{
		return;
	}

	const FName& SocketName = CharacterDefinition.AccessoryDefinition.SocketName;
		
	if (BaseComponent->DoesSocketExist(SocketName))
	{
		// Construct Transform
		
		const FVector LocalTranslation = CharacterDefinition.AccessoryDefinition.LocalPosition;

		const FVector LocalRotationVector = CharacterDefinition.AccessoryDefinition.LocalRotation;
		const FRotator LocalRotation(LocalRotationVector.Y, LocalRotationVector.Z, LocalRotationVector.X);

		FTransform LocalTransform(LocalRotation, LocalTranslation);

		// Snap Component
		AccessoryComponent->SetRelativeTransform(LocalTransform);
		AccessoryComponent->AttachToComponent(BaseComponent, FAttachmentTransformRules::KeepRelativeTransform, SocketName);	
	}	
}

void UCitySampleCrowdFunctionLibrary::AttachComponent(const FName SocketName, USkeletalMeshComponent* BaseComponent, USceneComponent* SceneComponent, const FVector LocalOffset)
{	
	if (!BaseComponent || !BaseComponent->GetSkeletalMeshAsset() || !SceneComponent)
	{
		return;
	}

	// Calculate Offset from Skeleton Ref Pose
	if (BaseComponent->DoesSocketExist(SocketName)) 
	{
		const FTransform& BoneTransform = GetWorldSpaceRefPoseTransform(SocketName, BaseComponent);
		
		// Snap Component
		SceneComponent->AttachToComponent(BaseComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		
		// Removes any existing transformation
		// this is needed if we are adding a local transform later, so it tis not additive in some scenarios.
		SceneComponent->ResetRelativeTransform();
		
		// Add Offset
		SceneComponent->AddLocalTransform(BoneTransform.Inverse(), false, nullptr, TeleportFlagToEnum(false));

		// Add Offset Delta
		SceneComponent->AddLocalOffset(LocalOffset, false, nullptr, TeleportFlagToEnum(false));
	}
}

void UCitySampleCrowdFunctionLibrary::ScaleComponentByFactor(const FCrowdCharacterDefinition& CharacterDefinition, USceneComponent* SceneComponent)
{
	if (!SceneComponent)
		return;

	SceneComponent->SetRelativeScale3D(FVector(CharacterDefinition.ScaleFactor));
}

// Copied from FAnimationRuntime::GetComponentSpaceTransform
FTransform UCitySampleCrowdFunctionLibrary::GetWorldSpaceRefPoseTransform(const FName SocketName, USkinnedMeshComponent* SkinnedMeshComponent)
{
	if (!SkinnedMeshComponent || !SkinnedMeshComponent->GetSkinnedAsset())
	{
		return FTransform::Identity;
	};

	// Get Ref Skeleton
	const FReferenceSkeleton& RefSkeleton = SkinnedMeshComponent->GetSkinnedAsset()->GetRefSkeleton();
		
	// Find Bone Index
	const int32 BoneIndex = SkinnedMeshComponent->GetBoneIndex(SocketName);

	if (RefSkeleton.IsValidIndex(BoneIndex))
	{
		// Get Bone Local Transforms
		const TArray<FTransform>& BoneSpaceTransforms = RefSkeleton.GetRefBonePose();

		// initialize to identity since some of them don't have tracks
		int32 IterBoneIndex = BoneIndex;
		FTransform CompTransform = BoneSpaceTransforms[BoneIndex];

		do
		{
			int32 ParentIndex = RefSkeleton.GetParentIndex(IterBoneIndex);
			if (ParentIndex != INDEX_NONE)
			{
				CompTransform = CompTransform * BoneSpaceTransforms[ParentIndex];
			}

			IterBoneIndex = ParentIndex;
		} while (RefSkeleton.IsValidIndex(IterBoneIndex));

		return CompTransform;
	}

	return FTransform::Identity;
}

void UCitySampleCrowdFunctionLibrary::GenerateCharacterDefinitionFromOptions(const FCrowdCharacterOptions& CharacterOptions,
	const UCrowdCharacterDataAsset* DataAsset, FCrowdCharacterDefinition& CharacterDefinition)
{
	CharacterOptions.GenerateCharacterDefinition(DataAsset, CharacterDefinition);
}

#if WITH_EDITOR
void UCitySampleCrowdFunctionLibrary::SetLODSettings(USkeletalMesh* Mesh, USkeletalMeshLODSettings* LODSettings)
{
	const int32 NumLODs = LODSettings->GetNumberOfSettings();

	// Add LODs
	while (Mesh->GetLODNum() < NumLODs)
	{
		Mesh->AddLODInfo();
	}

	// Set Settings
	Mesh->SetLODSettings(LODSettings);
}

void UCitySampleCrowdFunctionLibrary::EnableSkeletalMeshSkinCache(USkeletalMesh* Mesh, const TArray<ESkinCacheUsage> LODSkinCache)
{
	if (!Mesh)
	{
		return;
	}

	TArray<FSkeletalMeshLODInfo>& LODInfoArray = Mesh->GetLODInfoArray();

	bool bIsDirty = false;
	for (int32 LODIndex = 0; LODIndex < LODInfoArray.Num(); ++LODIndex)
	{		
		if (LODSkinCache.IsValidIndex(LODIndex))
		{
			FSkeletalMeshLODInfo& LODInfo = LODInfoArray[LODIndex];

			if (LODInfo.SkinCacheUsage != LODSkinCache[LODIndex])
			{
				LODInfo.SkinCacheUsage = LODSkinCache[LODIndex];
				bIsDirty = true;
			}
		}
	}

	// Mark as dirty
	if (bIsDirty)
	{
		Mesh->MarkPackageDirty();
	};
}

void UCitySampleCrowdFunctionLibrary::DisableSkeletalMeshPostProcess(USkeletalMesh* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	bool bIsDirty = false;
	if (Mesh->GetPostProcessAnimBlueprint())
	{
		TSubclassOf<UAnimInstance> InPostProcessAnimBlueprint;
		Mesh->SetPostProcessAnimBlueprint(InPostProcessAnimBlueprint);
		bIsDirty = true;
	}

	// Mark as dirty
	if (bIsDirty)
	{
		Mesh->MarkPackageDirty();
	}
}

void UCitySampleCrowdFunctionLibrary::SetGroomBindingType(UGroomAsset* Groom, const TArray<EGroomBindingType> LODBindingType)
{
	if (!Groom)
	{
		return;
	}

	bool bIsDirty = false;
	for (FHairGroupsLOD& HairGroupLOD : Groom->HairGroupsLOD)
	{
		//for (FHairLODSettings& HairLODSettings : HairGroupLOD.LODs)
		for (int32 LODIndex = 0; LODIndex < HairGroupLOD.LODs.Num(); ++LODIndex)
		{
			if (LODBindingType.IsValidIndex(LODIndex))
			{
				FHairLODSettings& HairLODSettings = HairGroupLOD.LODs[LODIndex];

				if (HairLODSettings.BindingType != LODBindingType[LODIndex])
				{
					HairLODSettings.BindingType = LODBindingType[LODIndex];
					bIsDirty = true;
				}
			}
		}
	}

	// Mark as dirty
	if (bIsDirty)
	{
		Groom->MarkPackageDirty();
	}
}

void UCitySampleCrowdFunctionLibrary::SetGroomGeometryType(UGroomAsset* Groom)
{
	if (!Groom)
	{
		return;
	}

	bool bIsDirty = false;
	for (FHairGroupsLOD& HairGroupLOD : Groom->HairGroupsLOD)
	{
		for (FHairLODSettings& HairLODSettings : HairGroupLOD.LODs)
		{			
			// Turn Strands to Cards
			if (HairLODSettings.GeometryType == EGroomGeometryType::Strands)
			{
				HairLODSettings.GeometryType = EGroomGeometryType::Cards;
				bIsDirty = true;
			}
		}
	}

	// Mark as dirty
	if (bIsDirty)
	{
		Groom->MarkPackageDirty();
	}
}

void UCitySampleCrowdFunctionLibrary::SetGroomInterpolationType(UGroomAsset* Groom, EGroomInterpolationType InterpolationType, bool bUseRBF)
{
	if (!Groom)
	{
		return;
	}

	// Set interpolation type
	bool bIsDirty = false;
	if (Groom->HairInterpolationType != InterpolationType)
	{
		Groom->HairInterpolationType = InterpolationType;
		bIsDirty = true;
	}

	// Set RBF
	if (Groom->EnableGlobalInterpolation != bUseRBF)
	{
		Groom->EnableGlobalInterpolation = bUseRBF;
		bIsDirty = true;
	}

	// Mark as dirty
	if (bIsDirty)
	{
		Groom->MarkPackageDirty();
	}
}

void UCitySampleCrowdFunctionLibrary::DisableGroomPhysics(UGroomAsset* Groom)
{
	if (!Groom)
	{
		return;
	}
	
	bool bIsDirty = false;
	for (FHairGroupsPhysics& HairGroupsPhysics : Groom->HairGroupsPhysics)
	{
		if (HairGroupsPhysics.SolverSettings.EnableSimulation)
		{
			HairGroupsPhysics.SolverSettings.EnableSimulation = false;
			bIsDirty = true;
		}
	}

	// Mark as dirty
	if (bIsDirty)
	{
		Groom->MarkPackageDirty();
	}
}

bool UCitySampleCrowdFunctionLibrary::RemoveAllMorphTargets(USkeletalMesh* Mesh)
{
	if (!Mesh)
	{
		return false;
	}

	// Skin of already empty.
	if (!Mesh->GetMorphTargets().Num())
	{
		return true;
	}

	// PostEditChange will be called after going out of scope.
	// This will run the following chain: ImportedData -> SourceModel -> RenderData
	{
		FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh);

		// Remove All Targets, need to be under the skeletalmesh scoped post edit to avoid triggerring a build that will recreate the morph targets stored in the LODImportData.
		Mesh->UnregisterAllMorphTarget();

		// CreateImportDataFromLODModel will create the BulkData from LODModel, but only if there is no existing data.
		// so we have to remove it first.
		for (int32 LODIndex = 0; LODIndex < Mesh->GetLODNum(); ++LODIndex)
		{
			Mesh->EmptyLODImportData(LODIndex);
		}

		const IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
		MeshUtilities.CreateImportDataFromLODModel(Mesh);
	}

	Mesh->MarkPackageDirty();

	return true;
}

bool UCitySampleCrowdFunctionLibrary::AddMorphTarget(USkeletalMesh* BaseMesh, USkeletalMesh* SourceMesh, USkeletalMesh* TargetMesh, 
	const FName MorphTargetName, const TArray<int32> Sections, const int32 UVChannel)
{
	if (!BaseMesh || !SourceMesh || !TargetMesh)
	{
		return false;
	}

	// Get Number of LODs
	const int32 NumLOD = BaseMesh->GetLODNum();

	// Get Base Mesh Sections
	const FSkeletalMeshModel* BaseMeshModel = BaseMesh->GetImportedModel();
	check(BaseMeshModel);

	// Allocate New MorphTarget
	UMorphTarget* MorphTarget = NewObject<UMorphTarget>(BaseMesh, MorphTargetName);
	
	// Loop thru LODs
	for (int32 LODIndex = 0 ; LODIndex < NumLOD; ++LODIndex)
	{	
		// Check Base LOD in Source and Target
		if (!BaseMesh->IsValidLODIndex(LODIndex) || !SourceMesh->IsValidLODIndex(LODIndex) || !TargetMesh->IsValidLODIndex(LODIndex))
		{
			continue;
		};

		//
		// The SkeletalMeshes might have different vertices even if same topology
		// We will create a mapping using closest vertex in uv space.
		//

		// Get Mapping Between Source and Target		
		TArray<FVector> SourceVertices;
		TArray<FVector> TargetVertices;
		TArray<int32> SourceToTargetMapping;
		if (!GetSkeletalMeshUVMapping(*SourceMesh, LODIndex, UVChannel, *TargetMesh, LODIndex, UVChannel,
			SourceVertices, TargetVertices, SourceToTargetMapping))
		{
			continue;
		}

		// Create Mapping Between Base and Source.
		TArray<int32> BaseToSourceMapping;
		if (BaseMesh != SourceMesh)
		{
			TArray<FVector> BaseVertices;
			if (!GetSkeletalMeshUVMapping(*BaseMesh, LODIndex, UVChannel, *SourceMesh, LODIndex, UVChannel,
				BaseVertices, SourceVertices, BaseToSourceMapping))
			{
				continue;
			}
		}
		// If Base and Source are the same mesh, use a one-to-one mapping
		else
		{
			TArray<FVector> BaseVertices;
			const int32 NumVertices = GetSkeletalMeshVertices(*BaseMesh, LODIndex, BaseVertices);

			BaseToSourceMapping.SetNum(NumVertices);
			for (int32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
			{
				BaseToSourceMapping[VertexIndex] = VertexIndex;
			}
		}

		// Computing Deltas on same topology
		const int32 BaseNumVertices = BaseToSourceMapping.Num();

		TArray<FMorphTargetDelta> Deltas;
		Deltas.SetNumZeroed(BaseNumVertices);

		for (int32 BaseVertexIndex = 0; BaseVertexIndex < BaseNumVertices; ++BaseVertexIndex)
		{
			// Skip vertex delta if not in given sections.
			bool bIsVertexInSections = false;
			if (!Sections.IsEmpty())
			{
				// Get LODModel
				const FSkeletalMeshLODModel& BaseMeshLODModel = BaseMeshModel->LODModels[LODIndex];

				for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
				{
					if (BaseMeshLODModel.Sections.IsValidIndex(SectionIndex))
					{
						const FSkelMeshSection& Section = BaseMeshLODModel.Sections[SectionIndex];

						// Check if vertex out of range
						if (BaseVertexIndex >= (int32)Section.BaseVertexIndex &&
							BaseVertexIndex < (int32)Section.NumVertices)
						{
							bIsVertexInSections = true;
						}
					}
				}
			}
			else
			{
				bIsVertexInSections = true;
			}

			// Compute Delta
			if (bIsVertexInSections)
			{
				const int32 SourceVertexIndex = BaseToSourceMapping[BaseVertexIndex];
				const int32 TargetVertexIndex = SourceToTargetMapping[SourceVertexIndex];

				Deltas[BaseVertexIndex].PositionDelta = FVector3f(TargetVertices[TargetVertexIndex] - SourceVertices[SourceVertexIndex]);
				Deltas[BaseVertexIndex].SourceIdx = BaseVertexIndex;
			}
		}		

		// Populate Deltas
		const bool bCompareNormals = false;
		const bool bGeneratedByReductionSetting = false;
		MorphTarget->PopulateDeltas(Deltas, LODIndex, BaseMeshModel->LODModels[LODIndex].Sections, bCompareNormals, bGeneratedByReductionSetting);
	}

	// Skip of no valid data
	if (!MorphTarget->HasValidData())
	{
		return false;
		//UE_LOG(LogTemp, Warning, TEXT("Invalid data: %s"), *MorphTargetName.ToString())
	}
	
	// Register Morph Target
	const bool bInvalidateRenderData = false;
	if (BaseMesh->RegisterMorphTarget(MorphTarget, bInvalidateRenderData))
	{
		// PostEditChange will be called after going out of scope.
		// This will run the following chain: ImportedData -> SourceModel -> RenderData
		{
			FScopedSkeletalMeshPostEditChange ScopedPostEditChange(BaseMesh);

			// Update MorphTargetIndexMap
			BaseMesh->InitMorphTargets();

			// CreateImportDataFromLODModel will create the BulkData from LODModel, but only if there is no existing data.
			// so we have to remove it first.
			for (int32 LODIndex = 0; LODIndex < NumLOD; ++LODIndex)
			{
				BaseMesh->EmptyLODImportData(LODIndex);
			}

			const IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
			MeshUtilities.CreateImportDataFromLODModel(BaseMesh);
		}

		BaseMesh->MarkPackageDirty();

		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to register MorphTarget: %s"), *MorphTargetName.ToString())
	}

	return false;
};

int32 UCitySampleCrowdFunctionLibrary::GetSkeletalMeshVertices(const USkeletalMesh& Mesh, const int32 LODIndex, TArray<FVector>& Vertices)
{
	Vertices.Reset();

	// Get Imported Model
	const FSkeletalMeshModel* MeshModel = Mesh.GetImportedModel();
	check(MeshModel);

	if (!MeshModel->LODModels.IsValidIndex(LODIndex))
	{
		return INDEX_NONE;
	};

	// Get Vertices
	TArray<FSoftSkinVertex> SoftSkinVertices;
	MeshModel->LODModels[LODIndex].GetVertices(SoftSkinVertices);

	const int32 NumVertices = SoftSkinVertices.Num();
	Vertices.SetNumUninitialized(NumVertices);
	
	for (int32 VertexIndex = 0; VertexIndex < SoftSkinVertices.Num(); ++VertexIndex)
	{
		Vertices[VertexIndex] = (FVector)SoftSkinVertices[VertexIndex].Position;
	}

	return NumVertices;
}

int32 UCitySampleCrowdFunctionLibrary::GetSkeletalMeshVerticesAndUVs(const USkeletalMesh& Mesh, const int32 LODIndex, const int32 UVChannel, 
	TArray<FVector>& Vertices, TArray<FVector2D>& UVs)
{
	Vertices.Reset();
	UVs.Reset();

	// Get Imported Model
	const FSkeletalMeshModel* MeshModel = Mesh.GetImportedModel();
	check(MeshModel);

	if (!MeshModel->LODModels.IsValidIndex(LODIndex))
	{
		return INDEX_NONE;
	};

	if (UVChannel >= (int32)MeshModel->LODModels[LODIndex].NumTexCoords)
	{
		return INDEX_NONE;
	}

	// Get Vertices
	TArray<FSoftSkinVertex> SoftSkinVertices;
	MeshModel->LODModels[LODIndex].GetVertices(SoftSkinVertices);

	const int32 NumVertices = SoftSkinVertices.Num();
	Vertices.SetNumUninitialized(NumVertices);
	UVs.SetNumUninitialized(NumVertices);

	for (int32 VertexIndex = 0; VertexIndex < SoftSkinVertices.Num(); ++VertexIndex)
	{
		Vertices[VertexIndex] = (FVector)SoftSkinVertices[VertexIndex].Position;
		UVs[VertexIndex] = FVector2D(SoftSkinVertices[VertexIndex].UVs[UVChannel]);
	}

	return NumVertices;
}

bool UCitySampleCrowdFunctionLibrary::GetSkeletalMeshUVMapping(
	const USkeletalMesh& SkeletalMesh, const int32 LODIndex, const int32 UVChannel,
	const USkeletalMesh& OtherSkeletalMesh, const int32 OtherLODIndex, const int32 OtherUVChannel,
	TArray<FVector>& Vertices, TArray<FVector>& OtherVertices, TArray<int32>& Mapping)
{
	Vertices.Reset();
	OtherVertices.Reset();
	Mapping.Reset();

	// Get Vertices and UVs
	//TArray<FVector> Vertices;
	TArray<FVector2D> UVs;
	const int32 NumVertices = GetSkeletalMeshVerticesAndUVs(SkeletalMesh, LODIndex, UVChannel, Vertices, UVs);
	if (NumVertices == INDEX_NONE)
	{
		return false;
	}

	// Get Vertices and UVs
	//TArray<FVector> OtherVertices;
	TArray<FVector2D> OtherUVs;
	const int32 OtherNumVertices = GetSkeletalMeshVerticesAndUVs(OtherSkeletalMesh, OtherLODIndex, OtherUVChannel, OtherVertices, OtherUVs);
	if (OtherNumVertices == INDEX_NONE)
	{
		return false;
	}

	// Allocate Mapping
	Mapping.SetNumUninitialized(NumVertices);

	// Find Closest Vertices using UVs
	for (int32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
	{
		float MinDistance = TNumericLimits<float>::Max();
		int32 ClosestIndex = INDEX_NONE;

		// Find Closest SkeletalMesh Vertex.
		for (int32 OtherVertexIndex = 0; OtherVertexIndex < OtherNumVertices; ++OtherVertexIndex)
		{
			const float Distance = FVector2D::Distance(UVs[VertexIndex], OtherUVs[OtherVertexIndex]);
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestIndex = OtherVertexIndex;
			}
		}

		Mapping[VertexIndex] = ClosestIndex;
	}

	return true;
}

void UCitySampleCrowdFunctionLibrary::RecomputeTangents(USkeletalMesh* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	// Recompute Tangents
	{
		// PostEditChange will be called after going out of scope.
		// This will run the following chain: ImportedData -> SourceModel -> RenderData
		FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh);

		// Enable Recompute Tangents from BuildSettings
		{
			TArray<FSkeletalMeshLODInfo>& LODInfoArray = Mesh->GetLODInfoArray();
			for (FSkeletalMeshLODInfo& LODInfo : LODInfoArray)
			{
				LODInfo.BuildSettings.bRecomputeNormals = true;
				LODInfo.BuildSettings.bRecomputeTangents = true;
			}
		}

		// Force the creation of a new GUID use to build the derive data cache key.
		// Next time a build happen the whole skeletal mesh will be rebuild.
		Mesh->InvalidateDeriveDataCacheGUID();

		const IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
		MeshUtilities.CreateImportDataFromLODModel(Mesh);
	}
	
	// Disable Recompute Tangents from Material Sections
	{
		FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh); // This will update the RenderRegions

		FSkeletalMeshModel* MeshModel = Mesh->GetImportedModel();
		check(MeshModel);

		// Loop thru LODs
		for (FSkeletalMeshLODModel& LODModel : MeshModel->LODModels)
		{
			// Loop thru LODs
			for (FSkelMeshSection& Section : LODModel.Sections)
			{
				Section.bRecomputeTangent = false;
				Section.RecomputeTangentsVertexMaskChannel = ESkinVertexColorChannel::None; // This None is actually All				

				// Set SourceSectionUserData
				FSkelMeshSourceSectionUserData& SourceSectionUserData = LODModel.UserSectionsData.FindOrAdd(Section.OriginalDataSectionIndex);
				SourceSectionUserData.bRecomputeTangent = false;
				SourceSectionUserData.RecomputeTangentsVertexMaskChannel = ESkinVertexColorChannel::None;
			}
		}
	}

	// Mark as dirty
	Mesh->MarkPackageDirty();

	return;
}

UStaticMesh* UCitySampleCrowdFunctionLibrary::GetHairCardsImportedMesh(const FHairGroupsCardsSourceDescription& CardsSourceDescription)
{
	return CardsSourceDescription.ImportedMesh;
}

#endif // WITH_EDITOR

UMaterialInstanceDynamic* UCitySampleCrowdFunctionLibrary::CreateDynamicMaterialInstance(UMeshComponent* Component, const FName MaterialSlotName)
{
	if (!Component)
	{
		return nullptr;
	}

	// Find Material by SlotName
	const int32 ElementIndex = Component->GetMaterialIndex(MaterialSlotName);
	if (ElementIndex == INDEX_NONE)
	{
		return nullptr;
	}

	// Get Material
	UMaterialInterface* MaterialInterface = nullptr;

	// There is an issue with skeletal mesh components where the materials on the component may not match the
	// materials defined in the actual skeletal mesh asset. To avoid this we'll query the mesh where possible.
	if (USkinnedMeshComponent* SkinnedComponent = Cast<USkinnedMeshComponent>(Component))
	{
		if (!SkinnedComponent->GetSkinnedAsset())
		{
			return nullptr;
		}
		
		TArray<FSkeletalMaterial>& MeshMaterials = SkinnedComponent->GetSkinnedAsset()->GetMaterials();
		if (!MeshMaterials.IsValidIndex(ElementIndex))
		{
			return nullptr;
		}
	
		MaterialInterface = MeshMaterials[ElementIndex].MaterialInterface;
	}
	else
	{
		MaterialInterface = Component->GetMaterial(ElementIndex);
	}
	
	if (!MaterialInterface)
	{
		return nullptr;
	}

	// Create Dynamic Material Instance
	return Component->CreateDynamicMaterialInstance(ElementIndex, MaterialInterface);
};

TSoftObjectPtr<USkeletalMesh> UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(const TSoftObjectPtr<UAnimToTextureDataAsset>& DataAssetSoftPtr)
{
	TSoftObjectPtr<USkeletalMesh> FoundMesh = nullptr;
	
	if (UAnimToTextureDataAsset* DataAsset = ResolveSoftObjectPtr(DataAssetSoftPtr))
	{
		FoundMesh = DataAsset->SkeletalMesh;
	}

	return FoundMesh;
}

TSoftObjectPtr<USkeletalMesh> UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(const TSoftObjectPtr<UAnimToTextureDataAsset>& DataAssetSoftPtr, const TSoftObjectPtr<USkeletalMesh>& FallbackMesh)
{
	TSoftObjectPtr<USkeletalMesh> FoundMesh = nullptr;
	
	if (UAnimToTextureDataAsset* DataAsset = ResolveSoftObjectPtr(DataAssetSoftPtr))
	{
		FoundMesh = DataAsset->SkeletalMesh;
	}

	if (!FoundMesh)
	{
		FoundMesh = FallbackMesh;
	}

	return FoundMesh;
}

TSoftObjectPtr<USkeletalMesh> UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(const TArray<TSoftObjectPtr<UAnimToTextureDataAsset>>& DataArray, const int32 Index)
{
	TSoftObjectPtr<USkeletalMesh> FoundMesh = nullptr;
	
	if (DataArray.Num())
	{
		const TSoftObjectPtr<UAnimToTextureDataAsset> DataAssetSoftPtr = DataArray[Index % DataArray.Num()];
		if (UAnimToTextureDataAsset* DataAsset = ResolveSoftObjectPtr(DataAssetSoftPtr))
		{
			FoundMesh = DataAsset->SkeletalMesh;
		}
	}

	return FoundMesh;
}

TSoftObjectPtr<USkeletalMesh> UCitySampleCrowdFunctionLibrary::ResolveSkeletalMesh(const TArray<TSoftObjectPtr<UAnimToTextureDataAsset>>& DataArray, const TArray<TSoftObjectPtr<USkeletalMesh>>& FallbackMeshes, const int32 Index)
{
	TSoftObjectPtr<USkeletalMesh> FoundMesh = nullptr;
	
	if (DataArray.Num())
	{
		const TSoftObjectPtr<UAnimToTextureDataAsset> DataAssetSoftPtr = DataArray[Index % DataArray.Num()];
		if (UAnimToTextureDataAsset* DataAsset = ResolveSoftObjectPtr(DataAssetSoftPtr))
		{
			FoundMesh = DataAsset->SkeletalMesh;
		}
	}

	if (!FoundMesh)
	{
		FoundMesh = FallbackMeshes[Index % FallbackMeshes.Num()];
	}

	return FoundMesh;
}