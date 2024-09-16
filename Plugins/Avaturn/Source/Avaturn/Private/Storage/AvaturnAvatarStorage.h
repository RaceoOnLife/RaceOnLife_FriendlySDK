// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"

class FAvaturnAvatarStorage
{
public:
	static void SaveAvatar(const FString& GlbFilePath, const TArray<uint8>& Data);
	
	static bool AvatarExists(const FAvatarUri& AvatarUri);
	static bool FileExists(const FString& Path);
	static void ClearCache();
	static void DeleteDirectory(const FString& Path);
	static bool IsCacheEmpty();
	static void ClearAvatar(const FString& Guid);
	static int32 GetAvatarCount();
	static int64 GetCacheSize();

private:
	static bool CheckAndRemoveExistingFile(const FString& FilePath);
};
