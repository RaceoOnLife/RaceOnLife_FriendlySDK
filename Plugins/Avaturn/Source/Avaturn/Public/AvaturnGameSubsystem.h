// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AvaturnTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "glTFRuntimeFunctionLibrary.h"
#include "AvaturnGameSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE(FAvatarAssetsLoaded);
DECLARE_DYNAMIC_DELEGATE(FExportMeshLoaded);
DECLARE_DYNAMIC_DELEGATE(FCustomTokenInvalid);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSeparateAssetLoaded, class USkeletalMesh*, SkeletalMesh);

UCLASS()
class UAvaturnGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UAvaturnGameSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	///////////// Properties /////////////

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* BodyMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* HeadMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* LookMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* EyesMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* ShoesMeshComponent;
	//

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FString> AssetTypes = { "clothing", "hair_and_headwear", "eyewear", "footwear" };

	UPROPERTY(BlueprintReadOnly)
	TArray<FAvatarAsset> AvatarAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeleton* TargetSkeleton = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDataTable* TargetSkeletonDataTable = nullptr;

	// 1st element: gender, 2nd: no face/face anims, 3rd: body type
	UPROPERTY(BlueprintReadOnly)
	TArray<uint8> CurrentGenderTypeBody = { 0, 0, 0 };

	FAvatarAssetsLoaded AssetsLoadedDelegate;
	FCustomTokenInvalid CustomTokenInvalidDelegate;
	FSeparateAssetLoaded SeparateAssetLoadedDelegate;
	FExportMeshLoaded MergedMeshLoadedDelegate;

	// Need set before Avaturn Editor start - Firebase Auth
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CustomToken = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AvatarId = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Origin = "";

	UPROPERTY(BlueprintReadWrite)
	bool bAutoExport = false;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<class AAvaturnEditorActor> EditorActorClass;

	UPROPERTY(BlueprintReadWrite)
	AAvaturnEditorActor* EditorActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> HairColors = { "#6d4539", "#242322", "#a07c5a", "#a64725", "#e557d2", "#8c8282", "#57b2e6" };

	UPROPERTY(BlueprintReadWrite)
	FString DefaultHaircapId = "stripes";

	///////////// Functions /////////////

	void SpawnEditorActor();

	// Firebase Auth
	UFUNCTION(BlueprintCallable)
	void InitAvaturnSDK(const FString& SessionLink);
	//

	UFUNCTION(BlueprintCallable)
	void SendUpdatedAvatarToServer();

	UFUNCTION(BlueprintCallable)
	const FString GetHaircapLinkByType(const FString& HaircapType);

	UFUNCTION(BlueprintCallable)
	void DownloadAsset(const FString& AssetCategory, const FString& Url, const FString& Id, const bool bAutoSelect = true);

	UFUNCTION(BlueprintCallable)
	void AddSeparateAssetToQueue(const FString& Id, const FSeparateAssetLoaded& Response);

	UFUNCTION(BlueprintCallable)
	void DownloadSeparateAsset();

	UFUNCTION(BlueprintCallable)
	void RetargetAssets(const FString& LastCategory);

	UFUNCTION(BlueprintCallable)
	void EquipAvatarAsset(const FString& Id);

	UFUNCTION(BlueprintCallable)
	void EquipAvatarBody(const TArray<int32> NewGenderTypeBody);

	UFUNCTION(BlueprintCallable)
	void SelectHairColor(const FString& HairColor);

	UFUNCTION(BlueprintCallable)
	const FglTFRuntimeSkeletalMeshConfig& GetSkeletalMeshConfig();

	UFUNCTION(BlueprintCallable)
	void EquipAssetOnPlayer(const FString& Id, const FExportMeshLoaded& LoadedDelegate);

	UFUNCTION(BlueprintCallable)
	const FString GetAssetTypeById(const FString& AssetId);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FColor GetHairColorByIndex(const int32 ColorIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	uint8 GetCurrentBodyType();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	uint8 GetCurrentGenderType();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FString GetCurrentGenderString();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FString GetSelectedAssetIdByCategory(const FString& Category);

	// Ramp
	UFUNCTION(BlueprintCallable)
	void ApplyRampColorToHair(USkeletalMeshComponent* HeadMesh);

private:
	///////////// Properties /////////////

	// Firebase Auth
	const FString API_Key = "AIzaSyDy0k5N2IXOFlPd9JBVpGB-yB4ZLbVoQ7o";

	UPROPERTY()
	FString IdToken = "";

	UPROPERTY()
	FString RefreshToken = "";
	//

	// Loaded avatar from server
	TSharedPtr<FJsonObject> AvatarJson = nullptr;

	// for retarget
	UPROPERTY()
	TArray<FVector> AvatarVertexPositions;

	UPROPERTY()
	TArray<FVector> VertexOffsets;

	UPROPERTY()
	TArray<FVector4> Ids0Array;

	UPROPERTY()
	TArray<FVector4> Ids1Array;

	UPROPERTY()
	TArray<uint16> CorrespIds;

	UPROPERTY()
	FString CorrespIdsPath = FPaths::ProjectContentDir() + TEXT("Avaturn/Data/corresp_ids.dat");
	//

	// Source pixels from Base Color and Metallic Roughness Textures of Body
	UPROPERTY()
	TArray<uint8> BaseColorTexturePixels;

	UPROPERTY()
	TArray<uint8> RoughnessTexturePixels;
	//

	UPROPERTY()
	UTexture2D* HaircapTexture = nullptr;

	// for body alpha(hide mesh under cloth)
	UPROPERTY()
	TArray<UTexture2D*> AssetTextures;

	UPROPERTY()
	UTexture2D* AlphaTexture = nullptr;
	//

	UPROPERTY()
	FglTFRuntimeSkeletalMeshConfig SkeletalMeshConfig;

	UPROPERTY()
	UglTFRuntimeAsset* BodyAsset = nullptr;

	UPROPERTY()
	TArray<FAvatarAsset> AvatarSelectedAssets = { FAvatarAsset(), FAvatarAsset(), FAvatarAsset(), FAvatarAsset() };

	UPROPERTY()
	TArray<UglTFRuntimeAsset*> AvatarRuntimeAssets = { nullptr, nullptr, nullptr, nullptr };

	UPROPERTY()
	USkeletalMesh* BodyMesh = nullptr;

	UPROPERTY()
	TArray<USkeletalMesh*> AvatarMeshAssets = { nullptr, nullptr, nullptr, nullptr };

	UPROPERTY()
	TArray<FLinearColor> HairRampColor = { FLinearColor(0.095, 0.045, 0.091), FLinearColor(0.095, 0.445, 0.791), FLinearColor(0.125, 0.545, 0.851) };

	UPROPERTY()
	FString AvatarDataString;

	FDateTime ContentDownloadTime;

	bool bBodyInitialized = false;

	TArray<FString> AssetsToDownload;

	struct FSeparateAssetToDownload
	{
		FString Id;
		FString Url;
		FString Category;
		TSharedPtr<FSeparateAssetLoaded> Delegate;

		FSeparateAssetToDownload(const FString& NewId, const FString& NewUrl, const FString& NewCategory, const TSharedPtr<FSeparateAssetLoaded> NewDelegate)
		{
			Id = NewId;
			Url = NewUrl;
			Category = NewCategory;
			Delegate = NewDelegate;
		}

		FSeparateAssetToDownload()
		{
			Id = NULL;
			Url = NULL;
			Category = NULL;
			Delegate = nullptr;
		}
	};
	TArray<FSeparateAssetToDownload> SepAssetsToDownload;

	TMap<FString, FString> DefaultAssets = { {"clothing", ""}, {"hair_and_headwear", ""}, {"eyewear", ""}, {"footwear", ""} };

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> AuthRequest;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ContentRequest;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> AssetsRequest;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HaircapRequest;

	FString BodyMeshUrl = "";
	FString BodyDataUrl = "";
	FString BodyNormalsUrl = "";

	///////////// Functions /////////////

	bool ReadUInt16DataFromFile(const FString& FilePath, TArray<uint16_t>& OutData);

	// Firebase Auth
	UFUNCTION()
	void GetIdAndRefreshTokens();

	void OnTokensReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnAvatarInfoReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	//

	// Download all avaturn assets
	UFUNCTION()
	void DownloadContent();

	void OnContentDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void LoadContent(const TArray<TSharedPtr<FJsonValue>> ContentJson);

	void OnAssetDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void CacheDownloadedFile(const FString& Url, const FString& Id, const TArray<uint8>& Data, const FString& Extension = ".glb");
	const bool TryLoadFromCache(const FString& AssetCategory, const FString& Url, const FString& Id, const FString& Extension = ".glb");
	void LoadAsset(const FString AssetType, const TArray<uint8>& Data);
	void LoadHaircapTexture(const TArray<uint8>& Data);
	const bool DownloadFromAssetsToDownload();

	const bool IsCurrentAssetsLoaded();
	//

	const FString GetAvatarData();

	// for retarget
	UFUNCTION()
	void Retarget(UglTFRuntimeAsset* Asset, const int32 AssetIndex);

	UFUNCTION()
	void GetVertexExtraData(UglTFRuntimeAsset* Asset, const int32 PrimitiveIndex);

	UFUNCTION()
	void RetargetEyes(UglTFRuntimeAsset* Asset, const int32 AssetIndex);

	UFUNCTION()
	void PrepareTextureData();

	UFUNCTION()
	void UpdateAvatarMask();

	UFUNCTION()
	void DownloadHairCap(const FString& HaircapType);

	UFUNCTION()
	void EnableHairCap();

	UFUNCTION()
	void AssignMeshes();

	UFUNCTION()
	void CopyBodyTexturesData();
	//

	// Helper function
	TSharedPtr<FJsonValue> ParseJSON(FString JsonResponse);

	// Ramp and haircap
	UFUNCTION()
	FVector ApplyHairCap(const FVector DiffuseColor, const float map_val);

	UFUNCTION()
	uint8 ApplyHairCapRoughness(const uint8 roughness, const float map_valR, const float map_valG);

	void OnHaircapDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	//
};