// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"
#include "glTFRuntimeParser.h"
#include "AvaturnMemoryCache.generated.h"

USTRUCT(BlueprintType)
struct FAvatarPreloadData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	FExportAvatarResult ExportResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	USkeleton* TargetSkeleton = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	FglTFRuntimeSkeletalMeshConfig SkeletalMeshConfig;
};

USTRUCT(BlueprintType)
struct FAvatarMemoryCacheData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	FExportAvatarResult ExportResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn")
	USkeletalMesh* Mesh = nullptr;
};

/**
 * Is used to preload avatars and store the cached skeletal meshes.
 * Cached avatars will be instantiated instantly. 
 */
UCLASS(Blueprintable, BlueprintType)
class AVATURN_API UAvaturnMemoryCache : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Preloads the avatar data so the avatars would be instantiated instantly when loading them from the AvaturnComponent.
	 * 
	 * @param PreloadDataList List of avatar data needed for preloading.
	 * @param OnPreloadCompleted Complete callback. Called when the avatars are preloaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Preload", AutoCreateRefTerm = "OnPreloadCompleted"))
	void Preload(const TArray<FAvatarPreloadData>& PreloadDataList, const FAvatarPreloadCompleted& OnPreloadCompleted);

	/**
	 * Returns the preload avatar data for a specific avatar.
	 *
	 * @param Url Avatar url.
	 * @param AvatarConfig Avatar config.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	FAvatarMemoryCacheData GetAvatarCacheData(const FExportAvatarResult& ExportResult) const;

	/**
	 * Adds an already loaded avatar to the memory cache.
	 *
	 * @param AvatarId Avatar Id.
	 * @param SkeletalMesh Preloaded skeletalMesh of the avatar.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	void AddAvatar(const FExportAvatarResult& ExportResult, USkeletalMesh* Mesh);

	/**
	 * Removes specific avatar data from the memory cache.
	 *
	 * @param Avatar Id.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	void RemoveAvatar(const FExportAvatarResult& ExportResult);

	/** Clears all avatars from the memory cache. */
	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	void ClearAvatars();

	/** Avatar Data for all the preloaded avatars. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Avaturn")
	TArray<FAvatarMemoryCacheData> CachedAvatars;

	UPROPERTY(BlueprintReadOnly, Category = "Avaturn")
	FExportAvatarResult LastExportResult;

private:
	UFUNCTION()
	void OnAvatarDownloaded(USkeletalMesh* Mesh);

	UFUNCTION()
	void OnAvatarLoadFailed(const FString& ErrorMessage);

	void CompleteLoading();

	UPROPERTY()
	TMap<class UAvaturnAvatarLoader*, FAvatarPreloadData> AvatarLoaders;

	FAvatarDownloadCompleted OnAvatarDownloadCompleted;
	FAvatarLoadFailed OnLoadFailed;
	FAvatarPreloadCompleted OnAvatarPreloadCompleted;

	int32 FailedRequestCount = 0;
};
