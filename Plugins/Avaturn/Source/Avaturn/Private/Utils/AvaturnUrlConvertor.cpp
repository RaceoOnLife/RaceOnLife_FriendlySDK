// Copyright © 2023++ Avaturn

#include "Utils/AvaturnUrlConvertor.h"

static const TCHAR* AVATARS_FOLDER = TEXT("Avatars");

FAssetUri FAvaturnUrlConvertor::CreateAssetUri(const FString& Url, const FString& AssetId, const bool bWithFolder, const TCHAR* Extension)
{
	FAssetUri AssetUri;

	AssetUri.ModelUrl = Url;
	AssetUri.Guid = AssetId;

	const FString AvatarsFolder = FPaths::ProjectPersistentDownloadDir() + "/" + AVATARS_FOLDER;
	const FString ModelFolderName = bWithFolder ? AssetUri.Guid : "";
	const FString ModelName = AssetUri.Guid;
	AssetUri.LocalAvatarDirectory = AvatarsFolder;
	AssetUri.LocalModelPath = AvatarsFolder + "/" + ModelFolderName + "/" + ModelName + Extension;

	return AssetUri;
}
