// Copyright © 2023++ Avaturn


#include "Utils/AvaturnUrlConvertor.h"

#include "Internationalization/Regex.h"

// example link https://api.avaturn.me/avatars/exports/0187ac02-98a9-7e63-9c49-f43b78e2853d/model

static const FString URL_PREFIX = "https://api.avaturn.me/avatars/exports/";
static const TCHAR* SUFFIX_GLB = TEXT(".glb");
static const TCHAR* AVATARS_FOLDER = TEXT("Avatars");

FAvatarUri FAvaturnUrlConvertor::CreateAvatarUri(const FString& Url, const FString AvatarId)
{
	FAvatarUri AvatarUri;

	AvatarUri.ModelUrl = Url;
	AvatarUri.Guid = AvatarId;

	const FString AvatarsFolder = FPaths::ProjectPersistentDownloadDir() + "/" + AVATARS_FOLDER;
	const FString ModelFolderName = AvatarUri.Guid;
	AvatarUri.LocalAvatarDirectory = AvatarsFolder;
	AvatarUri.LocalModelPath = AvatarsFolder + "/" + ModelFolderName + "/" + ModelFolderName + SUFFIX_GLB;

	return AvatarUri;
}
