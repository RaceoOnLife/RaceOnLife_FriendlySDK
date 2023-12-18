// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GroomComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CrowdCharacterDataAsset.h"
#include "CrowdCharacterDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "CrowdBlueprintLibrary.generated.h"

/**
	Set of utilities for dealing with the Crowd Definition Objects.
*/
UCLASS()
class CITYSAMPLE_API UCitySampleCrowdFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/* Sets Mesh Components for Body, Outfit and Hair. 
	*  All the Outfit Meshes described in the CharacterDefinition are merged in one single SkelMesh
	*/
	UFUNCTION(BlueprintCallable)
	static void SetMeshComponents(const FCrowdCharacterDefinition& CharacterDefinition,
		USkeletalMeshComponent* BaseComponent, USkeletalMeshComponent* BodyComponent, USkeletalMeshComponent* HeadComponent,
		USkeletalMeshComponent* TopComponent, USkeletalMeshComponent* BottomComponent, USkeletalMeshComponent* ShoesComponent,
		UStaticMeshComponent* AccessoryComponent);

	/* Sets All Groom Components */
	UFUNCTION(BlueprintCallable)
	static void SetGroomComponents(const UCrowdCharacterDataAsset* CrowdCharacterDataAsset, const FCrowdCharacterDefinition& CharacterDefinition,
		UGroomComponent* Hair, UGroomComponent* Eyebrows,
		UGroomComponent* Fuzz, UGroomComponent* Eyelashes,
		UGroomComponent* Mustache, UGroomComponent* Beard, bool bUseCards=true, bool bEnableGroomBinding=true);

	// ---------------------------------------------------------------------------------------------------

	/* Set Hair Material Instance Parameters 
	*  The Hair meshes have multiple materials, for different LODs: cards, helmets, etc.
	*  This function will override the same parameter for all materials.
	*/
	UFUNCTION(BlueprintCallable)
	static void SetHairMaterials(const FCrowdCharacterDefinition& CharacterDefinition, UGroomComponent* HairComponent,
		const FName MelaninParameterName, const FName RednessParameterName, const FName RoughnessParameterName);

	/* Set Head Material Instance Parameters
	*  This overrides the texture for face and chest.
	*/
	UFUNCTION(BlueprintCallable)
	static void SetHeadMaterials(const FCrowdCharacterDefinition& CharacterDefinition, USkeletalMeshComponent* HeadComponent, const FName MaterialSlotName,
	                             const FName FaceParameterName, const FName ChestParameterName, const int LODIndex);

	/* Set Head Material Instance Parameters
	*/
	UFUNCTION(BlueprintCallable)
	static void SetBodyMaterials(const FCrowdCharacterDefinition& CharacterDefinition, USkeletalMeshComponent* BodyComponent, const FName MaterialSlotName,
		const FName BodyParameterName, USkeletalMeshComponent* BottomComponent = nullptr);

	/* Sets Dynamic Material Instances for Outfit.
	*/
	UFUNCTION(BlueprintCallable)
	static void SetOutfitMaterials(const FCrowdCharacterDefinition& CharacterDefinition, 
		USkeletalMeshComponent* TopComponent, USkeletalMeshComponent* BottomComponent, USkeletalMeshComponent* ShoesComponent, UStaticMeshComponent* AccessoryComponent);

	// ---------------------------------------------------------------------------------------------------

	/* Returns Reference Pose Socket Transform in WorldSpace */
	UFUNCTION(BlueprintCallable)
	static FTransform GetWorldSpaceRefPoseTransform(const FName SocketName, USkinnedMeshComponent* SkinnedMeshComponent);

	/** Snaps Accessory to the Socket defined in the AccessoryDefinition */
	UFUNCTION(BlueprintCallable)
	static void AttachAccessory(const FCrowdCharacterDefinition& CharacterDefinition, USkeletalMeshComponent* BaseComponent, UStaticMeshComponent* AccessoryComponent);

	/** Attach Component to socket keeping Offset. 
	*   Local offset can be used for adding an additional translation delta after.
	*/
	UFUNCTION(BlueprintCallable)
	static void AttachComponent(const FName SocketName, USkeletalMeshComponent* BaseComponent, USceneComponent* SceneComponent, const FVector LocalOffset);

	/** Scales given component by the Character Definition Scale Factor */
	UFUNCTION(BlueprintCallable)
	static void ScaleComponentByFactor(const FCrowdCharacterDefinition& CharacterDefinition, USceneComponent* SceneComponent);

	// ----------------------------------------------------------------------------------------------------

	// Utility functions to simplify resolving a skeletal mesh when there are both dataassets and direct soft pointers
	static TSoftObjectPtr<USkeletalMesh> ResolveSkeletalMesh(const TSoftObjectPtr<UAnimToTextureDataAsset>& DataAssetSoftPtr);
	static TSoftObjectPtr<USkeletalMesh> ResolveSkeletalMesh(const TSoftObjectPtr<UAnimToTextureDataAsset>& DataAssetSoftPtr, const TSoftObjectPtr<USkeletalMesh>& FallbackMesh);

	static TSoftObjectPtr<USkeletalMesh> ResolveSkeletalMesh(const TArray<TSoftObjectPtr<UAnimToTextureDataAsset>>& DataArray, const int32 Index);
	static TSoftObjectPtr<USkeletalMesh> ResolveSkeletalMesh(const TArray<TSoftObjectPtr<UAnimToTextureDataAsset>>& DataArray, const TArray<TSoftObjectPtr<USkeletalMesh>>& FallbackMeshes, const int32 Index);

