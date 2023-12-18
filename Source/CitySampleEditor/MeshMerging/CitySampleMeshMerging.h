// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/MeshMerging.h"
#include "SkeletalMeshMerge.h"

#include "CitySampleMeshMerging.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCitySampleMeshMerging, Log, All);

// Hack class that mimics the behavior of the Merge Actors editor tool for use with Python
// Hopefully this can be removed when the actual merging functions are exposed directly
UCLASS()
class CITYSAMPLEEDITOR_API UCitySampleMeshMergingBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "CitySample | MeshMerging")
	static USkeletalMesh* MergeSkeletalMeshes(TArray<USkeletalMesh*> SkeletalMeshes, const FString PackageName, USkeleton* Skeleton = nullptr);
	
	/**
	 * Merge to a single static mesh actor
	 *
	 * @param	PackageName				The path to put the new merged actor in
	 * @param	ActorsToMerge			An array of actors to merge
	 * @param	MergeSettings			A structure containing all the settings required for the merge. The options are the same as what you'd see in the Merge Actors window
	 * @param	bReplaceSourceActors	(Optional) Whether the given actors should be replaced by the merged actor. Defaults to False
	 * @param	bAllowShapeComponents	(Optional) Whether to include Shape Components when merging. Defaults to True
	 *
	 * @returns	True, if the Merge was able to be completed. Otherwise, false.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample | MeshMerging")
	static bool MergeActors(const FString PackageName, const TArray<AActor*> ActorsToMerge, const FMeshMergingSettings& MergeSettings, const bool bReplaceSourceActors = false, const bool bAllowShapeComponents = true);


	/**
	 * Approximate actors with a single static mesh actor
	 *
	 * @param	PackageName				The path to put the new merged actor in
	 * @param	ActorsToMerge			An array of actors to merge
	 * @param	MergeSettings			A structure containing all the settings required for the merge. The options are the same as what you'd see in the Merge Actors window
	 * @param	bReplaceSourceActors	(Optional) Whether the given actors should be replaced by the merged actor. Defaults to False
	 *
	 * @returns	True, if the Merge was able to be completed. Otherwise, false.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample | MeshMerging")
	static bool ApproximateActors(const FString PackageName, const TArray<AActor*> ActorsToMerge, const FMeshApproximationSettings& ApproxSettings, const bool bReplaceSourceActors = false);


};