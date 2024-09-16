// Copyright © 2023++ Avaturn


#include "Utils/AvaturnGlTFConfigCreator.h"

void FAvaturnGlTFConfigCreator::OverrideConfig(FglTFRuntimeSkeletalMeshConfig& SkeletalMeshConfig, const FString& RootBoneName, USkeleton* Skeleton)
{
	SkeletalMeshConfig.MorphTargetsDuplicateStrategy = EglTFRuntimeMorphTargetsDuplicateStrategy::Merge;
	SkeletalMeshConfig.bAddVirtualBones = true;
	SkeletalMeshConfig.Skeleton = Skeleton;

	SkeletalMeshConfig.SkeletonConfig.CopyRotationsFrom = Skeleton;
	if (RootBoneName != "root")
	{
		SkeletalMeshConfig.SkeletonConfig.bAddRootBone = true;
	}
	SkeletalMeshConfig.SkeletonConfig.RootBoneName = RootBoneName;
}

FglTFRuntimeConfig FAvaturnGlTFConfigCreator::GetGlTFRuntimeConfig()
{
	FglTFRuntimeConfig RuntimeConfig;
	RuntimeConfig.TransformBaseType = EglTFRuntimeTransformBaseType::YForward;
	return RuntimeConfig;
}
