// Copyright © 2023++ Avaturn

#include "AvaturnGlbLoader.h"
#include "Utils/AvaturnGlTFConfigCreator.h"
#include "glTFRuntimeFunctionLibrary.h"

UAvaturnGlbLoader::UAvaturnGlbLoader()
{
	OnSkeletalMeshCallback.BindDynamic(this, &UAvaturnGlbLoader::OnSkeletalMeshLoaded);
}
                                              
void UAvaturnGlbLoader::LoadFromFile(const FString& LocalModelPath, const FGlbLoadCompleted& LoadCompleted)
{
	OnLoadCompleted = LoadCompleted;
	UglTFRuntimeAsset* Asset = UglTFRuntimeFunctionLibrary::glTFLoadAssetFromFilename(LocalModelPath, false, FAvaturnGlTFConfigCreator::GetGlTFRuntimeConfig());
	LoadSkeletalMesh(Asset);
}

void UAvaturnGlbLoader::LoadFromData(const TArray<uint8>& Data, const FGlbLoadCompleted& LoadCompleted)
{
	OnLoadCompleted = LoadCompleted;
	UglTFRuntimeAsset* Asset = UglTFRuntimeFunctionLibrary::glTFLoadAssetFromData(Data, FAvaturnGlTFConfigCreator::GetGlTFRuntimeConfig());
	LoadSkeletalMesh(Asset);
}

void UAvaturnGlbLoader::LoadSkeletalMesh(UglTFRuntimeAsset* Asset)
{
	if (Asset == nullptr)
	{
		(void)OnLoadCompleted.ExecuteIfBound(nullptr);
		UE_LOG(LogAvaturn, Warning, TEXT("Failed to load the avatar model"));
	}
	const FString RootBoneName = Root_Bone;
	FAvaturnGlTFConfigCreator::OverrideConfig(SkeletalMeshConfig, RootBoneName, TargetSkeleton);

	Asset->LoadSkeletalMeshRecursiveAsync(Asset->GetNodes().Last().Name, {}, OnSkeletalMeshCallback, SkeletalMeshConfig);
}

void UAvaturnGlbLoader::OnSkeletalMeshLoaded(USkeletalMesh* SkeletalMesh)
{
	(void)OnLoadCompleted.ExecuteIfBound(SkeletalMesh);
}
