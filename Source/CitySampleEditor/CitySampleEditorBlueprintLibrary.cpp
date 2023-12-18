// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleEditorBlueprintLibrary.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "MovieSceneTrack.h"
#include "LevelSequence/Public/LevelSequence.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_FunctionEntry.h"

// Fracture includes
#include "FractureEditorModeToolkit.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionConversion.h"
#include "GeometryCollection/GeometryCollectionClusteringUtility.h"
#include "GeometryCollection/GeometryCollectionObject.h"

// System includes for fracture helper funcs
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "ScopedTransaction.h"
#include "Layers/LayersSubsystem.h"

#include "StaticMeshEditor/Private/StaticMeshEditorTools.h"
#include "EditorReimportHandler.h"

void FCitySampleEditorBlueprintLibraryHelper::AddSingleRootNodeIfRequired(UGeometryCollection* GeometryCollectionObject)
{
	TSharedPtr<FGeometryCollection, ESPMode::ThreadSafe> GeometryCollectionPtr = GeometryCollectionObject->GetGeometryCollection();
	if (FGeometryCollection* GeometryCollection = GeometryCollectionPtr.Get())
	{
		if (FGeometryCollectionClusteringUtility::ContainsMultipleRootBones(GeometryCollection))
		{
			FGeometryCollectionClusteringUtility::ClusterAllBonesUnderNewRoot(GeometryCollection);
		}
	}
}

AActor* FCitySampleEditorBlueprintLibraryHelper::AddActor(ULevel* InLevel, UClass* Class)
{
	check(Class);

	UWorld* World = InLevel->OwningWorld;
	ULevel* DesiredLevel = InLevel;

	// Transactionally add the actor.
	AActor* Actor = NULL;
	{
		FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "AddActor", "Add Actor"));

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.OverrideLevel = DesiredLevel;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.ObjectFlags = RF_Transactional;
		const auto Location = FVector(0);
		const auto Rotation = FTransform(FVector(0)).GetRotation().Rotator();
		Actor = World->SpawnActor(Class, &Location, &Rotation, SpawnInfo);

		check(Actor);
		Actor->InvalidateLightingCache();
		Actor->PostEditMove(true);
	}

	// If this actor is part of any layers (set in its default properties), add them into the visible layers list.
	ULayersSubsystem* Layers = GEditor->GetEditorSubsystem<ULayersSubsystem>();
	Layers->SetLayersVisibility(Actor->Layers, true);

	// Clean up.
	Actor->MarkPackageDirty();
	ULevel::LevelDirtiedEvent.Broadcast();

	return Actor;
}

ULevel* FCitySampleEditorBlueprintLibraryHelper::GetSelectedLevel()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	TArray<ULevel*> UniqueLevels;
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			UniqueLevels.AddUnique(Actor->GetLevel());
		}
	}
	check(UniqueLevels.Num() == 1);
	return UniqueLevels[0];
}

class AGeometryCollectionActor* FCitySampleEditorBlueprintLibraryHelper::CreateNewGeometryActor(const FString& InAssetPath, const FTransform& Transform, bool AddMaterials /*= false*/)
{

	FString UniquePackageName = InAssetPath;
	FString UniqueAssetName = FPackageName::GetLongPackageAssetName(InAssetPath);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(UniquePackageName, TEXT(""), UniquePackageName, UniqueAssetName);

	UPackage* Package = CreatePackage(*UniquePackageName);
	UGeometryCollection* InGeometryCollection = static_cast<UGeometryCollection*>(NewObject<UGeometryCollection>(Package, UGeometryCollection::StaticClass(), FName(*UniqueAssetName), RF_Transactional | RF_Public | RF_Standalone));

	// Create the new Geometry Collection actor
	AGeometryCollectionActor* NewActor = Cast<AGeometryCollectionActor>(AddActor(GetSelectedLevel(), AGeometryCollectionActor::StaticClass()));
	check(NewActor->GetGeometryCollectionComponent());

	// Set the Geometry Collection asset in the new actor
	NewActor->GetGeometryCollectionComponent()->SetRestCollection(InGeometryCollection);

	// copy transform of original static mesh actor to this new actor
	NewActor->SetActorLabel(UniqueAssetName);
	NewActor->SetActorTransform(Transform);

