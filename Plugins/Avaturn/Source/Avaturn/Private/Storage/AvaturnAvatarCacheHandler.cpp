// Copyright © 2023++ Avaturn


#include "AvaturnAvatarCacheHandler.h"
#include "AvaturnAvatarStorage.h"
#include "AvaturnSettings.h"


FAvaturnAvatarCacheHandler::FAvaturnAvatarCacheHandler(const FAssetUri& AssetUri)
	: AssetUri(AssetUri)
{
}

bool FAvaturnAvatarCacheHandler::IsCachingEnabled()
{
	const UAvaturnSettings* Settings = GetDefault<UAvaturnSettings>();
	if (Settings)
	{
		return Settings->bEnableAvatarCaching;
	}
	return false;
}

bool FAvaturnAvatarCacheHandler::ShouldLoadFromCache() const
{
	return IsCachingEnabled() && FAvaturnAvatarStorage::AssetExists(AssetUri);
}

void FAvaturnAvatarCacheHandler::SaveAssetInCache(const FString AssetPath, const TArray<uint8>* Data) const
{
	if (IsCachingEnabled() && Data != nullptr)
	{
		FAvaturnAvatarStorage::SaveAsset(AssetPath, *Data);
	}
}