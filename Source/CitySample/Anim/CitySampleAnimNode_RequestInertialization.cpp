// Copyright Epic Games, Inc. All Rights Reserved.

#include "Anim/CitySampleAnimNode_RequestInertialization.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Animation/AnimNode_AssetPlayerBase.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimNode_RequestInertialization"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FCitySampleAnimNode_RequestInertialization

void FCitySampleAnimNode_RequestInertialization::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread);

	Super::Initialize_AnyThread(Context);
	Source.Initialize(Context);

	bReinitialized = true;
}


void FCitySampleAnimNode_RequestInertialization::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);

	Super::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);
}


void FCitySampleAnimNode_RequestInertialization::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread);

	GetEvaluateGraphExposedInputs().Execute(Context);

	if (bReinitialized)
	{
		switch (GetCondition())
		{
		case ERequestInertializationCondition::OnChange: 
			bLastInput = bInput; 
			break;
		case ERequestInertializationCondition::OnTrue:
			bLastInput = false; 
			break;
		default: break;
		}
	}
	bReinitialized = false;


	bool bRequestInertialization = false;

	switch (GetCondition())
	{
	case ERequestInertializationCondition::OnChange:
		bRequestInertialization = bInput != bLastInput;
		break;
	case ERequestInertializationCondition::OnTrue:
		bRequestInertialization = bInput;
		break;
	default:
		break;
	}

	bLastInput = bInput;

	const bool bSkipForBecomingRelevant =
		GetSkipOnBecomingRelevant() &&
		UpdateCounter.HasEverBeenUpdated() &&
		!UpdateCounter.WasSynchronizedCounter(Context.AnimInstanceProxy->GetUpdateCounter());
	UpdateCounter.SynchronizeWith(Context.AnimInstanceProxy->GetUpdateCounter());

	bRequestInertialization = bRequestInertialization && !bSkipForBecomingRelevant;

	if (bRequestInertialization)
	{
		UE::Anim::IInertializationRequester* InertializationRequester = 
			Context.GetMessage<UE::Anim::IInertializationRequester>();
		if (InertializationRequester)
		{
			InertializationRequester->RequestInertialization(BlendTime);
			InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
		}
		else
		{
			FAnimNode_Inertialization::LogRequestError(Context, Source);
		}
	}

	Source.Update(Context);

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Request Inertialization"), bRequestInertialization);
}


void FCitySampleAnimNode_RequestInertialization::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);
	Source.Evaluate(Output);
}


void FCitySampleAnimNode_RequestInertialization::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData);
	Source.GatherDebugData(DebugData);
}


ERequestInertializationCondition FCitySampleAnimNode_RequestInertialization::GetCondition() const
{
	return GET_ANIM_NODE_DATA(ERequestInertializationCondition, Condition);
}

bool FCitySampleAnimNode_RequestInertialization::GetSkipOnBecomingRelevant() const
{
	return GET_ANIM_NODE_DATA(bool, bSkipOnBecomingRelevant);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FCitySampleAnimNode_RequestInertializationOnAssetChange

void FCitySampleAnimNode_RequestInertializationOnAssetChange::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread);

	Super::Initialize_AnyThread(Context);
	Source.Initialize(Context);

	bReinitialized = true;

	// Look for an asset player in the Source link
	if (IAnimClassInterface* AnimBlueprintClass = Context.GetAnimClass())
	{
		const TArray<FStructProperty*>& AnimNodeProperties = AnimBlueprintClass->GetAnimNodeProperties();
		if (AnimNodeProperties.IsValidIndex(Source.LinkID))
		{
			FStructProperty* LinkedProperty = AnimNodeProperties[Source.LinkID];
			if (LinkedProperty->Struct->IsChildOf(FAnimNode_AssetPlayerBase::StaticStruct()))
			{
				void* LinkedNodePtr = LinkedProperty->ContainerPtrToValuePtr<void>(Context.AnimInstanceProxy->GetAnimInstanceObject());
				AssetPlayerNode = (FAnimNode_AssetPlayerBase*)LinkedNodePtr;
			}
		}
	}
}


void FCitySampleAnimNode_RequestInertializationOnAssetChange::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);

	Super::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);
}


void FCitySampleAnimNode_RequestInertializationOnAssetChange::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread);

	GetEvaluateGraphExposedInputs().Execute(Context);

	Source.Update(Context);

	bool bRequestInertialization = false;
	
	if (AssetPlayerNode != nullptr)
	{
		if (bReinitialized)
		{
			CachedAnimationAsset = AssetPlayerNode->GetAnimAsset();
		}
		bReinitialized = false;

		const UAnimationAsset* CurrentAnimationAsset = AssetPlayerNode->GetAnimAsset();

		const bool bSkipForBecomingRelevant =
			UpdateCounter.HasEverBeenUpdated() &&
			!UpdateCounter.WasSynchronizedCounter(Context.AnimInstanceProxy->GetUpdateCounter());
		UpdateCounter.SynchronizeWith(Context.AnimInstanceProxy->GetUpdateCounter());

		if (!bSkipForBecomingRelevant)
		{
			bRequestInertialization = CachedAnimationAsset != CurrentAnimationAsset && CurrentAnimationAsset;
			if (bRequestInertialization)
			{
				UE::Anim::IInertializationRequester* InertializationRequester =
					Context.GetMessage<UE::Anim::IInertializationRequester>();
				if (InertializationRequester)
				{
					InertializationRequester->RequestInertialization(BlendTime);
					InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
				}
				else
				{
					FAnimNode_Inertialization::LogRequestError(Context, Source);
				}
			}
		}

		CachedAnimationAsset = CurrentAnimationAsset;
	}
	else
	{
		FText Message = LOCTEXT(
			"RequestInertializationOnAssetChangeError",
			"No asset player node found as the source of FAnimNode_RequestInertializationOnAssetChange.");
		Context.LogMessage(EMessageSeverity::Error, Message);
	}

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Request Inertialization"), bRequestInertialization);
}


void FCitySampleAnimNode_RequestInertializationOnAssetChange::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);
	Source.Evaluate(Output);
}


void FCitySampleAnimNode_RequestInertializationOnAssetChange::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData);
	Source.GatherDebugData(DebugData);
}

#undef LOCTEXT_NAMESPACE