	// Mark relevant stuff dirty
	FAssetRegistryModule::AssetCreated(InGeometryCollection);
	InGeometryCollection->MarkPackageDirty();
	Package->SetDirtyFlag(true);

	return NewActor;
}

AGeometryCollectionActor* FCitySampleEditorBlueprintLibraryHelper::ConvertStaticMeshToGeometryCollection(const FString& InAssetPath, TArray<AActor*>& Actors)
{
	ensure(Actors.Num() > 0);
	AActor* FirstActor = Actors[0];
	const FString& Name = FirstActor->GetActorLabel();
	const FVector FirstActorLocation(FirstActor->GetActorLocation());


	AGeometryCollectionActor* NewActor = CreateNewGeometryActor(InAssetPath, FTransform(), true);

	FGeometryCollectionEdit GeometryCollectionEdit = NewActor->GetGeometryCollectionComponent()->EditRestCollection(GeometryCollection::EEditUpdate::RestPhysicsDynamic);
	UGeometryCollection* FracturedGeometryCollection = GeometryCollectionEdit.GetRestCollection();

	for (AActor* Actor : Actors)
	{
		const FTransform ActorTransform(Actor->GetTransform());
		const FVector ActorOffset(Actor->GetActorLocation() - FirstActor->GetActorLocation());

		check(FracturedGeometryCollection);

		TArray<UStaticMeshComponent*> StaticMeshComponents;
		Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents, true);
		for (int32 ii = 0, ni = StaticMeshComponents.Num(); ii < ni; ++ii)
		{
			// We're partial to static mesh components, here
			UStaticMeshComponent* StaticMeshComponent = StaticMeshComponents[ii];
			if (StaticMeshComponent != nullptr)
			{
				UStaticMesh* ComponentStaticMesh = StaticMeshComponent->GetStaticMesh();
				FTransform ComponentTranform(StaticMeshComponent->GetComponentTransform());
				ComponentTranform.SetTranslation((ComponentTranform.GetTranslation() - ActorTransform.GetTranslation()) + ActorOffset);
				FGeometryCollectionConversion::AppendStaticMesh(ComponentStaticMesh, StaticMeshComponent, ComponentTranform, FracturedGeometryCollection, true);
			}
		}

		FracturedGeometryCollection->InitializeMaterials();
	}
	AddSingleRootNodeIfRequired(FracturedGeometryCollection);

	return NewActor;
}

void UCitySampleEditorBlueprintLibrary::SetNaniteSettings(UStaticMesh* StaticMesh, bool bEnabled, float PercentTriangles)
{
	StaticMesh->Modify();
	StaticMesh->NaniteSettings.bEnabled = bEnabled;
	StaticMesh->NaniteSettings.FallbackPercentTriangles = PercentTriangles;
	StaticMesh->PostEditChange();
}

void UCitySampleEditorBlueprintLibrary::GenerateGeometryCollection(const FString& AssetPath, TArray<AActor*> Actors)
{
	// This method is a hacked reimplementation FractureEditorModeToolkit::OnGenerateAssetPathChose that strips out most UI-coupled logic.
	FCitySampleEditorBlueprintLibraryHelper Helper = FCitySampleEditorBlueprintLibraryHelper();
	UGeometryCollectionComponent* GeometryCollectionComponent = nullptr;//  = Cast<UGeometryCollectionComponent>(FractureContext.OriginalPrimitiveComponent);

	if (Actors.Num() > 0)
	{
		AActor* FirstActor = Actors[0];

		AGeometryCollectionActor* GeometryCollectionActor = Helper.ConvertStaticMeshToGeometryCollection(AssetPath, Actors);

		GeometryCollectionComponent = GeometryCollectionActor->GetGeometryCollectionComponent();

		// Move GC actor to source actors position and remove source actor from scene
		const FVector ActorLocation(FirstActor->GetActorLocation());
		GeometryCollectionActor->SetActorLocation(ActorLocation);

		// Clear selection of mesh actor used to make GC before selecting, will cause details pane to not display geometry collection details.
		GEditor->SelectNone(true, true, false);
		GEditor->SelectActor(GeometryCollectionActor, true, true);

		GeometryCollectionComponent->MarkRenderDynamicDataDirty();
		GeometryCollectionComponent->MarkRenderStateDirty();

		for (AActor* Actor : Actors)
		{
			Actor->Destroy();
		}
	}
}

