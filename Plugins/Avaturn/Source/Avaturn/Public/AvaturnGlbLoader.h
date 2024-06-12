// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"
#include "glTFRuntimeParser.h"
#include "AvaturnGlbLoader.generated.h"

UCLASS(BlueprintType)
class AVATURN_API UAvaturnGlbLoader : public UObject
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	UAvaturnGlbLoader();

	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load Glb From File"))
	void LoadFromFile(const FString& LocalModelPath, const FGlbLoadCompleted& LoadCompleted);

	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load Glb From Data"))
	void LoadFromData(const TArray<uint8>& Data, const FGlbLoadCompleted& LoadCompleted);

	/** It provides a flexibility to chose the skeleton that will be used for the loaded avatar.
	 * If it's not set the default skeleton will be used for the loaded avatar. */
	UPROPERTY(BlueprintReadWrite, Category="Avaturn")
	USkeleton* TargetSkeleton;

	/**
	 * glTFRuntime skeletal mesh config that will be used for loading the avatar.
	 * This property should be changed only for very custom cases.
	 */
	UPROPERTY(BlueprintReadWrite, Category="Avaturn")
	FglTFRuntimeSkeletalMeshConfig SkeletalMeshConfig;

	UPROPERTY(BlueprintReadWrite, Category = "Avaturn")
	FString Root_Bone = "Armature";

private:
	UFUNCTION()
	void OnSkeletalMeshLoaded(USkeletalMesh* SkeletalMesh);

	void LoadSkeletalMesh(class UglTFRuntimeAsset* Asset);

	FglTFRuntimeSkeletalMeshAsync OnSkeletalMeshCallback;
	FGlbLoadCompleted OnLoadCompleted;
};
