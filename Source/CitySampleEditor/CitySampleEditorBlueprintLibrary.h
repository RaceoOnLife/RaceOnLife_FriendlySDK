// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "CitySampleEditorBlueprintLibrary.generated.h"

class UPhysicsAsset;
class UStaticMeshComponent;
class UMovieSceneTrack;
class ULevelSequence;

class FCitySampleEditorBlueprintLibraryHelper
{
public:
	// These methods are ripped from FractureEditorModeToolkit, since the original was too UI-coupled
	void AddSingleRootNodeIfRequired(UGeometryCollection* GeometryCollectionObject);
	AActor* AddActor(ULevel* InLevel, UClass* Class);
	ULevel* GetSelectedLevel();
	class AGeometryCollectionActor* CreateNewGeometryActor(const FString& InAssetPath, const FTransform& Transform, bool AddMaterials /*= false*/);
	AGeometryCollectionActor* ConvertStaticMeshToGeometryCollection(const FString& InAssetPath, TArray<AActor*>& Actors);
};

/**
 * 
 */
UCLASS()
class CITYSAMPLEEDITOR_API UCitySampleEditorBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static void SetNaniteSettings(UStaticMesh* StaticMesh, bool bEnabled, float PercentTriangles);

	UFUNCTION(BlueprintCallable, Category = "CitySample | Fracture")
	static void GenerateGeometryCollection(const FString& AssetPath, TArray<AActor*> Actors);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta=(ScriptMethod))
	static void SetCollisionComplexity(UStaticMesh* StaticMesh,  const TEnumAsByte<ECollisionTraceFlag> Complexity);

	UFUNCTION(BlueprintPure, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static bool GetCollisionComplexity(UStaticMesh* StaticMesh, TEnumAsByte<ECollisionTraceFlag>& Complexity);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static void SetNeverNeedsCookedCollisionData(UStaticMesh* StaticMesh, bool bNeverNeedsCookedCollisionData);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static bool GetNeverNeedsCookedCollisionData(UStaticMesh* StaticMesh);

	UFUNCTION(BlueprintCallable, Category = "CitySample | PhysicsAsset", meta = (ScriptMethod))
	static void CopyPhysicsAssetSettings(UPhysicsAsset* SourcePhysicsAsset, TArray<UPhysicsAsset*> TargetPhysicsAssets);

	UFUNCTION(BlueprintCallable, Category = "CitySample | PhysicsAsset", meta = (ScriptMethod))
	static void CopyConstraintSettingsFromJointNames(UPhysicsAsset* SourcePhysicsAsset, const FName& SourceJointName, UPhysicsAsset* TargetPhysicsAsset, const FName& TargetJointName, bool bKeepPosition = true, bool bKeepRotation = true);

	UFUNCTION(BlueprintCallable, Category = "CitySample | PhysicsAsset", meta = (ScriptMethod))
	static void CopyConstraintSettingsFromBoneNames(UPhysicsAsset* SourcePhysicsAsset, const FName& SourceBone1Name, const FName& SourceBone2Name, UPhysicsAsset* TargetPhysicsAsset, const FName& TargetBone1Name, const FName& TargetBone2Name, bool bKeepPosition = true, bool bKeepRotation = true);

	UFUNCTION(BlueprintCallable, Category = "CitySample | Profiling")
	static void SetStatsShouldEmitNamedEvents(bool bShouldEmitNamedEvents);

	UFUNCTION(BlueprintCallable, Category = "CitySample | Profiling")
	static void ExploreProfilesDirectory();

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static void DeleteMaterialSlot(UStaticMesh* Mesh, int32 MaterialIndex);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static void UpdateReimportPath(UStaticMesh* StaticMesh, const FString& Filename, int32 SourceFileIndex);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static void ReimportStaticMeshWithModify(UStaticMesh* StaticMesh);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static bool GetGenerateLightmapUVs(UStaticMesh* StaticMesh);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static void ResetMaterialSectionSlots(UStaticMesh* Mesh);

	UFUNCTION(BlueprintCallable, Category = "CitySample | StaticMesh", meta = (ScriptMethod))
	static bool IsMaterialIndexUsed(UStaticMesh* Mesh, int32 MaterialIndex);

	UFUNCTION(BlueprintCallable, Category = "CitySample | SkeletalMesh", meta = (AutoCreateRefTerm = "Mapping"))
	static void CopyMeshMaterials(USkeletalMesh* SourceMesh, USkeletalMesh* TargetMesh, const TMap<FName, FName>& Mapping);

	UFUNCTION(BlueprintCallable, Category = "CitySample | Raytracing", meta = (ScriptMethod))
	static void SetRayTracingGroupID (UStaticMeshComponent* StaticMeshComponent, int32 RayTracingGroupID);

	UFUNCTION(BlueprintPure, Category = "CitySample | Sequencer", meta = (ScriptMethod))
	static bool IsTrackMuted(UMovieSceneTrack* MovieSceneTrack);

	UFUNCTION(BlueprintPure, Category = "CitySample | Sequencer", meta = (ScriptMethod))
	static TArray<FString> GetSequenceEvents(ULevelSequence* LevelSequence);

private:
	static void CopyConstraintSettings(UPhysicsConstraintTemplate* FromConstraintSetup, UPhysicsConstraintTemplate* ToConstraintSetup, bool bKeepPosition, bool bKeepRotation);
}; 