// -----------------------------------------------------------------------
//							METAHUMAN UTILITIES
// -----------------------------------------------------------------------

#if WITH_EDITOR
	
	/** Sets SkeletalMesh with LODSettings. 
	* This makes sure the SkelMesh has enough LODInfo Data */
	UFUNCTION(BlueprintCallable)
	static void SetLODSettings(USkeletalMesh* Mesh, USkeletalMeshLODSettings* LODSettings);

	/**
	* Enable SkeletalMeshLODs SkinCache.
	*/
	UFUNCTION(BlueprintCallable)
	static void EnableSkeletalMeshSkinCache(USkeletalMesh* Mesh, const TArray<ESkinCacheUsage> LODSkinCache);

	/**
	* Disable SkeletalMesh PostProcess AnimBP.
	*/
	UFUNCTION(BlueprintCallable)
	static void DisableSkeletalMeshPostProcess(USkeletalMesh* Mesh);

	/**
	* Sets GroomAsset LOD Binding to given type. Strands and Cards: Skinning, Meshes: Rigid.
	*/
	UFUNCTION(BlueprintCallable)
	static void SetGroomBindingType(UGroomAsset* Groom, const TArray<EGroomBindingType> LODBindingType);
	
	/**
	* Turns all Strands to Cards
	*/
	UFUNCTION(BlueprintCallable)
	static void SetGroomGeometryType(UGroomAsset* Groom);

	/**
	* Sets GroomAsset LOD Binding to given type. Strands and Cards: Skinning, Meshes: Rigid.
	*/
	UFUNCTION(BlueprintCallable)
	static void SetGroomInterpolationType(UGroomAsset* Groom, 
		EGroomInterpolationType InterpolationType, bool bUseRBF=false);

	/**
	* Disable GroomAsset Physics
	*/
	UFUNCTION(BlueprintCallable)
	static void DisableGroomPhysics(UGroomAsset* Groom);

	/*
	* Remove All Targets
	*/
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Sections"))
	static bool RemoveAllMorphTargets(USkeletalMesh* Mesh);

	/* 
	* Adds a MorphTarget to Base using the difference between Source and Target.
	*/
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Sections"))
	static bool AddMorphTarget(USkeletalMesh* BaseMesh, USkeletalMesh* SourceMesh, USkeletalMesh* TargetMesh,
		const FName MorphName, const TArray<int32> Sections, const int32 UVChannel = 0);

	/* Returns SkeletalMesh Vertex Position */
	static int32 GetSkeletalMeshVertices(const USkeletalMesh& Mesh, const int32 LODIndex,
        TArray<FVector>& Vertices);

	/* Returns SkeletalMesh Vertex Position and UVs */
	static int32 GetSkeletalMeshVerticesAndUVs(const USkeletalMesh& Mesh, const int32 LODIndex, const int32 UVChannel,
        TArray<FVector>& Vertices, TArray<FVector2D>& UVs);

	/*
	* Computes Vertex Mapping between 2 SkeletalMeshes using UV coordinates */
	static bool GetSkeletalMeshUVMapping(
        const USkeletalMesh& SkeletalMesh, const int32 LODIndex, const int32 UVChannel,
        const USkeletalMesh& OtherSkeletalMesh, const int32 OtherLODIndex, const int32 OtherUVChannel,
        TArray<FVector>& Vertices, TArray<FVector>& OtherVertices,
        TArray<int32>& Mapping);

	/* Recompute Tangents and Disable Material RecomputeTanents */
	UFUNCTION(BlueprintCallable)
	static void RecomputeTangents(USkeletalMesh* Mesh);

	UFUNCTION(BlueprintPure)
	static UStaticMesh* GetHairCardsImportedMesh(const FHairGroupsCardsSourceDescription& CardsSourceDescription);

