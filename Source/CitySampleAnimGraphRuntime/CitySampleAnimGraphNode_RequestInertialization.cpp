// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimGraphNode_RequestInertialization.h"
#include "Animation/AnimNode_Inertialization.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimGraphNode_RequestInertialization"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UAnimGraphNode_RequestInertialization

FLinearColor UCitySampleAnimGraphNode_RequestInertialization::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.1f, 0.2f);
}

FText UCitySampleAnimGraphNode_RequestInertialization::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Request Inertialization");
}

FText UCitySampleAnimGraphNode_RequestInertialization::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Request Inertialization");
}

FText UCitySampleAnimGraphNode_RequestInertialization::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Inertialization");
}

void UCitySampleAnimGraphNode_RequestInertialization::GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const
{
	OutAttributes.Add(UE::Anim::IInertializationRequester::Attribute);
}

#undef LOCTEXT_NAMESPACE // AnimGraphNode_RequestInertialization


#define LOCTEXT_NAMESPACE "CitySampleAnimGraphNode_RequestInertializationOnAssetChange"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UAnimGraphNode_RequestInertializationOnAssetChange

FLinearColor UCitySampleAnimGraphNode_RequestInertializationOnAssetChange::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.1f, 0.2f);
}

FText UCitySampleAnimGraphNode_RequestInertializationOnAssetChange::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Request Inertialization On Asset Change");
}

FText UCitySampleAnimGraphNode_RequestInertializationOnAssetChange::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Request Inertialization On Asset Change");
}

FText UCitySampleAnimGraphNode_RequestInertializationOnAssetChange::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Inertialization");
}

void UCitySampleAnimGraphNode_RequestInertializationOnAssetChange::GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const
{
	OutAttributes.Add(UE::Anim::IInertializationRequester::Attribute);
}

#undef LOCTEXT_NAMESPACE // AnimGraphNode_RequestInertializationOnAssetChange
