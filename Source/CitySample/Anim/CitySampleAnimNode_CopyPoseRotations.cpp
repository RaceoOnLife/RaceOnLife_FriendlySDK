// Copyright Epic Games, Inc. All Rights Reserved.
#include "Anim/CitySampleAnimNode_CopyPoseRotations.h"
#include "Animation/AnimInstanceProxy.h"
#include "ReferenceSkeleton.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimNode_CopyPoseRotations"

/////////////////////////////////////////////////////
// FAnimNode_CopyPoseRotations

FCitySampleAnimNode_CopyPoseRotations::FCitySampleAnimNode_CopyPoseRotations()
	: SourceMeshComponent(nullptr)
	, bUseAttachedParent(false)
	, bCopyCurves(false)
{
}

void FCitySampleAnimNode_CopyPoseRotations::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);

	In.Initialize(Context);

	// Initial update of the node, so we dont have a frame-delay on setup
	GetEvaluateGraphExposedInputs().Execute(Context);
}

void FCitySampleAnimNode_CopyPoseRotations::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	In.CacheBones(Context);
}

void FCitySampleAnimNode_CopyPoseRotations::RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent)
{
	auto ResetMeshComponent = [this](USkeletalMeshComponent* InMeshComponent, USkeletalMeshComponent* InTargetMeshComponent)
	{
		USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent.Get();
		// if current mesh exists, but not same as input mesh
		if (CurrentMeshComponent)
		{
			// if component has been changed, reinitialize
			if (CurrentMeshComponent != InMeshComponent)
			{
				ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
			}
			// if component is still same but mesh has been changed, we have to reinitialize
			else if (CurrentMeshComponent->GetSkeletalMeshAsset() != CurrentlyUsedSourceMesh.Get())
			{
				ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
			}
			else if (InTargetMeshComponent)
			{
				// see if target mesh has changed
				if (InTargetMeshComponent->GetSkeletalMeshAsset() != CurrentlyUsedTargetMesh.Get())
				{
					ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
				}
			}
		}
		// if not valid, but input mesh is
		else if (!CurrentMeshComponent && InMeshComponent)
		{
			ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
		}
	};

	if (SourceMeshComponent.IsValid())
	{
		ResetMeshComponent(SourceMeshComponent.Get(), TargetMeshComponent);
	}
	else if (bUseAttachedParent)
	{
		if (TargetMeshComponent)
		{
			USkeletalMeshComponent* ParentComponent = Cast<USkeletalMeshComponent>(TargetMeshComponent->GetAttachParent());
			if (ParentComponent)
			{
				ResetMeshComponent(ParentComponent, TargetMeshComponent);
			}
			else
			{
				CurrentlyUsedSourceMeshComponent.Reset();
			}
		}
		else
		{
			CurrentlyUsedSourceMeshComponent.Reset();
		}
	}
	else
	{
		CurrentlyUsedSourceMeshComponent.Reset();
	}
}

void FCitySampleAnimNode_CopyPoseRotations::PreUpdate(const UAnimInstance* InAnimInstance)
{
	QUICK_SCOPE_CYCLE_COUNTER(FAnimNode_CopyPoseRotations_PreUpdate);

	RefreshMeshComponent(InAnimInstance->GetSkelMeshComponent());

	USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get() : nullptr;

	if (CurrentMeshComponent && CurrentMeshComponent->GetSkeletalMeshAsset() && CurrentMeshComponent->IsRegistered())
	{
		// If our source is running under leader-pose, then get bone data from there
		if (USkeletalMeshComponent* LeaderPoseComponent = Cast<USkeletalMeshComponent>(CurrentMeshComponent->LeaderPoseComponent.Get()))
		{
			CurrentMeshComponent = LeaderPoseComponent;
		}

		// re-check mesh component validity as it may have changed to leader
		if (CurrentMeshComponent->GetSkeletalMeshAsset() && CurrentMeshComponent->IsRegistered())
		{
			const bool bUROInSync = CurrentMeshComponent->ShouldUseUpdateRateOptimizations() && CurrentMeshComponent->AnimUpdateRateParams != nullptr && CurrentMeshComponent->AnimUpdateRateParams == InAnimInstance->GetSkelMeshComponent()->AnimUpdateRateParams;
			const bool bUsingExternalInterpolation = CurrentMeshComponent->IsUsingExternalInterpolation();
			const TArray<FTransform>& CachedComponentSpaceTransforms = CurrentMeshComponent->GetCachedComponentSpaceTransforms();
			const bool bArraySizesMatch = CachedComponentSpaceTransforms.Num() == CurrentMeshComponent->GetComponentSpaceTransforms().Num();

			// Copy source array from the appropriate location
			SourceMeshTransformArray.Reset();
			SourceMeshTransformArray.Append((bUROInSync || bUsingExternalInterpolation) && bArraySizesMatch ? CachedComponentSpaceTransforms : CurrentMeshComponent->GetComponentSpaceTransforms());

			// Ref skeleton is need for parent index lookups later, so store it now
			CurrentlyUsedMesh = CurrentMeshComponent->GetSkeletalMeshAsset();

			if (bCopyCurves)
			{
				UAnimInstance* SourceAnimInstance = CurrentMeshComponent->GetAnimInstance();
				if (SourceAnimInstance)
				{
					// attribute curve contains all list
					SourceCurveList.Reset();
					SourceCurveList.Append(SourceAnimInstance->GetAnimationCurveList(EAnimCurveType::AttributeCurve));
				}
				else
				{
					SourceCurveList.Reset();
				}
			}
		}
		else
		{
			CurrentlyUsedMesh.Reset();
		}
	}
}

