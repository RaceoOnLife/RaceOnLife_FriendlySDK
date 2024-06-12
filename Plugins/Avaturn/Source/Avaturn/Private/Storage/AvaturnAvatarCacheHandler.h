// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"

class FAvaturnAvatarCacheHandler
{
public:
	explicit FAvaturnAvatarCacheHandler(const FAvatarUri& AvatarUri);

	void SetModelData(const TArray<uint8>* Data);

	void SaveAvatarInCache() const;

	bool ShouldLoadFromCache() const;

	static bool IsCachingEnabled();

private:
	const FAvatarUri AvatarUri;

	const TArray<uint8>* ModelData;
};
