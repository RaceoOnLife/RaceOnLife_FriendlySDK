// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleMeshMerging.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkeletalMeshSocket.h"
#include "IContentBrowserSingleton.h"
#include "IMeshMergeUtilities.h"
#include "MeshMergeModule.h"
#include "ScopedTransaction.h"
#include "MeshUtilities.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/ScopedSlowTask.h"
#include "RawMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ShapeComponent.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "IGeometryProcessingInterfacesModule.h"
#include "GeometryProcessingInterfaces/ApproximateActors.h"

#define LOCTEXT_NAMESPACE "CitySampleMeshMerging"

DEFINE_LOG_CATEGORY(LogCitySampleMeshMerging);

namespace HackMeshMerging
{
	/** Data structure used to keep track of the selected mesh components, and whether or not they should be incorporated in the merge */
	struct FMergeComponentData
	{
		FMergeComponentData(UPrimitiveComponent* InPrimComponent)
			: PrimComponent(InPrimComponent)
			, bShouldIncorporate(true)
		{}

		/** Component extracted from selected actors */
		TWeakObjectPtr<UPrimitiveComponent> PrimComponent;
		/** Flag determining whether or not this component should be incorporated into the merge */
		bool bShouldIncorporate;
	};

	void BuildMergeDataFromActorsArray(const TArray<AActor*>& InActors, TArray<TSharedPtr<FMergeComponentData>>& OutComponentsData, TArray<AActor*>& OutActors, TArray<ULevel*>* OutLevels, bool bAllowShapeComponents = false)
	{
		OutComponentsData.Empty();

		TSet<AActor*> Actors;

		for (AActor* Actor : InActors)
		{
			if (Actor)
			{
				Actors.Add(Actor);

				// Add child actors & actors found under foundations
				Actor->EditorGetUnderlyingActors(Actors);
			}
		}

		for (AActor* Actor : Actors)
		{
			check(Actor != nullptr);

			TArray<UPrimitiveComponent*> PrimComponents;
			Actor->GetComponents<UPrimitiveComponent>(PrimComponents);
			for (UPrimitiveComponent* PrimComponent : PrimComponents)
			{
				bool bInclude = false; // Should put into UI list
				bool bShouldIncorporate = false; // Should default to part of merged mesh
				bool bIsMesh = false;
				if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimComponent))
				{
					bShouldIncorporate = (StaticMeshComponent->GetStaticMesh() != nullptr);
					bInclude = true;
					bIsMesh = true;
				}
				else if (UShapeComponent* ShapeComponent = Cast<UShapeComponent>(PrimComponent))
				{
					if (bAllowShapeComponents)
					{
						bShouldIncorporate = true;
						bInclude = true;
					}
				}

				if (bInclude)
				{
					// Functionality copied from BuildMergeComponentDataFromSelection
					OutComponentsData.Add(MakeShared<FMergeComponentData>(PrimComponent));
					TSharedPtr<FMergeComponentData>& ComponentData = OutComponentsData.Last();
					ComponentData->bShouldIncorporate = bShouldIncorporate;

					// Functionality copied from BuildActorsListFromMergeComponentsData
					if (ComponentData->PrimComponent.IsValid())
					{
						OutActors.AddUnique(Actor);
						if (OutLevels)
						{
							OutLevels->AddUnique(Actor->GetLevel());
						}
					}
				}
			}
		}
	}
}

USkeletalMesh* UCitySampleMeshMergingBlueprintLibrary::MergeSkeletalMeshes(TArray<USkeletalMesh*> SkeletalMeshes, const FString PackageName, USkeleton* Skeleton)
{
	if (SkeletalMeshes.IsEmpty())
	{
		return nullptr;
	}

	USkeletalMesh* OutSkeletalMesh = nullptr;
	OutSkeletalMesh = NewObject<USkeletalMesh>();

	if (!OutSkeletalMesh)
	{
		return nullptr;
	}

	// Convert FSkelMeshMergeSectionMapping_MBP to FSkelMeshMergeSectionMapping
	// They are basically same data, but the custom one is exposed in Blueprints.
	TArray<FSkelMeshMergeSectionMapping> SectionMappings;

	// Set Skeleton
	if (!Skeleton)
	{
		Skeleton = SkeletalMeshes[0]->GetSkeleton();
	}
	OutSkeletalMesh->SetSkeleton(Skeleton);

	// Setup Merger
	const int32 StripTopLODs = 0;
	const EMeshBufferAccess bMeshNeedsCPUAccess = EMeshBufferAccess::Default;
	FSkelMeshMergeUVTransformMapping* UvTransforms = nullptr;
	FSkeletalMeshMerge Merger(OutSkeletalMesh, SkeletalMeshes, SectionMappings, StripTopLODs, bMeshNeedsCPUAccess, UvTransforms);

	// Merge Meshes.
	if (Merger.DoMerge())
	{
		//OutSkeletalMesh->MarkPackageDirty();
		return OutSkeletalMesh;
	}
	// Something went wrong.
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Merge failed!"));
		return nullptr;
	}
}

