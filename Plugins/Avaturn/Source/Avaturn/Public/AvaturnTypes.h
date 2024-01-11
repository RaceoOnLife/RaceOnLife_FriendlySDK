// Copyright © 2023++ Avaturn
#pragma once

#include "Engine/DataTable.h"
#include "AvaturnTypes.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarDownloadCompleted, class USkeletalMesh*, SkeletalMesh);

DECLARE_DYNAMIC_DELEGATE(FAvatarLoadCompleted);

DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarLoadFailed, const FString&, ErrorMessage);

DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarPreloadCompleted, bool, bSuccess);

DECLARE_DYNAMIC_DELEGATE_OneParam(FGlbLoadCompleted, class USkeletalMesh*, SkeletalMesh);

struct FAssetUri
{
	FString Guid;
	FString ModelUrl;
	FString LocalModelPath;
	FString LocalAvatarDirectory;
};

USTRUCT(BlueprintType) struct FTargetSkeleton : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USkeleton> Skeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UAnimInstance> AnimClass;

	FTargetSkeleton()
	{
		Skeleton = NULL;
		AnimClass = NULL;
	}
};

USTRUCT(BlueprintType) struct FAvatarAsset
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Id;

	UPROPERTY(BlueprintReadWrite)
	FString Category;

	UPROPERTY(BlueprintReadWrite)
	FString Glb_Url;

	UPROPERTY(BlueprintReadWrite)
	FString Preview_Url;

	UPROPERTY(BlueprintReadWrite)
	FString Gender;

	UPROPERTY(BlueprintReadWrite)
	bool bDisableHead;

	UPROPERTY(BlueprintReadWrite)
	bool bDisableShoes;

	UPROPERTY(BlueprintReadWrite)
	bool bOnlyHaircap;

	UPROPERTY(BlueprintReadWrite)
	FString	HaircapId;

	UPROPERTY(BlueprintReadWrite)
	bool bUseColorRamp;

	UPROPERTY(BlueprintReadWrite)
	FString Alias;

	FAvatarAsset()
	{
		Id = NULL;
		Category = NULL;
		Glb_Url = NULL;
		Preview_Url = NULL;
		Gender = NULL;
		bDisableHead = false;
		bDisableShoes = false;
		bOnlyHaircap = false;
		HaircapId = NULL;
		bUseColorRamp = true;
		Alias = NULL;
	}
};

DECLARE_LOG_CATEGORY_EXTERN(LogAvaturn, Log, All);
