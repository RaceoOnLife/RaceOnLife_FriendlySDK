// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CitySampleButtonPrompt.h"

#include "Components/Image.h"

UCitySampleButtonPrompt::UCitySampleButtonPrompt()
{
	bIsHighlighted = false;
	bIsDisabled = false;
	bIsPressed = false;

	bUsePromptAsImageLabel = false;
}

void UCitySampleButtonPrompt::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// If there is a controls specific image config
	if (FCitySampleImageConfig* const ControlsFlavorImageConfig = ControlsFlavorImageConfigMap.Find(GetControlsFlavor()))
	{
		// Set the button image config to the controls image config
		ButtonImageConfig = *ControlsFlavorImageConfig;
	}

	// Configure the button image
	ButtonImageConfig.Configure(ButtonImage);
	
	// Configure the prompt text
	for (const ECitySamplePromptTextType PromptType : TEnumRange<ECitySamplePromptTextType>())
	{
		check(PromptTextMap.Contains(PromptType) && PromptTextMap[PromptType]);
		check(PromptTextConfigMap.Contains(PromptType) && PromptTextConfigMap[PromptType]);

		if (bUsePromptAsImageLabel && PromptType == ImageLabelPrompt)
		{
			// Set the prompt text config to the button image label text config
			ConfigurePromptText(PromptType, ButtonImageConfig.LabelTextConfig);
		}
		else
		{
			PromptTextConfigMap[PromptType]->Configure(*PromptTextMap[PromptType]);
		}
	}

	// Initializes the state of the button based on the flags
	SetHighlighted(bIsHighlighted);
	SetDisabled(bIsDisabled);
}

void UCitySampleButtonPrompt::ConfigureImage(FCitySampleImageConfig NewConfig)
{
	if (ButtonImage)
	{
		// If there is a controls specific image config
		if (FCitySampleImageConfig* const ControlsFlavorImageConfig = ControlsFlavorImageConfigMap.Find(GetControlsFlavor()))
		{
			// Set the current controls image config to the new config
			*ControlsFlavorImageConfig = MoveTemp(NewConfig);
			
			// Set the button image config to the controls image config
			ButtonImageConfig = *ControlsFlavorImageConfig;
		}
		else
		{
			// Set the button image config to the new config directly
			ButtonImageConfig = MoveTemp(NewConfig);
		}

		// Configure the image
		check(PromptTextMap.Contains(ImageLabelPrompt) && PromptTextMap[ImageLabelPrompt]);
		ButtonImageConfig.Configure(ButtonImage, bUsePromptAsImageLabel ? *PromptTextMap[ImageLabelPrompt] : nullptr);
	}
}

void UCitySampleButtonPrompt::ConfigurePromptText(const ECitySamplePromptTextType Type, FCitySampleTextConfig NewConfig)
{
	check(PromptTextMap.Contains(Type) && PromptTextMap[Type]);
	check(PromptTextConfigMap.Contains(Type) && PromptTextConfigMap[Type]);

	if (UTextBlock* const PromptText = *PromptTextMap[Type])
	{
		FCitySampleTextConfig& PromptTextConfig = *PromptTextConfigMap[Type];
		PromptTextConfig = MoveTemp(NewConfig);
		PromptTextConfig.Configure(PromptText);
	}
}

void UCitySampleButtonPrompt::ConfigureControlsFlavorImages(TMap<ECitySampleControlsFlavor, FCitySampleImageConfig> NewConfigs)
{
	ControlsFlavorImageConfigMap = MoveTemp(NewConfigs);

	// If there is a controls specific image config
	if (FCitySampleImageConfig* const ControlsFlavorImageConfig = ControlsFlavorImageConfigMap.Find(GetControlsFlavor()))
	{
		// Set the button image config to the controls image config and configure the image
		ButtonImageConfig = *ControlsFlavorImageConfig;
		check(PromptTextMap.Contains(ImageLabelPrompt) && PromptTextMap[ImageLabelPrompt]);
		ButtonImageConfig.Configure(ButtonImage, bUsePromptAsImageLabel ? *PromptTextMap[ImageLabelPrompt] : nullptr);
	}
}

void UCitySampleButtonPrompt::PressButton()
{
	if (!bIsPressed)
	{
		// Sets the pressed state
		SetIsPressed(true);

		// Lets BP handle the event and play an animation
		ReceiveButtonPressEvent();
		OnButtonPressedEvent.Broadcast(this);
		PlayPressAnimation();
	}
}

void UCitySampleButtonPrompt::ReleaseButton()
{
	if (bIsPressed)
	{
		// Set the pressed state
		SetIsPressed(false);

		// Let BP handle the event and play an animation
		ReceiveButtonReleaseEvent();
		OnButtonReleasedEvent.Broadcast(this);
		PlayReleaseAnimation();
	}
}

void UCitySampleButtonPrompt::SetHighlighted(const bool bShouldBeHighlighted)
{
	bIsHighlighted = bShouldBeHighlighted;
	ReceiveButtonHighlightedEvent(bIsHighlighted);
	OnButtonHighlightedEvent.Broadcast(this, bIsHighlighted);
}

void UCitySampleButtonPrompt::SetDisabled(const bool bShouldBeDisabled)
{
	bIsDisabled = bShouldBeDisabled;
	ReceiveButtonDisabledEvent(bIsDisabled);
	OnButtonDisabledEvent.Broadcast(this, bIsDisabled);
}

void UCitySampleButtonPrompt::NativeControlsFlavorChanged(const ECitySampleControlsFlavor NewControlsFlavor)
{
	// If there is a controls specific image config
	if (FCitySampleImageConfig* const ControlsFlavorImageConfig = ControlsFlavorImageConfigMap.Find(NewControlsFlavor))
	{
		// Set the button image config to the controls image config and configure the image
		ButtonImageConfig = *ControlsFlavorImageConfig;
		check(PromptTextMap.Contains(ImageLabelPrompt) && PromptTextMap[ImageLabelPrompt]);
		ButtonImageConfig.Configure(ButtonImage, bUsePromptAsImageLabel ? *PromptTextMap[ImageLabelPrompt] : nullptr);
	}
	
	Super::NativeControlsFlavorChanged(NewControlsFlavor);
}