bool UCitySampleMeshMergingBlueprintLibrary::MergeActors(const FString PackageName, const TArray<AActor*> ActorsToMerge, const FMeshMergingSettings& MergeSettings, const bool bReplaceSourceActors /*= false*/, const bool bAllowShapeComponents /*= true*/)
{
	const IMeshMergeUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	TArray<TSharedPtr<HackMeshMerging::FMergeComponentData>> SelectedComponents;

	BuildMergeDataFromActorsArray(ActorsToMerge, SelectedComponents, Actors, bReplaceSourceActors ? &UniqueLevels : nullptr, bAllowShapeComponents);

	// This restriction is only for replacement of selected actors with merged mesh actor
	if (UniqueLevels.Num() > 1 && bReplaceSourceActors)
	{
		UE_LOG(LogCitySampleMeshMerging, Error, TEXT("Unable to merge actors: The selected actors should be in the same level"))
		return false;
	}

	FVector MergedActorLocation;
	TArray<UObject*> AssetsToSync;
	// Merge...
	{
		FScopedSlowTask SlowTask(0, LOCTEXT("MergingActorsSlowTask", "Merging actors..."));
		SlowTask.MakeDialog();

		// Extracting static mesh components from the selected mesh components in the dialog
		TArray<UPrimitiveComponent*> ComponentsToMerge;

		for (const TSharedPtr<HackMeshMerging::FMergeComponentData>& SelectedComponent : SelectedComponents)
		{
			// Determine whether or not this component should be incorporated according the user settings
			if (SelectedComponent->bShouldIncorporate && SelectedComponent->PrimComponent.IsValid())
			{
				ComponentsToMerge.Add(SelectedComponent->PrimComponent.Get());
			}
		}

		if (ComponentsToMerge.Num())
		{
			UWorld* World = ComponentsToMerge[0]->GetWorld();
			checkf(World != nullptr, TEXT("Invalid World retrieved from Mesh components"));
			const float ScreenAreaSize = TNumericLimits<float>::Max();

			// If the merge destination package already exists, it is possible that the mesh is already used in a scene somewhere, or its materials or even just its textures.
			// Static primitives uniform buffers could become invalid after the operation completes and lead to memory corruption. To avoid it, we force a global reregister.
			if (FindObject<UObject>(nullptr, *PackageName))
			{
				FGlobalComponentReregisterContext GlobalReregister;
				MeshUtilities.MergeComponentsToStaticMesh(ComponentsToMerge, World, MergeSettings, nullptr, nullptr, PackageName, AssetsToSync, MergedActorLocation, ScreenAreaSize, true);
			}
			else
			{
				MeshUtilities.MergeComponentsToStaticMesh(ComponentsToMerge, World, MergeSettings, nullptr, nullptr, PackageName, AssetsToSync, MergedActorLocation, ScreenAreaSize, true);
			}
		}
	}

	if (AssetsToSync.Num())
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		int32 AssetCount = AssetsToSync.Num();
		for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
		{
			AssetRegistry.AssetCreated(AssetsToSync[AssetIndex]);
			GEditor->BroadcastObjectReimported(AssetsToSync[AssetIndex]);
		}

		//Also notify the content browser that the new assets exists
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);

		// Place new mesh in the world
		if (bReplaceSourceActors)
		{
			UStaticMesh* MergedMesh = nullptr;
			if (AssetsToSync.FindItemByClass(&MergedMesh))
			{
				const FScopedTransaction Transaction(LOCTEXT("PlaceMergedActor", "Place Merged Actor"));
				UniqueLevels[0]->Modify();

				UWorld* World = UniqueLevels[0]->OwningWorld;
				FActorSpawnParameters Params;
				Params.OverrideLevel = UniqueLevels[0];
				FRotator MergedActorRotation(ForceInit);

				AStaticMeshActor* MergedActor = World->SpawnActor<AStaticMeshActor>(MergedActorLocation, MergedActorRotation, Params);
				MergedActor->GetStaticMeshComponent()->SetStaticMesh(MergedMesh);
				MergedActor->SetActorLabel(MergedMesh->GetName());
				World->UpdateCullDistanceVolumes(MergedActor, MergedActor->GetStaticMeshComponent());
				GEditor->SelectNone(true, true);
				GEditor->SelectActor(MergedActor, true, true);
				// Remove source actors
				for (AActor* Actor : Actors)
				{
					Actor->Destroy();
				}
			}
		}
	}

	return true;
}




