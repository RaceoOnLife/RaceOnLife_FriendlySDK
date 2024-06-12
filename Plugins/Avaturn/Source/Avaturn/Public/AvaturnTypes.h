// Copyright © 2023++ Avaturn
#pragma once

#include "AvaturnTypes.generated.h"

UENUM(BlueprintType)
enum class EStandardMorphTargetGroup : uint8
{
	None UMETA(DisplayName = "None"),
};


DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarDownloadCompleted, class USkeletalMesh*, SkeletalMesh);

DECLARE_DYNAMIC_DELEGATE(FAvatarLoadCompleted);

DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarLoadFailed, const FString&, ErrorMessage);

DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarPreloadCompleted, bool, bSuccess);

DECLARE_DYNAMIC_DELEGATE_OneParam(FGlbLoadCompleted, class USkeletalMesh*, SkeletalMesh);

struct FAvatarUri
{
	FString Guid;
	FString ModelUrl;
	FString LocalModelPath;
	FString LocalAvatarDirectory;
};

USTRUCT(BlueprintType) struct FExportAvatarResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Url;

	UPROPERTY(BlueprintReadWrite)
	FString AvatarId;

	UPROPERTY(BlueprintReadWrite)
	bool bAvatarSupportsFaceAnimations;

	UPROPERTY(BlueprintReadWrite)
	FString BodyId;

	UPROPERTY(BlueprintReadWrite)
	FString Gender;

	UPROPERTY(BlueprintReadWrite)
	FString SessionId;

	UPROPERTY(BlueprintReadWrite)
	bool bDataUrl;

	FExportAvatarResult()
	{
		Url = NULL;
		AvatarId = NULL;
		bAvatarSupportsFaceAnimations = false;
		BodyId = NULL;
		Gender = NULL;
		SessionId = NULL;
		bDataUrl = false;
	}

	bool IsValid()
	{
		return !Url.IsEmpty() && !AvatarId.IsEmpty() && !BodyId.IsEmpty() && !Gender.IsEmpty();
	}
};

USTRUCT(BlueprintType) struct FTargetSkeleton
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
	FString Preview;

	FAvatarAsset()
	{
		Id = NULL;
		Category = NULL;
		Preview = NULL;
	}
};

USTRUCT(BlueprintType) struct FAvatarBody
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Id;

	UPROPERTY(BlueprintReadWrite)
	FString Preview;

	UPROPERTY(BlueprintReadWrite)
	FString Gender;

	FAvatarBody()
	{
		Id = NULL;
		Preview = NULL;
		Gender = NULL;
	}
};

DECLARE_LOG_CATEGORY_EXTERN(LogAvaturn, Log, All);