void UCitySampleEditorBlueprintLibrary::SetCollisionComplexity(UStaticMesh* StaticMesh, const TEnumAsByte<ECollisionTraceFlag> Complexity)
{
	if (!StaticMesh)
	{
		return;
	}

	StaticMesh->Modify();
	if (UBodySetup * BodySetup = StaticMesh->GetBodySetup())
	{
		BodySetup->CollisionTraceFlag = Complexity;
	}
	StaticMesh->PostEditChange();
}

bool UCitySampleEditorBlueprintLibrary::GetCollisionComplexity(UStaticMesh* StaticMesh, TEnumAsByte<ECollisionTraceFlag>& Complexity)
{
	if (!StaticMesh)
	{
		return false;
	}

	if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
	{
		Complexity = BodySetup->CollisionTraceFlag;
		return true;
	}
	return false;
}

void UCitySampleEditorBlueprintLibrary::SetNeverNeedsCookedCollisionData(UStaticMesh* StaticMesh, bool bNeverNeedsCookedCollisionData)
{
	UBodySetup* BodySetup = StaticMesh ? StaticMesh->GetBodySetup() : nullptr;
	if (BodySetup && (BodySetup->bNeverNeedsCookedCollisionData != bNeverNeedsCookedCollisionData))
	{
		BodySetup->Modify();
		BodySetup->bNeverNeedsCookedCollisionData = bNeverNeedsCookedCollisionData;
		StaticMesh->PostEditChange();
	}
}

bool UCitySampleEditorBlueprintLibrary::GetNeverNeedsCookedCollisionData(UStaticMesh* StaticMesh)
{
	UBodySetup* BodySetup = StaticMesh ? StaticMesh->GetBodySetup() : nullptr;
	return BodySetup ? BodySetup->bNeverNeedsCookedCollisionData : false;
}

void UCitySampleEditorBlueprintLibrary::CopyPhysicsAssetSettings(UPhysicsAsset* SourcePhysicsAsset, TArray<UPhysicsAsset*> TargetPhysicsAssets)
{
	if (SourcePhysicsAsset)
	{
		for (UPhysicsAsset* TargetPhysicsAsset : TargetPhysicsAssets)
		{
			if (TargetPhysicsAsset)
			{
				// Copy BodySetup settings
				for (USkeletalBodySetup* TargetSkeletalBodySetup : TargetPhysicsAsset->SkeletalBodySetups)
				{
					if (SourcePhysicsAsset->BodySetupIndexMap.Contains(TargetSkeletalBodySetup->BoneName))
					{
						TargetPhysicsAsset->Modify();
						TargetSkeletalBodySetup->CopyBodySetupProperty(SourcePhysicsAsset->SkeletalBodySetups[*SourcePhysicsAsset->BodySetupIndexMap.Find(TargetSkeletalBodySetup->BoneName)]);
					}
				}

				// Copy Constraint settings
				for (UPhysicsConstraintTemplate* TargetConstraintTemplate : TargetPhysicsAsset->ConstraintSetup)
				{
					// check if constraint name matches
					for (UPhysicsConstraintTemplate* SourceConstraintTemplate : SourcePhysicsAsset->ConstraintSetup)
					{
						if (SourceConstraintTemplate->DefaultInstance.JointName == TargetConstraintTemplate->DefaultInstance.JointName)
						{
							TargetPhysicsAsset->Modify();
							TargetConstraintTemplate->DefaultInstance.CopyProfilePropertiesFrom(SourceConstraintTemplate->DefaultInstance.ProfileInstance);
						}
					}
				}
			}
		}
	}
}