void FCitySampleAnimNode_CopyPoseRotations::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	// This introduces a frame of latency in setting the pin-driven source component,
	// but we cannot do the work to extract transforms on a worker thread as it is not thread safe.
	GetEvaluateGraphExposedInputs().Execute(Context);

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Component"), *GetNameSafe(CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get() : nullptr));
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Mesh"), *GetNameSafe(CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get()->GetSkeletalMeshAsset() : nullptr));

	In.Update(Context);
}

void FCitySampleAnimNode_CopyPoseRotations::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)

	FPoseContext InputPose(Output);
	In.Evaluate(InputPose);

	// Initialize OutPose with InputPose. 
	FCompactPose& OutPose = Output.Pose;
	OutPose.CopyBonesFrom(InputPose.Pose);


	// Copy the rotations from the SourceMeshPose to the Output Pose. 
	//TODO - Continue from here

	USkeletalMesh* CurrentMesh = CurrentlyUsedMesh.IsValid() ? CurrentlyUsedMesh.Get() : nullptr;
	if (SourceMeshTransformArray.Num() > 0 && CurrentMesh)
	{
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();

		for (FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
		{
			const int32 SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);
			const int32 MeshBoneIndex = RequiredBones.GetMeshPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(SkeletonBoneIndex)).GetInt();
			const FCitySample_BoneSourceSettings* Value = BoneMapToSourceSettings.Find(MeshBoneIndex);
			if (Value)
			{
				FTransform DesiredTargetTransform = FTransform::Identity;

				if (Value->bIncludeInputPose)
				{
					DesiredTargetTransform = OutPose[PoseBoneIndex];
				}

				for(int32 SourceIdx = 0; SourceIdx < Value->SourceBoneIndices.Num(); ++SourceIdx)
				{
					int32 SourceBoneIndex = Value->SourceBoneIndices[SourceIdx];
					
					if(SourceMeshTransformArray.IsValidIndex(SourceBoneIndex))
					{
						const int32 ParentIndex = CurrentMesh->GetRefSkeleton().GetParentIndex(SourceBoneIndex);
						const FCompactPoseBoneIndex MyParentIndex = RequiredBones.GetParentBoneIndex(PoseBoneIndex);
						// only apply if I also have parent, otherwise, it should apply the space bases
						if (SourceMeshTransformArray.IsValidIndex(ParentIndex) && MyParentIndex != INDEX_NONE)
						{
							const FTransform ParentTransform = SourceMeshTransformArray[ParentIndex];
							const FTransform ChildTransform = SourceMeshTransformArray[SourceBoneIndex];
							DesiredTargetTransform.Accumulate(ChildTransform.GetRelativeTransform(ParentTransform));
						}
						else
						{
							DesiredTargetTransform.Accumulate(SourceMeshTransformArray[SourceBoneIndex]);
						}
					}
				}

				if (Value->bIncludeFullTransform)
				{
					OutPose[PoseBoneIndex] = FTransform(DesiredTargetTransform.GetRotation());
				}
				else
				{
					OutPose[PoseBoneIndex].SetRotation(DesiredTargetTransform.GetRotation());
				}
			}
		}
	}

	if (bCopyCurves)
	{
		for (auto Iter = SourceCurveList.CreateConstIterator(); Iter; ++Iter)
		{
			const SmartName::UID_Type* UID = CurveNameToUIDMap.Find(Iter.Key());
			if (UID)
			{
				// set source value to output curve
				Output.Curve.Set(*UID, Iter.Value());
			}
		}
	}
}

