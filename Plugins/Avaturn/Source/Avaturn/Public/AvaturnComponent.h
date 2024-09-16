// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AvaturnTypes.h"
#include "glTFRuntimeParser.h"
#include "AvaturnComponent.generated.h"

class UglTFRuntimeAsset;

/**
 * Responsible for the loading of the avatar and visualizing it by setting the SkeletalMesh of the avatar.
 * It also provides useful functions for loading the avatar and loading a rendered image of the avatar.
 *
 * @see AvaturnAvatarLoader
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AVATURN_API UAvaturnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	UAvaturnComponent();

	/** 
	 * Downloads the avatar from the web and generates a skeletal mesh of the avatar.
	 * If the model was previously downloaded and stored locally, the local model will be used for the loading of the avatar. 
	 * 
	 * @param OnLoadCompleted Success callback. Called when the avatar asset is downloaded and the skeletal mesh is set.
	 * @param OnLoadFailed Failure callback. If the avatar fails to load, the failure callback will be called.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load Avatar", AutoCreateRefTerm = "OnLoadCompleted,OnLoadFailed"))
	void LoadAvatar(const FAvatarLoadCompleted& OnLoadCompleted, const FAvatarLoadFailed& OnLoadFailed);

	/**
	 * Downloads the avatar from the web using the provided url and generates a skeletal mesh of the avatar.
	 * If the model was previously downloaded and stored locally, the local model will be used for the loading of the avatar. 
	 * 
	 * @param Url Avatar url or shortcode.
	 * @param OnLoadCompleted Success callback. Called when the avatar asset is downloaded and the skeletal mesh is set.
	 * @param OnLoadFailed Failure callback. If the avatar fails to load, the failure callback will be called.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load New Avatar", AutoCreateRefTerm = "OnLoadCompleted,OnLoadFailed"))
	void LoadNewAvatar(const FExportAvatarResult& ExportResult, const FAvatarLoadCompleted& OnLoadCompleted, const FAvatarLoadFailed& OnLoadFailed);

	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load Avatar From File", AutoCreateRefTerm = "OnLoadCompleted,OnLoadFailed"))
	void LoadAvatarFromFile(const FString& Path, const FAvatarLoadCompleted& OnLoadCompleted, const FAvatarLoadFailed& OnLoadFailed);

	UPROPERTY(BlueprintReadWrite, Category = "Avaturn")
	FExportAvatarResult LastExportResult;

	/** It provides a flexibility to chose the skeleton that will be used for the loaded avatar.
	 * If it's not set the default skeleton will be used for the loaded avatar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Avaturn")
	FTargetSkeleton TargetSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	FString Root_Bone = "Armature";

	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	USkeleton* GetTargetSkeleton();

	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	UClass* GetTargetAnimClass();

	/**
	 * Skeletal mesh component used for setting the skeletal mesh of the loaded avatar.
	 * If not set, It will be initialised with the skeletal mesh component of the parent actor.
	 * For runtime animation retargeting, if the actor has multiple skeletal mesh components, this property needs to be set.
	 */
	UPROPERTY(BlueprintReadWrite, Category="Avaturn")
	USkeletalMeshComponent* SkeletalMeshComponent;

	/**
	 * glTFRuntime skeletal mesh config that will be used for loading the avatar.
	 * This property should be changed only for very custom cases.
	 * @note Changing this property might break the avatar.
	 */
	UPROPERTY(BlueprintReadWrite, Category="Avaturn")
	FglTFRuntimeSkeletalMeshConfig SkeletalMeshConfig;

	/**
	 * Allow to use the preloaded avatars. If set to true, the MemoryCache will be used when loading the avatar.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	bool bUseMemoryCache;

	/**
	 * Immediately cancels the avatar loading.
	 * When the garbage collector is removing the AvatarLoader, avatar loading gets automatically cancelled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Cancel Avatar"))
	void CancelAvatarLoad();

	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class UAvaturnAvatarLoader* AvatarLoader;

	UFUNCTION()
	void OnAvatarDownloaded(USkeletalMesh* Mesh);

	UFUNCTION()
	void OnSkeletalMeshLoaded(USkeletalMesh* Mesh);

	void InitSkeletalMeshComponent();

	void LoadSkeletalMesh(UglTFRuntimeAsset* Asset);

	void SetAvatarData(USkeletalMesh* SkeletalMesh);

	FAvatarDownloadCompleted OnAvatarDownloadCompleted;

	FAvatarLoadCompleted OnAvatarLoadCompleted;

	FglTFRuntimeSkeletalMeshAsync OnSkeletalMeshCallback;

	bool bLoadNewAvatar = false;
};