void UCitySampleEditorBlueprintLibrary::CopyConstraintSettingsFromJointNames(UPhysicsAsset* SourcePhysicsAsset, const FName& SourceJointName, UPhysicsAsset* TargetPhysicsAsset, const FName& TargetJointName, bool bKeepPosition, bool bKeepRotation)
{
	if (!SourcePhysicsAsset || !TargetPhysicsAsset)
	{
		return;
	}

	UPhysicsConstraintTemplate* FromConstraintSetup = nullptr;

	for (UPhysicsConstraintTemplate* SourceConstraintTemplate : SourcePhysicsAsset->ConstraintSetup)
	{
		if (SourceConstraintTemplate && SourceConstraintTemplate->DefaultInstance.JointName == SourceJointName)
		{
			FromConstraintSetup = SourceConstraintTemplate;
			break;
		}
	}

	UPhysicsConstraintTemplate* ToConstraintSetup = nullptr;

	for (UPhysicsConstraintTemplate* TargetConstraintTemplate : TargetPhysicsAsset->ConstraintSetup)
	{
		if (TargetConstraintTemplate && TargetConstraintTemplate->DefaultInstance.JointName == TargetJointName)
		{
			ToConstraintSetup = TargetConstraintTemplate;
			break;
		}
	}

	if (FromConstraintSetup && ToConstraintSetup)
	{
		CopyConstraintSettings(FromConstraintSetup, ToConstraintSetup, bKeepPosition, bKeepRotation);
	}	
}

void UCitySampleEditorBlueprintLibrary::CopyConstraintSettingsFromBoneNames(UPhysicsAsset* SourcePhysicsAsset, const FName& SourceBone1Name, const FName& SourceBone2Name, UPhysicsAsset* TargetPhysicsAsset, const FName& TargetBone1Name, const FName& TargetBone2Name, bool bKeepPosition, bool bKeepRotation)
{
	if (!SourcePhysicsAsset || !TargetPhysicsAsset)
	{
		return;
	}

	UPhysicsConstraintTemplate* FromConstraintSetup = nullptr;

	for (UPhysicsConstraintTemplate* SourceConstraintTemplate : SourcePhysicsAsset->ConstraintSetup)
	{
		if (SourceConstraintTemplate && SourceConstraintTemplate->DefaultInstance.ConstraintBone1 == SourceBone1Name && SourceConstraintTemplate->DefaultInstance.ConstraintBone2 == SourceBone2Name)
		{
			FromConstraintSetup = SourceConstraintTemplate;
			break;
		}
	}

	UPhysicsConstraintTemplate* ToConstraintSetup = nullptr;

	for (UPhysicsConstraintTemplate* TargetConstraintTemplate : TargetPhysicsAsset->ConstraintSetup)
	{
		if (TargetConstraintTemplate && TargetConstraintTemplate->DefaultInstance.ConstraintBone1 == TargetBone1Name && TargetConstraintTemplate->DefaultInstance.ConstraintBone2 == TargetBone2Name)
		{
			ToConstraintSetup = TargetConstraintTemplate;
			break;
		}
	}

	if (FromConstraintSetup && ToConstraintSetup)
	{
		CopyConstraintSettings(FromConstraintSetup, ToConstraintSetup, bKeepPosition, bKeepRotation);
	}
}

void UCitySampleEditorBlueprintLibrary::CopyConstraintSettings(UPhysicsConstraintTemplate* FromConstraintSetup, UPhysicsConstraintTemplate* ToConstraintSetup, bool bKeepPosition, bool bKeepRotation)
{
	check(FromConstraintSetup && ToConstraintSetup);

	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "PasteConstraintProperties", "Copy Constraint Properties"));

	ToConstraintSetup->Modify();
	FConstraintInstance OldInstance = ToConstraintSetup->DefaultInstance;
	ToConstraintSetup->DefaultInstance.CopyConstraintPhysicalPropertiesFrom(&FromConstraintSetup->DefaultInstance, bKeepPosition, bKeepRotation);
	ToConstraintSetup->UpdateProfileInstance();
}

void UCitySampleEditorBlueprintLibrary::SetStatsShouldEmitNamedEvents(bool bShouldEmitNamedEvents)
{
	static bool bEmitNamedEventsHasBeenIncremented = false;

	if (bShouldEmitNamedEvents && !bEmitNamedEventsHasBeenIncremented)
	{
		GCycleStatsShouldEmitNamedEvents++;
		bEmitNamedEventsHasBeenIncremented = true;
	}
	else if (!bShouldEmitNamedEvents && bEmitNamedEventsHasBeenIncremented)
	{
		GCycleStatsShouldEmitNamedEvents--;
		bEmitNamedEventsHasBeenIncremented = false;
	}
}

