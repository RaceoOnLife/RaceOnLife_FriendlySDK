// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanToolEditorBlueprintLibrary.h"

#include "AnimGraphNode_PoseDriver.h"
#include "AnimNodes/AnimNode_PoseDriver.h"
#include "AnimationGraph.h"

#include "EdGraph/EdGraph.h"

void UMetaHumanToolEditorFunctionLibrary::RetrievePoseDriverGraphNodes(UAnimBlueprint* AnimBlueprint, TArray<UAnimGraphNode_PoseDriver*>& OutNodes)
{
	if (AnimBlueprint)
	{
		for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
		{
			if (UAnimationGraph* AnimGraph = Cast<UAnimationGraph>(Graph))
			{
				AnimGraph->GetNodesOfClass<UAnimGraphNode_PoseDriver>(OutNodes);
				break;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to retrieve PoseDriver graph nodes because argument AnimBlueprint is invalid"));
	}
}

FAnimNode_PoseDriver& UMetaHumanToolEditorFunctionLibrary::GetPoseDriverNode(UAnimGraphNode_PoseDriver* GraphNode)
{
	ensure(GraphNode);
	return GraphNode->Node;
}

void UMetaHumanToolEditorFunctionLibrary::SetSourceBones(FAnimNode_PoseDriver& PoseDriveNode, const TArray<FName> BoneNames)
{
	PoseDriveNode.SourceBones.Empty(BoneNames.Num());
	for (const FName& BoneName : BoneNames)
	{
		PoseDriveNode.SourceBones.Add(BoneName);
	}
}

void UMetaHumanToolEditorFunctionLibrary::GetSourceBones(FAnimNode_PoseDriver& PoseDriveNode, TArray<FName>& OutBoneNames)
{
	for (const FBoneReference& SourceBone : PoseDriveNode.SourceBones)
	{
		OutBoneNames.Add(SourceBone.BoneName);
	}
}

void UMetaHumanToolEditorFunctionLibrary::SetDrivingBones(FAnimNode_PoseDriver& PoseDriveNode, const TArray<FName> BoneNames)
{
	PoseDriveNode.OnlyDriveBones.Empty(BoneNames.Num());
	for (const FName& BoneName : BoneNames)
	{
		PoseDriveNode.OnlyDriveBones.Add(BoneName);
	}
}

void UMetaHumanToolEditorFunctionLibrary::GetDrivingBones(FAnimNode_PoseDriver& PoseDriveNode, TArray<FName>& OutBoneNames)
{
	for (const FBoneReference& SourceBone : PoseDriveNode.OnlyDriveBones)
	{
		OutBoneNames.Add(SourceBone.BoneName);
	}
}

//void UMetaHumanToolEditorFunctionLibrary::CopyTargetsFromPoseAsset(UAnimGraphNode_PoseDriver* GraphNode)
//{
//	if (!GraphNode)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Failed to copy targets because argument GraphNode is invalid"));
//		return;
//	}
//
//	GraphNode->CopyTargetsFromPoseAsset();
//}

void UMetaHumanToolEditorFunctionLibrary::UpdateFromAnimSequence(UPoseAsset* PoseAsset, UAnimSequence* AnimSequence)
{
	if (!PoseAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to update PoseAsset pose because argument PoseAsset is invalid"));
		return;
	}

	if (!AnimSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to update PoseAsset pose because argument AnimSequence is invalid"));
		return;
	}

	PoseAsset->UpdatePoseFromAnimation(AnimSequence);
}

//const TArray<FName> UMetaHumanToolEditorFunctionLibrary::GetPoseNames(UPoseAsset* PoseAsset)
//{
//	TArray<FName> PoseNames;
//
//	if (!PoseAsset)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Failed to get pose names because argument PoseAsset is invalid"));
//		return PoseNames;
//	}
//
//	
//	int32 numPoses = PoseAsset->GetNumPoses();
//	for (int32 PoseIndex = 0; PoseIndex < numPoses; ++PoseIndex)
//	{
//		PoseNames.Add(PoseAsset->GetPoseNameByIndex(PoseIndex));
//	}
//
//	return PoseNames;
//}

//void UMetaHumanToolEditorFunctionLibrary::RenamePose(UPoseAsset* PoseAsset, const FName& InOriginalName, const FName& InNewName)
//{
//	if (PoseAsset == nullptr)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Failed to rename pose because argument PoseAsset is invalid"));
//		return;
//	}
//
//	PoseAsset->ModifyPoseName(InOriginalName, InNewName, nullptr);
//}

void UMetaHumanToolEditorFunctionLibrary::SetBpPreviewMesh(UAnimBlueprint* AnimBlueprint, USkeletalMesh* TargetPreviewSkeletalMesh)
{
	AnimBlueprint->SetPreviewMesh(TargetPreviewSkeletalMesh, true);
}

void UMetaHumanToolEditorFunctionLibrary::SetAnimPreviewMesh(UAnimSequence* AnimSequence, USkeletalMesh* TargetPreviewSkeletalMesh)
{
	AnimSequence->SetPreviewMesh(TargetPreviewSkeletalMesh, true);
}

void UMetaHumanToolEditorFunctionLibrary::SetPoseAssetPreviewMesh(UPoseAsset* PoseAsset, USkeletalMesh* TargetPreviewSkeletalMesh)
{
	PoseAsset->SetPreviewMesh(TargetPreviewSkeletalMesh, true);
}