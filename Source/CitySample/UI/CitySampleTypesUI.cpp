// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleTypesUI.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"


void FCitySampleTextConfig::Configure(UTextBlock* TextBlock) const
{
	if (TextBlock)
	{
		TextBlock->SetText(Text);
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(Color);

		const bool bCollapse = bCollapseWhenEmpty && Text.IsEmpty();
		TextBlock->SetVisibility(bCollapse ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void FCitySampleImageConfig::Configure(class UImage* Image, class UTextBlock* LabelText/*=nullptr*/) const
{
	if (Image)
	{
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(Color);

		if (LabelText)
		{
			LabelTextConfig.Configure(LabelText);
		}
	}
}