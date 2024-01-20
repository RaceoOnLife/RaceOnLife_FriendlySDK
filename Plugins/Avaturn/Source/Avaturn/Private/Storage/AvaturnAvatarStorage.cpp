// Copyright © 2023++ Avaturn


#include "AvaturnAvatarStorage.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

#if ENGINE_MAJOR_VERSION > 4
#include "HAL/PlatformFileManager.h"
#else
#include "HAL/PlatformFilemanager.h"
#endif

namespace
{
	FString GetAvatarCacheDir()
	{
		return FPaths::ProjectPersistentDownloadDir() + "/Avatars/";
	}
}

bool FAvaturnAvatarStorage::AssetExists(const FAssetUri& AvatarUri)
{
	return FileExists(AvatarUri.LocalModelPath);
}

bool FAvaturnAvatarStorage::FileExists(const FString& Path)
{
	return !Path.IsEmpty() && FPaths::FileExists(*Path);
}

void FAvaturnAvatarStorage::DeleteDirectory(const FString& Path)
{
	if (Path.IsEmpty())
	{
		return;
	}
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.DirectoryExists(*Path) && !PlatformFile.DeleteDirectoryRecursively(*Path))
	{
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to delete directory"));
	}
}

bool FAvaturnAvatarStorage::CheckAndRemoveExistingFile(const FString& FilePath)
{
	if (FilePath.IsEmpty())
	{
		return false;
	}
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString Path;
	FString Filename;
	FString Extension;
	FPaths::Split(FilePath, Path, Filename, Extension);
	if (!PlatformFile.DirectoryExists(*Path) && !PlatformFile.CreateDirectoryTree(*Path))
	{
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to create a directory to save the downloaded file"));
		return false;
	}

	if (PlatformFile.FileExists(*FilePath) && !PlatformFile.DeleteFile(*FilePath))
	{
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to delete the existing file"));
		return false;
	}
	return true;
}

void FAvaturnAvatarStorage::SaveAsset(const FString& GlbFilePath, const TArray<uint8>& Data)
{
	UE_LOG(LogAvaturn, Warning, TEXT("%s"), *GlbFilePath);
	UE_LOG(LogAvaturn, Warning, TEXT("%d"), Data.Num());
	if (!CheckAndRemoveExistingFile(GlbFilePath))
	{
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to delete the cached avatar model"));
	}
	if (!FFileHelper::SaveArrayToFile(Data, *GlbFilePath))
	{
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to save the downloaded file"));
	}
}

void FAvaturnAvatarStorage::ClearCache()
{
	DeleteDirectory(GetAvatarCacheDir());
}

bool FAvaturnAvatarStorage::IsCacheEmpty()
{
	return GetAssetCount() == 0;
}

void FAvaturnAvatarStorage::ClearAsset(const FString& Guid)
{
	DeleteDirectory(GetAvatarCacheDir() + Guid);
}

int32 FAvaturnAvatarStorage::GetAssetCount()
{
	const FString Path = GetAvatarCacheDir() + "*";
	TArray<FString> FoundDirs;
	IFileManager::Get().FindFiles(FoundDirs, *Path, false, true);
	return FoundDirs.Num();
}

int64 FAvaturnAvatarStorage::GetCacheSize()
{
	int64 DirectorySize = 0;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*GetAvatarCacheDir(),
		[&DirectorySize, &PlatformFile](const TCHAR* Filename, bool bIsDirectory) -> bool
		{
			if (!bIsDirectory)
			{
				DirectorySize += PlatformFile.FileSize(Filename);
			}
			return true;
		});
	return DirectorySize;
}
