// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimNodes/AnimNode_PoseDriver.h"
#include "Animation/PoseAsset.h"
#include "Animation/AnimSequence.h"
#include "EditorReimportHandler.h"
#include "MetaHumanToolEditorBlueprintLibrary.generated.h"

class UAnimGraphNode_PoseDriver;

UCLASS()
class CITYSAMPLEEDITOR_API UMetaHumanToolEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void RetrievePoseDriverGraphNodes(UAnimBlueprint* AnimBlueprint, TArray<UAnimGraphNode_PoseDriver*>& OutNodes);

	UFUNCTION(BlueprintPure, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static FAnimNode_PoseDriver& GetPoseDriverNode(UAnimGraphNode_PoseDriver* GraphNode);
	
	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void SetSourceBones(UPARAM(ref) FAnimNode_PoseDriver& PoseDriveNode, const TArray<FName> BoneNames);

	UFUNCTION(BlueprintPure, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void GetSourceBones(UPARAM(ref) FAnimNode_PoseDriver& PoseDriveNode, TArray<FName>& OutBoneNames);

	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void SetDrivingBones(UPARAM(ref) FAnimNode_PoseDriver& PoseDriveNode, const TArray<FName> BoneNames);

	UFUNCTION(BlueprintPure, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void GetDrivingBones(UPARAM(ref) FAnimNode_PoseDriver& PoseDriveNode, TArray<FName>& OutBoneNames);

	// UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	// static void CopyTargetsFromPoseAsset(UAnimGraphNode_PoseDriver* GraphNode);

	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void UpdateFromAnimSequence(UPoseAsset* PoseAsset, UAnimSequence* AnimSequence);

	// UFUNCTION(BlueprintPure, Category = "MetaHumanCreator", meta = (ScriptMethod))
	// static const TArray<FName> GetPoseNames(UPoseAsset* PoseAsset);

	// UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	// static void RenamePose(UPoseAsset* PoseAsset, const FName& InOriginalName, const FName& InNewName);

	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void SetBpPreviewMesh(UAnimBlueprint* AnimBlueprint, USkeletalMesh* TargetPreviewSkeletalMesh);

	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void SetAnimPreviewMesh(UAnimSequence* AnimSequence, USkeletalMesh* TargetPreviewSkeletalMesh);

	UFUNCTION(BlueprintCallable, Category = "MetaHumanCreator", meta = (ScriptMethod))
	static void SetPoseAssetPreviewMesh(UPoseAsset* PoseAsset, USkeletalMesh* TargetPreviewSkeletalMesh);
};