void UCitySampleEditorBlueprintLibrary::ExploreProfilesDirectory()
{
	FPlatformProcess::ExploreFolder(*FPaths::ConvertRelativePathToFull(FPaths::ProfilingDir()));
}

void UCitySampleEditorBlueprintLibrary::DeleteMaterialSlot(UStaticMesh* Mesh, int32 MaterialIndex)
{
	if (!Mesh || !Mesh->GetStaticMaterials().IsValidIndex(MaterialIndex)) return;

	Mesh->Modify();
	Mesh->GetStaticMaterials().RemoveAt(MaterialIndex);

	//Fix the section info, the FMeshDescription use FName to retrieve the indexes when we build so no need to fix it
	for (int32 LodIndex = 0; LodIndex < Mesh->GetNumLODs(); ++LodIndex)
	{
		for (int32 SectionIndex = 0; SectionIndex < Mesh->GetNumSections(LodIndex); ++SectionIndex)
		{
			if (Mesh->GetSectionInfoMap().IsValidSection(LodIndex, SectionIndex))
			{
				FMeshSectionInfo SectionInfo = Mesh->GetSectionInfoMap().Get(LodIndex, SectionIndex);
				if (SectionInfo.MaterialIndex > MaterialIndex)
				{
					SectionInfo.MaterialIndex -= 1;
					Mesh->GetSectionInfoMap().Set(LodIndex, SectionIndex, SectionInfo);
				}
			}
		}
	}
	Mesh->PostEditChange();
}

void UCitySampleEditorBlueprintLibrary::UpdateReimportPath(UStaticMesh* Mesh, const FString& Filename, int32 SourceFileIndex)
{
	Mesh->Modify();
	FReimportManager::Instance()->UpdateReimportPath(Mesh, Filename, SourceFileIndex);
	Mesh->PostEditChange();
}

void UCitySampleEditorBlueprintLibrary::ReimportStaticMeshWithModify(UStaticMesh* Mesh)
{
	Mesh->Modify(); 
	FReimportManager::Instance()->Reimport(Mesh, false, false, "", nullptr, INDEX_NONE, false, false);
	Mesh->PostEditChange();
}

bool UCitySampleEditorBlueprintLibrary::GetGenerateLightmapUVs(UStaticMesh* StaticMesh)
{
	bool bGenerateLightmapUVs = false; 
	for (int32 LodIndex = 0; LodIndex < StaticMesh->GetNumSourceModels(); LodIndex++)
	{
		bGenerateLightmapUVs |= StaticMesh->GetSourceModel(LodIndex).BuildSettings.bGenerateLightmapUVs;
	}
	return bGenerateLightmapUVs;
}


void UCitySampleEditorBlueprintLibrary::ResetMaterialSectionSlots(UStaticMesh* Mesh)
{
	if (!Mesh) return;

	Mesh->Modify();

	for (int32 LodIndex = 0; LodIndex < Mesh->GetNumLODs(); ++LodIndex)
	{
		for (int32 SectionIndex = 0; SectionIndex < Mesh->GetNumSections(LodIndex); ++SectionIndex)
		{
			if (Mesh->GetSectionInfoMap().IsValidSection(LodIndex, SectionIndex))
			{
				FMeshSectionInfo SectionInfo = Mesh->GetSectionInfoMap().Get(LodIndex, SectionIndex); 
				SectionInfo.MaterialIndex = SectionIndex; 
				Mesh->GetSectionInfoMap().Set(LodIndex, SectionIndex, SectionInfo);
			}
		}
	}
	Mesh->PostEditChange();
}