bool UCitySampleMeshMergingBlueprintLibrary::ApproximateActors(const FString PackageName, const TArray<AActor*> ActorsToMerge, const FMeshApproximationSettings& UseSettings, const bool bReplaceSourceActors /*= false*/)
{
	bool bAllowShapeComponents = false;		// not currently supported with this method

	// just calling this to detect error case, 
	const IMeshMergeUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	TArray<TSharedPtr<HackMeshMerging::FMergeComponentData>> SelectedComponents;
	BuildMergeDataFromActorsArray(ActorsToMerge, SelectedComponents, Actors, bReplaceSourceActors ? &UniqueLevels : nullptr, bAllowShapeComponents);
	// This restriction is only for replacement of selected actors with merged mesh actor
	if (UniqueLevels.Num() > 1 && bReplaceSourceActors)
	{
		UE_LOG(LogCitySampleMeshMerging, Error, TEXT("Unable to Approximate Actors: The selected actors should be in the same level to replace"));
		return false;
	}

	FString UniquePackageName;
	FString UniqueAssetName;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), UniquePackageName, UniqueAssetName);
	FString UsePackageName = UniquePackageName;
	if (FindObject<UObject>(nullptr, *UsePackageName) != nullptr)
	{
		UE_LOG(LogCitySampleMeshMerging, Error, TEXT("Unable to Approximate Actors: destination package already exists and unique name generation failed"));
		return false;
	}


	IGeometryProcessingInterfacesModule& GeomProcInterfaces = FModuleManager::Get().LoadModuleChecked<IGeometryProcessingInterfacesModule>("GeometryProcessingInterfaces");
	IGeometryProcessing_ApproximateActors* ApproxActorsAPI = GeomProcInterfaces.GetApproximateActorsImplementation();

	//
	// Construct options for ApproximateActors operation
	//

	IGeometryProcessing_ApproximateActors::FOptions Options = ApproxActorsAPI->ConstructOptions(UseSettings);
	Options.BasePackagePath = UsePackageName;

	// run actor approximation computation
	IGeometryProcessing_ApproximateActors::FResults Results;
	ApproxActorsAPI->ApproximateActors(Actors, Options, Results);

	// TODO: is this always the case?
	FVector MergedActorLocation = FVector::ZeroVector;

	// Notify Asset Browser about new Assets
	TArray<UObject*> AssetsToSync;
	for (UStaticMesh* StaticMesh : Results.NewMeshAssets)
	{
		AssetsToSync.Add(StaticMesh);
	}
	for (UMaterialInterface* Material : Results.NewMaterials)
	{
		AssetsToSync.Add(Material);
	}
	for (UTexture2D* Texture : Results.NewTextures)
	{
		AssetsToSync.Add(Texture);
	}


	if (AssetsToSync.Num())
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		int32 AssetCount = AssetsToSync.Num();
		for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
		{
			AssetRegistry.AssetCreated(AssetsToSync[AssetIndex]);
			GEditor->BroadcastObjectReimported(AssetsToSync[AssetIndex]);
		}

		//Also notify the content browser that the new assets exists
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);

		// Place new mesh in the world
		if (bReplaceSourceActors)
		{
			UStaticMesh* MergedMesh = nullptr;
			if (AssetsToSync.FindItemByClass(&MergedMesh))
			{
				const FScopedTransaction Transaction(LOCTEXT("PlaceMergedActor", "Place Merged Actor"));
				UniqueLevels[0]->Modify();

				UWorld* World = UniqueLevels[0]->OwningWorld;
				FActorSpawnParameters Params;
				Params.OverrideLevel = UniqueLevels[0];
				FRotator MergedActorRotation(ForceInit);

				AStaticMeshActor* MergedActor = World->SpawnActor<AStaticMeshActor>(MergedActorLocation, MergedActorRotation, Params);
				MergedActor->GetStaticMeshComponent()->SetStaticMesh(MergedMesh);
				MergedActor->SetActorLabel(MergedMesh->GetName());
				World->UpdateCullDistanceVolumes(MergedActor, MergedActor->GetStaticMeshComponent());
				GEditor->SelectNone(true, true);
				GEditor->SelectActor(MergedActor, true, true);
				// Remove source actors
				for (AActor* Actor : Actors)
				{
					Actor->Destroy();
				}
			}
		}
	}

	return true;
}



#undef LOCTEXT_NAMESPACE