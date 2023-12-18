// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimGraphNode_CrowdSlopeWarping.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimGraphNode_CrowdSlopeWarping"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UAnimGraphNode_CrowdSlopeWarping

FLinearColor UCitySampleAnimGraphNode_CrowdSlopeWarping::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.1f, 0.2f);
}

FText UCitySampleAnimGraphNode_CrowdSlopeWarping::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Warps the feet to match the floor normal using per-foot traces.");
}

FText UCitySampleAnimGraphNode_CrowdSlopeWarping::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UCitySampleAnimGraphNode_CrowdSlopeWarping::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Crowd");
}

FText UCitySampleAnimGraphNode_CrowdSlopeWarping::GetControllerDescription() const
{
	return LOCTEXT("NodeTitle", "Crowd Slope Warping");
}

#undef LOCTEXT_NAMESPACE // AnimGraphNode_CrowdSlopeWarping
