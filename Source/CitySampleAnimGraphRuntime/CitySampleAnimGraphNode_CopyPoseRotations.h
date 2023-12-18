// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "CitySample/Anim/CitySampleAnimNode_CopyPoseRotations.h"
#include "CitySampleAnimGraphNode_CopyPoseRotations.generated.h"


UCLASS(MinimalAPI)
class UCitySampleAnimGraphNode_CopyPoseRotations : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

		UPROPERTY(EditAnywhere, Category = Settings)
		FCitySampleAnimNode_CopyPoseRotations Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual bool UsingCopyPoseFromMesh() const override { return true; }
	virtual void GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const override;
	// End of UAnimGraphNode_Base interface
};