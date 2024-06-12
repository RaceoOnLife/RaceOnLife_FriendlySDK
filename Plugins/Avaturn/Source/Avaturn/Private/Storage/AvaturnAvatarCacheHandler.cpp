// Copyright © 2023++ Avaturn


#include "AvaturnAvatarCacheHandler.h"
#include "AvaturnAvatarStorage.h"
#include "AvaturnSettings.h"


FAvaturnAvatarCacheHandler::FAvaturnAvatarCacheHandler(const FAvatarUri& AvatarUri)
	: AvatarUri(AvatarUri)
	, ModelData(nullptr)
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
	return IsCachingEnabled() && FAvaturnAvatarStorage::AvatarExists(AvatarUri);
}

void FAvaturnAvatarCacheHandler::SetModelData(const TArray<uint8>* Data)
{
	if (!IsCachingEnabled())
	{
		return;
	}
	// We store the pointer because we don't want to copy the avatar data.
	ModelData = Data;
}

void FAvaturnAvatarCacheHandler::SaveAvatarInCache() const
{
	if (IsCachingEnabled() && ModelData != nullptr)
	{
		FAvaturnAvatarStorage::SaveAvatar(AvatarUri.LocalModelPath, *ModelData);
	}
}
