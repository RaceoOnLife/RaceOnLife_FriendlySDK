// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimGraphNode_Base.h"
#include "CitySample/Anim/CitySampleAnimNode_RequestInertialization.h"
#include "CitySampleAnimGraphNode_RequestInertialization.generated.h"


UCLASS(MinimalAPI)
class UCitySampleAnimGraphNode_RequestInertialization : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FCitySampleAnimNode_RequestInertialization Node;

	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const override;
};


UCLASS(MinimalAPI)
class UCitySampleAnimGraphNode_RequestInertializationOnAssetChange : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FCitySampleAnimNode_RequestInertializationOnAssetChange Node;

	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const override;
};