bool UCitySampleEditorBlueprintLibrary::IsMaterialIndexUsed(UStaticMesh* Mesh, int32 MaterialIndex)
{
	if (!Mesh) return false;

	TMap<int32, TArray<FSectionLocalizer>> MaterialUsedMap;
	for (int32 Idx = 0; Idx < Mesh->GetStaticMaterials().Num(); ++Idx)
	{
		TArray<FSectionLocalizer> SectionLocalizers;
		for (int32 LODIndex = 0; LODIndex < Mesh->GetNumLODs(); ++LODIndex)
		{
			for (int32 SectionIndex = 0; SectionIndex < Mesh->GetNumSections(LODIndex); ++SectionIndex)
			{
				FMeshSectionInfo Info = Mesh->GetSectionInfoMap().Get(LODIndex, SectionIndex);

				if (Info.MaterialIndex == Idx)
				{
					SectionLocalizers.Add(FSectionLocalizer(LODIndex, SectionIndex));
				}
			}
		}
		MaterialUsedMap.Add(Idx, SectionLocalizers);
	}

	bool bMaterialIsUsed = false;
	if (MaterialUsedMap.Contains(MaterialIndex))
	{
		bMaterialIsUsed = MaterialUsedMap.Find(MaterialIndex)->Num() > 0;
	}

	//return Mesh->GetStaticMaterials().IsValidIndex(MaterialIndex);
	return bMaterialIsUsed;
}

void UCitySampleEditorBlueprintLibrary::CopyMeshMaterials(USkeletalMesh* SourceMesh, USkeletalMesh* TargetMesh, const TMap<FName, FName>& Mapping)
{
	if (!SourceMesh || !TargetMesh)
	{
		return;
	}

	// Modify Target.
	TargetMesh->Modify();

	bool IsDirty = false;
	for (FSkeletalMaterial& SourceMaterial : SourceMesh->GetMaterials())
	{
		for (FSkeletalMaterial& TargetMaterial : TargetMesh->GetMaterials())
		{
			// Check if Source MaterialSlot in Mapping
			if (Mapping.Contains(SourceMaterial.MaterialSlotName) && 
				TargetMaterial.MaterialSlotName == *Mapping.Find(SourceMaterial.MaterialSlotName))
			{
				TargetMaterial.MaterialInterface = SourceMaterial.MaterialInterface;
				// TargetMaterial.UVChannelData = SourceMaterial.UVChannelData;
				IsDirty = true;
				break;
			}

			// Name Matching
			else if(TargetMaterial.MaterialSlotName == SourceMaterial.MaterialSlotName)
			{
				TargetMaterial.MaterialInterface = SourceMaterial.MaterialInterface;
				// TargetMaterial.UVChannelData = SourceMaterial.UVChannelData;
				IsDirty = true;
				break;
			}
		}		
	}

	// Done with Editing.
	TargetMesh->PostEditChange();

	if (IsDirty)
	{
		TargetMesh->MarkPackageDirty();
	}
}

void UCitySampleEditorBlueprintLibrary::SetRayTracingGroupID(UStaticMeshComponent* StaticMeshComponent, int32 RayTracingGroupID)
{
	StaticMeshComponent->RayTracingGroupId = RayTracingGroupID;
}

bool UCitySampleEditorBlueprintLibrary::IsTrackMuted(UMovieSceneTrack* MovieSceneTrack)
{
	return MovieSceneTrack->IsEvalDisabled();
}

TArray<FString> UCitySampleEditorBlueprintLibrary::GetSequenceEvents(ULevelSequence* LevelSequence)
{
	TArray<FString> SequenceEvents; 
	if (LevelSequence)
	{
		if (UBlueprint* DirectorBlueprint = LevelSequence->GetDirectorBlueprint())
		{
			TArray<UEdGraph*> EventGraphs = DirectorBlueprint->EventGraphs;

			for (const UEdGraph* EventGraph : EventGraphs)
			{
				if (EventGraph)
				{
					SequenceEvents.Add(EventGraph->GetName());
					TArray<UK2Node_FunctionEntry*> CustomEvents;
					EventGraph->GetNodesOfClass(CustomEvents);

					for (const UK2Node_FunctionEntry* CustomEvent : CustomEvents)
					{
						TArray<UEdGraphPin*> Pins = CustomEvent->Pins;

						for (UEdGraphPin* Pin : Pins)
						{
							if (Pin->PinType.PinSubCategoryObject.IsValid())
							{
								const UObject* PinnedObject = Pin->PinType.PinSubCategoryObject.Get();
								SequenceEvents.Add(FString::Printf(TEXT("    %s (%s)"), *PinnedObject->GetName(), *Pin->PinName.ToString()));
							}
						}
					}
				}
			}
		}
	}
	return SequenceEvents;
}
