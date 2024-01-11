// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"

#include "glTFRuntimeParser.h"

class FAvaturnGlTFConfigCreator
{
public:
	static void OverrideConfig(FglTFRuntimeSkeletalMeshConfig& SkeletalMeshConfig, const FString& RootBoneName, USkeleton* Skeleton);

	static FglTFRuntimeConfig GetGlTFRuntimeConfig();

};