#endif // WITH_EDITOR

	// ------------------------------------------------------------------------

	// Uses a provided data asset to resolve a set of character options to a complete character definition
	UFUNCTION(BlueprintCallable, Category="Character")
    static void GenerateCharacterDefinitionFromOptions(const FCrowdCharacterOptions& CharacterOptions, const UCrowdCharacterDataAsset* DataAsset, FCrowdCharacterDefinition& CharacterDefinition);

	/* Utility for creating and setting Colors with Dynamic Material Instance */
	static UMaterialInstanceDynamic* CreateDynamicMaterialInstance(UMeshComponent* Component, const FName MaterialSlotName);
private:

	template<typename AssetType>
	static AssetType* ResolveSoftObjectPtr(const TSoftObjectPtr<AssetType>& AssetPointer)
	{
		AssetType* ReturnVal = nullptr;
		if (AssetPointer.ToSoftObjectPath().IsValid())
		{
			ReturnVal = AssetPointer.Get();
			if (!ReturnVal)
			{
				AssetType* LoadedAsset = Cast<AssetType>(AssetPointer.ToSoftObjectPath().TryLoad());
				if (ensureMsgf(LoadedAsset, TEXT("Failed to load asset pointer %s"), *AssetPointer.ToString()))
				{
					ReturnVal = LoadedAsset;
				}
			}
		}
		return ReturnVal;
	}

	FORCEINLINE static void SetMeshComponent(USkeletalMeshComponent* MeshComponent, TSoftObjectPtr<USkeletalMesh> MeshPtr)
	{
		if (MeshComponent)
		{
			USkeletalMesh* Mesh = ResolveSoftObjectPtr(MeshPtr);
			MeshComponent->SetSkeletalMesh(Mesh);
		}
	}

	FORCEINLINE static void SetMeshComponent(UStaticMeshComponent* MeshComponent, TSoftObjectPtr<UStaticMesh> MeshPtr)
	{
		if (MeshComponent)
		{
			UStaticMesh* Mesh = ResolveSoftObjectPtr(MeshPtr);
			MeshComponent->SetStaticMesh(Mesh);
		}
	}

	FORCEINLINE static void SetGroomComponent(UGroomComponent* GroomComponent, TSoftObjectPtr<UGroomAsset> GroomPtr,
		UGroomBindingAsset* GroomBinding, bool bUseCards)
	{
		if (GroomComponent)
		{
			// Set Groom
			UGroomAsset* Groom = ResolveSoftObjectPtr(GroomPtr);
			GroomComponent->SetGroomAsset(Groom);

			// Set Binding
			GroomComponent->SetBindingAsset(GroomBinding);

			// Set Cards
			GroomComponent->bUseCards = bUseCards;
		}
	}
	
};
