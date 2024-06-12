// Copyright © 2023++ Avaturn


#include "AvaturnMemoryCache.h"
#include "AvaturnAvatarLoader.h"

void UAvaturnMemoryCache::Preload(const TArray<FAvatarPreloadData>& PreloadDataList, const FAvatarPreloadCompleted& OnPreloadCompleted)
{
	OnAvatarDownloadCompleted.BindDynamic(this, &UAvaturnMemoryCache::OnAvatarDownloaded);
	OnLoadFailed.BindDynamic(this, &UAvaturnMemoryCache::OnAvatarLoadFailed);
	OnAvatarPreloadCompleted = OnPreloadCompleted;

	for (const auto& PreloadData : PreloadDataList)
	{
		const FAvatarMemoryCacheData CacheData = GetAvatarCacheData(PreloadData.ExportResult);
		if (CacheData.Mesh == nullptr)
		{
			UAvaturnAvatarLoader* AvatarLoader = NewObject<UAvaturnAvatarLoader>(this);
			AvatarLoader->LoadAvatar(false, PreloadData.TargetSkeleton, OnAvatarDownloadCompleted, OnLoadFailed);
			AvatarLoaders.Add(AvatarLoader, PreloadData);
		}
	}
}

FAvatarMemoryCacheData UAvaturnMemoryCache::GetAvatarCacheData(const FExportAvatarResult& ExportResult) const
{
	const auto CacheData = CachedAvatars.FindByPredicate([&ExportResult](const FAvatarMemoryCacheData& Data){return Data.ExportResult.AvatarId == ExportResult.AvatarId;});
	if (CacheData != nullptr)
	{
		return *CacheData;
	}
	return {};
}

void UAvaturnMemoryCache::AddAvatar(const FExportAvatarResult& ExportResult, USkeletalMesh* Mesh)
{
	const FAvatarMemoryCacheData CacheData = GetAvatarCacheData(ExportResult);
	if (CacheData.Mesh == nullptr)
	{
		CachedAvatars.Add({ ExportResult, Mesh});
	}
}

void UAvaturnMemoryCache::RemoveAvatar(const FExportAvatarResult& ExportResult)
{
	CachedAvatars.RemoveAll([&ExportResult](const FAvatarMemoryCacheData& Data){return Data.ExportResult.AvatarId == ExportResult.AvatarId;});
}

void UAvaturnMemoryCache::ClearAvatars()
{
	CachedAvatars.Empty();
}

void UAvaturnMemoryCache::OnAvatarDownloaded(USkeletalMesh* Mesh)
{
	const UAvaturnAvatarLoader* AvatarLoader = nullptr;

	for (const auto& Pair : AvatarLoaders)
	{
		if (Pair.Key->SkeletalMesh == Mesh)
		{
			AvatarLoader = Pair.Key;
			break;
		}
	}

	AddAvatar(AvatarLoaders[AvatarLoader].ExportResult, Mesh);
	AvatarLoaders.Remove(AvatarLoader);
	CompleteLoading();
}

void UAvaturnMemoryCache::OnAvatarLoadFailed(const FString& ErrorMessage)
{
	++FailedRequestCount;
	CompleteLoading();
}

void UAvaturnMemoryCache::CompleteLoading()
{
	if (AvatarLoaders.Num() == FailedRequestCount)
	{
		OnLoadFailed.Unbind();
		OnAvatarDownloadCompleted.Unbind();
		AvatarLoaders.Empty();
		(void)OnAvatarPreloadCompleted.ExecuteIfBound(FailedRequestCount == 0);
	}
}
