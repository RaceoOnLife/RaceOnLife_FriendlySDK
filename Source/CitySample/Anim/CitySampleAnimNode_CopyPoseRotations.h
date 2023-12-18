// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "CitySampleAnimNode_CopyPoseRotations.generated.h"

class USkeletalMeshComponent;
struct FAnimInstanceProxy;

USTRUCT()
struct FCitySample_BoneSourceToTargetMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FName TargetBoneName;

	UPROPERTY(EditAnywhere, Category = Settings)
	TArray<FName> SourceBoneNames;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool bIncludeFullTransform = false;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool bIncludeInputPose = false;
};

struct FCitySample_BoneSourceSettings
{
	TArray<int32, TInlineAllocator<2>> SourceBoneIndices;
	bool bIncludeFullTransform;
	bool bIncludeInputPose;
};

/**
 *	Simple controller to copy a bone's rotation to another one.
 */

USTRUCT(BlueprintInternalUseOnly)
struct CITYSAMPLE_API FCitySampleAnimNode_CopyPoseRotations : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink In;

	/*  This is used by default if it's valid */
	UPROPERTY(BlueprintReadWrite, transient, Category = Copy, meta = (PinShownByDefault))
	TWeakObjectPtr<USkeletalMeshComponent> SourceMeshComponent;

	UPROPERTY(EditAnywhere, Category = Copy, meta = (NeverAsPin))
	TArray<FCitySample_BoneSourceToTargetMapping> BoneMapping;
	
	/* If SourceMeshComponent is not valid, and if this is true, it will look for attahced parent as a source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	uint8 bUseAttachedParent : 1;

	/* Copy curves also from SouceMeshComponent. This will copy the curves if this instance also contains */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	uint8 bCopyCurves : 1;

	/* If you want to specify copy root, use this - this will ensure copy only below of this joint (inclusively) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	FName RootBoneToCopy;

	FCitySampleAnimNode_CopyPoseRotations();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	// End of FAnimNode_Base interface

private:
	// this is source mesh references, so that we could compare and see if it has changed
	TWeakObjectPtr<USkeletalMeshComponent>	CurrentlyUsedSourceMeshComponent;
	TWeakObjectPtr<USkeletalMesh>			CurrentlyUsedSourceMesh;
	TWeakObjectPtr<USkeletalMesh>			CurrentlyUsedMesh;

	// target mesh 
	TWeakObjectPtr<USkeletalMesh>			CurrentlyUsedTargetMesh;
	// cache of target space bases to source space bases
	TMap<int32, FCitySample_BoneSourceSettings> BoneMapToSourceSettings;
	TMap<FName, SmartName::UID_Type> CurveNameToUIDMap;

	// Cached transforms, copied on the game thread
	TArray<FTransform> SourceMeshTransformArray;

	// Cached curves, copied on the game thread
	TMap<FName, float> SourceCurveList;

	// Cached attributes, copied on the game thread
	UE::Anim::FHeapAttributeContainer SourceCustomAttributes;

	// reinitialize mesh component 
	void ReinitializeMeshComponent(USkeletalMeshComponent* NewSkeletalMeshComponent, USkeletalMeshComponent* TargetMeshComponent);
	void RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent);
};
