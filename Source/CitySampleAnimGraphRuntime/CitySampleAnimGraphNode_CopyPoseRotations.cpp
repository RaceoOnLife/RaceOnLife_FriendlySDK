// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimGraphNode_CopyPoseRotations.h"
#include "Animation/AnimAttributes.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimGraphNode_CopyPoseRotations"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UAnimGraphNode_CopyPoseRotations
UCitySampleAnimGraphNode_CopyPoseRotations::UCitySampleAnimGraphNode_CopyPoseRotations(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UCitySampleAnimGraphNode_CopyPoseRotations::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_CopyPoseRotations_Tooltip", "The Copy Pose Rotations node copies the pose rotation data from another component to this. Only works when name matches.");
}

FText UCitySampleAnimGraphNode_CopyPoseRotations::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("CopyPoseRotations", "Copy Pose Rotations");
}

void UCitySampleAnimGraphNode_CopyPoseRotations::GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const
{
	OutAttributes.Add(UE::Anim::FAttributes::Curves);
	OutAttributes.Add(UE::Anim::FAttributes::Attributes);
}

#undef LOCTEXT_NAMESPACE // CitySampleAnimGraphNode_CopyPoseRotations