void FCitySampleAnimNode_CopyPoseRotations::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += FString::Printf(TEXT("('%s')"), *GetNameSafe(CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get()->GetSkeletalMeshAsset() : nullptr));
	DebugData.AddDebugItem(DebugLine, true);

	In.GatherDebugData(DebugData.BranchFlow(1.f));
}

void FCitySampleAnimNode_CopyPoseRotations::ReinitializeMeshComponent(USkeletalMeshComponent* NewSourceMeshComponent, USkeletalMeshComponent* TargetMeshComponent)
{
	CurrentlyUsedSourceMeshComponent.Reset();
	// reset source mesh
	CurrentlyUsedSourceMesh.Reset();
	CurrentlyUsedTargetMesh.Reset();
	BoneMapToSourceSettings.Reset();
	CurveNameToUIDMap.Reset();

	if (TargetMeshComponent && IsValid(NewSourceMeshComponent) && NewSourceMeshComponent->GetSkeletalMeshAsset())
	{
		USkeletalMesh* SourceSkelMesh = NewSourceMeshComponent->GetSkeletalMeshAsset();
		USkeletalMesh* TargetSkelMesh = TargetMeshComponent->GetSkeletalMeshAsset();

		if (IsValid(SourceSkelMesh) && !SourceSkelMesh->HasAnyFlags(RF_NeedPostLoad) &&
			IsValid(TargetSkelMesh) && !TargetSkelMesh->HasAnyFlags(RF_NeedPostLoad))
		{
			CurrentlyUsedSourceMeshComponent = NewSourceMeshComponent;
			CurrentlyUsedSourceMesh = SourceSkelMesh;
			CurrentlyUsedTargetMesh = TargetSkelMesh;

			for (int32 MappingIdx = 0; MappingIdx < BoneMapping.Num(); ++MappingIdx)
			{
				FCitySample_BoneSourceSettings SourceSettings;

				int32 TargetBoneIdx = TargetSkelMesh->GetRefSkeleton().FindBoneIndex(BoneMapping[MappingIdx].TargetBoneName);

				for(int32 SourceIdx = 0; SourceIdx < BoneMapping[MappingIdx].SourceBoneNames.Num(); ++SourceIdx)
				{
					int32 SourceBoneIdx = SourceSkelMesh->GetRefSkeleton().FindBoneIndex(BoneMapping[MappingIdx].SourceBoneNames[SourceIdx]);

					if (SourceBoneIdx != INDEX_NONE)
					{
						SourceSettings.SourceBoneIndices.Add(SourceBoneIdx);
					}
				}
				SourceSettings.bIncludeFullTransform = BoneMapping[MappingIdx].bIncludeFullTransform;
				SourceSettings.bIncludeInputPose = BoneMapping[MappingIdx].bIncludeInputPose;

				if ((SourceSettings.SourceBoneIndices.Num() > 0) && (TargetBoneIdx != INDEX_NONE))
				{
					BoneMapToSourceSettings.Add(TargetBoneIdx, SourceSettings);
				}
			}

			if (bCopyCurves)
			{
				USkeleton* SourceSkeleton = SourceSkelMesh->GetSkeleton();
				USkeleton* TargetSkeleton = TargetSkelMesh->GetSkeleton();

				// you shouldn't be here if this happened
				if (ensureMsgf(SourceSkeleton, TEXT("Invalid null source skeleton : %s"), *GetNameSafe(SourceSkelMesh))
					&& ensureMsgf(TargetSkeleton, TEXT("Invalid null target skeleton : %s"), *GetNameSafe(TargetSkelMesh)))
				{
					const FSmartNameMapping* SourceContainer = SourceSkeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
					const FSmartNameMapping* TargetContainer = TargetSkeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);

					TArray<FName> SourceCurveNames;
					SourceContainer->FillNameArray(SourceCurveNames);
					for (int32 Index = 0; Index < SourceCurveNames.Num(); ++Index)
					{
						SmartName::UID_Type UID = TargetContainer->FindUID(SourceCurveNames[Index]);
						if (UID != SmartName::MaxUID)
						{
							// has a valid UID, add to the list
							SmartName::UID_Type& Value = CurveNameToUIDMap.Add(SourceCurveNames[Index]);
							Value = UID;
						}
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE // CitySampleAnimNode_CopyPoseRotations