// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AvaturnTypes.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Interfaces/IHttpRequest.h"
#include "AvaturnAvatarLoader.generated.h"

/**
 * Responsible for Loading the avatar from the url and storing it in the local storage.
 * AvaturnAvatarLoader is used by AvaturnComponent for loading the avatar.
 */
UCLASS(BlueprintType)
class AVATURN_API UAvaturnAvatarLoader : public UObject
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	UAvaturnAvatarLoader();

	/**
	 * Downloads the avatar asset from the Url and saves it in the local storage.
	 *
	 * @param Url Avatar url.
	 * @param OnDownloadCompleted Success callback. Called when the avatar asset is downloaded.
	 * @param OnLoadFailed Failure callback. If the avatar fails to load, the failure callback will be called.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load Avatar", AutoCreateRefTerm = "OnLoadFailed"))
	void LoadAvatar(const bool bLDataUrl, USkeleton* TargetSkeleton, const FAvatarDownloadCompleted& OnDownloadCompleted, const FAvatarLoadFailed& OnLoadFailed);

	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Load Avatar From File", AutoCreateRefTerm = "OnLoadFailed"))
	void LoadAvatarFromFile(const FString& Path, USkeleton* TargetSkeleton, const FAvatarDownloadCompleted& OnDownloadCompleted, const FAvatarLoadFailed& OnLoadFailed);

	/**
	 * Cancels the avatar downloading process. This function is called during garbage collection, but it can also be called manually.
	 */
	UFUNCTION(BlueprintCallable, Category = "Avaturn", meta = (DisplayName = "Cancel Avatar Load"))
	void CancelAvatarLoad();

	UPROPERTY(BlueprintReadWrite, Category = "Avaturn")
	FString ModelUrl;

	UPROPERTY(BlueprintReadWrite, Category = "Avaturn")
	FString AvatarId;

	UPROPERTY(BlueprintReadWrite, Category = "Avaturn")
	TArray<uint8> Content;

	UPROPERTY()
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(BlueprintReadWrite, Category = "Avaturn")
	FString Root_Bone = "root";

private:
	void OnAvatarModelReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	
	void DownloadAvatarModel();

	void ExecuteSuccessCallback();

	void ExecuteFailureCallback(const FString& ErrorMessage);

	UFUNCTION()
	void OnGlbLoaded(USkeletalMesh* Mesh);
	
	void Reset();

	virtual void BeginDestroy() override;

	FAvatarDownloadCompleted OnAvatarDownloadCompleted;
	FAvatarLoadFailed OnAvatarLoadFailed;
	FGlbLoadCompleted OnGlbLoadCompleted;

	bool bDataURL;

	UPROPERTY()
	class UAvaturnGlbLoader* GlbLoader;

	TOptional<FAvatarUri> AvatarUri;
	TSharedPtr<class FAvaturnAvatarCacheHandler> CacheHandler;

	FDateTime AvatarDownloadTime;

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> AvatarModelRequest;
#else
	TSharedPtr<IHttpRequest> AvatarModelRequest;
#endif
};
