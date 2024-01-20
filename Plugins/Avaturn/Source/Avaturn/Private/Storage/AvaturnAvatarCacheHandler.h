// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"

class FAvaturnAvatarCacheHandler
{
public:
	explicit FAvaturnAvatarCacheHandler(const FAssetUri& AssetUri);

	bool ShouldLoadFromCache() const;

	static bool IsCachingEnabled();

	void SaveAssetInCache(const FString AssetPath, const TArray<uint8>* Data) const;

private:
	const FAssetUri AssetUri;
};
