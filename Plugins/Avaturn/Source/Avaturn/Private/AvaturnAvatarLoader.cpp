// Copyright © 2023++ Avaturn


#include "AvaturnAvatarLoader.h"

#include "Utils/AvaturnGlTFConfigCreator.h"
#include "Utils/AvaturnRequestCreator.h"
#include "Utils/AvaturnUrlConvertor.h"
#include "AvaturnMemoryCache.h"
#include "AvaturnGameSubsystem.h"
#include "AvaturnGlbLoader.h"
#include "Storage/AvaturnAvatarCacheHandler.h"

#include "Components/SkeletalMeshComponent.h"
#include "glTFRuntimeFunctionLibrary.h"
#include "Misc/Base64.h"

static const FString HEADER_LAST_MODIFIED = "Last-Modified";
constexpr float AVATAR_REQUEST_TIMEOUT = 60.f;


UAvaturnAvatarLoader::UAvaturnAvatarLoader()
	: SkeletalMesh(nullptr)
{
}

void UAvaturnAvatarLoader::LoadAvatar(const bool bLDataUrl, USkeleton* TargetSkeleton, const FAvatarDownloadCompleted& OnDownloadCompleted, const FAvatarLoadFailed& OnLoadFailed)
{
	if (ModelUrl.IsEmpty() && AvatarId.IsEmpty())
	{
		(void)OnLoadFailed.ExecuteIfBound("Url invalid");
		return;
	}

	bDataURL = bLDataUrl;

	Reset();
	OnAvatarDownloadCompleted = OnDownloadCompleted;
	OnAvatarLoadFailed = OnLoadFailed;

	const UAvaturnGameSubsystem* GameSubsystem = UGameInstance::GetSubsystem<UAvaturnGameSubsystem>(GetWorld()->GetGameInstance());
	if (IsValid(GameSubsystem))
	{
		AvatarUri = FAvaturnUrlConvertor::CreateAvatarUri(ModelUrl, AvatarId);
	}
	CacheHandler = MakeShared<FAvaturnAvatarCacheHandler>(*AvatarUri);

	GlbLoader = NewObject<UAvaturnGlbLoader>(this, TEXT("GlbLoader"));
	GlbLoader->TargetSkeleton = TargetSkeleton;
	GlbLoader->Root_Bone = Root_Bone;
	OnGlbLoadCompleted.BindDynamic(this, &UAvaturnAvatarLoader::OnGlbLoaded);

	//if (CacheHandler->ShouldLoadFromCache())
	//{
		//We try to load from cache
		//GlbLoader->LoadFromFile(AvatarUri->LocalModelPath, OnGlbLoadCompleted);
	//}
	//else
	//{
		DownloadAvatarModel();
	//}
}

void UAvaturnAvatarLoader::LoadAvatarFromFile(const FString& Path, USkeleton* TargetSkeleton, const FAvatarDownloadCompleted& OnDownloadCompleted, const FAvatarLoadFailed& OnLoadFailed)
{
	if (Path.IsEmpty())
	{
		(void)OnLoadFailed.ExecuteIfBound("Path invalid");
		return;
	}

	Reset();
	OnAvatarDownloadCompleted = OnDownloadCompleted;
	OnAvatarLoadFailed = OnLoadFailed;

	const UAvaturnGameSubsystem* GameSubsystem = UGameInstance::GetSubsystem<UAvaturnGameSubsystem>(GetWorld()->GetGameInstance());
	if (IsValid(GameSubsystem))
	{
		AvatarUri = FAvaturnUrlConvertor::CreateAvatarUri(ModelUrl, AvatarId);
	}
	CacheHandler = MakeShared<FAvaturnAvatarCacheHandler>(*AvatarUri);

	GlbLoader = NewObject<UAvaturnGlbLoader>(this, TEXT("GlbLoader"));
	GlbLoader->TargetSkeleton = TargetSkeleton;
	GlbLoader->Root_Bone = Root_Bone;
	OnGlbLoadCompleted.BindDynamic(this, &UAvaturnAvatarLoader::OnGlbLoaded);

	GlbLoader->LoadFromFile(Path, OnGlbLoadCompleted);
}

void UAvaturnAvatarLoader::CancelAvatarLoad()
{
	if (AvatarModelRequest.IsValid() && (AvatarModelRequest->GetStatus() == EHttpRequestStatus::Type::Processing || AvatarModelRequest->GetStatus() == EHttpRequestStatus::Type::NotStarted))
	{
		AvatarModelRequest->CancelRequest();
	}
	Reset();
}

void UAvaturnAvatarLoader::ExecuteSuccessCallback()
{
	if (SkeletalMesh != nullptr)
	{
		CacheHandler->SaveAvatarInCache();
		(void)OnAvatarDownloadCompleted.ExecuteIfBound(SkeletalMesh);
		Reset();
	}
}

void UAvaturnAvatarLoader::ExecuteFailureCallback(const FString& ErrorMessage)
{
	(void)OnAvatarLoadFailed.ExecuteIfBound(ErrorMessage);
	Reset();
}

void UAvaturnAvatarLoader::DownloadAvatarModel()
{
	// If object(data) Url
	if (bDataURL)
	{
		FBase64::Decode(ModelUrl, Content);

		CacheHandler->SetModelData(&Content);
		GlbLoader->LoadFromData(Content, OnGlbLoadCompleted);
	}
	else
	{
		// If Http Url
		AvatarDownloadTime = FDateTime::Now();
		AvatarModelRequest = FAvaturnRequestCreator::MakeHttpRequest(ModelUrl, AVATAR_REQUEST_TIMEOUT);
		AvatarModelRequest->OnProcessRequestComplete().BindUObject(this, &UAvaturnAvatarLoader::OnAvatarModelReceived);
		AvatarModelRequest->SetVerb(TEXT("GET"));
		AvatarModelRequest->ProcessRequest();
	}
}

void UAvaturnAvatarLoader::OnAvatarModelReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!OnAvatarDownloadCompleted.IsBound())
	{
		return;
	}
	if (bSuccess && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogAvaturn, Log, TEXT("Model Downloaded in [%.1fs]"), (FDateTime::Now() - AvatarDownloadTime).GetTotalSeconds());

		CacheHandler->SetModelData(&Response->GetContent());
		GlbLoader->LoadFromData(Response->GetContent(), OnGlbLoadCompleted);
	}
	else
	{
		ExecuteFailureCallback("Failed to download avatar");
	}
}

void UAvaturnAvatarLoader::OnGlbLoaded(USkeletalMesh* Mesh)
{
	if (!OnAvatarDownloadCompleted.IsBound())
	{
		return;
	}
	if (Mesh != nullptr)
	{
		SkeletalMesh = Mesh;
		ExecuteSuccessCallback();
	}
	else
	{
		ExecuteFailureCallback("Failed to load the avatar model");
	}
	OnGlbLoadCompleted.Unbind();
}

void UAvaturnAvatarLoader::Reset()
{
	SkeletalMesh = nullptr;
	OnAvatarDownloadCompleted.Unbind();
	OnAvatarLoadFailed.Unbind();
	AvatarModelRequest.Reset();
}

void UAvaturnAvatarLoader::BeginDestroy()
{
	CancelAvatarLoad();
	Super::BeginDestroy();
}
