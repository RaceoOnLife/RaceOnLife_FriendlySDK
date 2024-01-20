// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"

class FAvaturnUrlConvertor
{
public:
	static FAssetUri CreateAssetUri(const FString& Url, const FString& AssetId, const bool bWithFolder = true, const TCHAR* Extension = TEXT(".glb"));
};
