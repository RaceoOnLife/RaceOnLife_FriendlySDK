// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CitySamplePanel.h"
#include "CitySampleTypesUI.h"
#include "Game/CitySamplePlayerController.h"
#include "CitySampleButtonPrompt.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCitySampleButtonPromptOnPressed, UCitySampleButtonPrompt* const, ButtonPrompt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCitySampleButtonPromptOnReleased, UCitySampleButtonPrompt* const, ButtonPrompt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCitySampleButtonPromptOnHighlightedEvent, UCitySampleButtonPrompt* const, ButtonPrompt, const bool, bHighlighted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCitySampleButtonPromptOnDisabledEvent, UCitySampleButtonPrompt* const, ButtonPrompt, const bool, bDisabled);


/**
 * Base class for a button with descriptive prompt text that can be pressed and/or highlighted.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleButtonPrompt : public UCitySamplePanel
{
	GENERATED_BODY()

public:
	UCitySampleButtonPrompt();

	// ~Begin UUserWidget Interface
	virtual void SynchronizeProperties() override;
	// ~End UUserWidget Interface

	// ~Begin UCitySamplePanel Interface
	/** Whether the button image is configured specifically for the current controls flavor. */
	virtual bool HasControlsFlavor() const override
	{
		return ControlsFlavorImageConfigMap.Contains(GetControlsFlavor());
	}
	// ~End UCitySamplePanel Interface
	
	/** Gets the current button image config struct. */
	UFUNCTION(BlueprintPure, Category = "CitySample Button")
	const FCitySampleImageConfig& GetImageConfig() const
	{
		return ButtonImageConfig;
	}

	/** Gets the specified prompt text config struct. */
	UFUNCTION(BlueprintPure, Category = "CitySample Button")
	const FCitySampleTextConfig& GetPromptConfig(const ECitySamplePromptTextType Type) const
	{
		check(PromptTextConfigMap.Contains(Type) && PromptTextConfigMap[Type]);
		return *PromptTextConfigMap[Type];
	}

	/** Sets the button image config and reconfigures the image to match. */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void ConfigureImage(FCitySampleImageConfig NewConfig);

	/** Sets the specified button prompt text config and reconfigures the prompt to match. */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void ConfigurePromptText(const ECitySamplePromptTextType Type, FCitySampleTextConfig NewConfig);

	/** Sets the button controls flavor image config map and reconfigures the image to match, if applicable. */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void ConfigureControlsFlavorImages(TMap<ECitySampleControlsFlavor, FCitySampleImageConfig> NewConfigs);

	/** Whether the button is currently pressed. */
	UFUNCTION(BlueprintPure, Category = "CitySample Button")
	bool IsPressed() const
	{
		return bIsPressed;
	}

	/** 
	 * Performs the press event and fires BP events and delegates and calls PlayPressAnimation. 
	 * 
	 * @note	Sets pressed state to true. 
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void PressButton();

	/**
	 * Performs the release event and fires BP events and delegates and calls PlayReleaseAnimation.
	 *
	 * @note	Sets pressed state to false.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void ReleaseButton();

	/** Sets button press state. @note Does not fire Press/Release events or delegates. */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void SetIsPressed(const bool bShouldBePressed)
	{
		bIsPressed = bShouldBePressed;
	}

	/** Whether the button is highlighted. */
	UFUNCTION(BlueprintPure, Category = "CitySample Button")
	bool IsHighlighted() const
	{
		return bIsHighlighted;
	}

	/** Sets whether the button is highlighted. */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void SetHighlighted(const bool bShouldBeHighlighted);

	/** Whether the button is disabled. */
	UFUNCTION(BlueprintPure, Category = "CitySample Button")
	bool IsDisabled() const
	{
		return bIsDisabled;
	}

	/** Sets whether the button is disabled. */
	UFUNCTION(BlueprintCallable, Category = "CitySample Button")
	void SetDisabled(const bool bShouldBeDisabled);

	/** BP hook to respond to a button press event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample Button")
	void ReceiveButtonPressEvent();

	/** BP hook to respond to a button release event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample Button")
	void ReceiveButtonReleaseEvent();

	/** BP hook to respond to a button highlight event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample Button")
	void ReceiveButtonHighlightedEvent(const bool bHighlighted);

	/** BP hook to respond to a button disabled event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample Button")
	void ReceiveButtonDisabledEvent(const bool bDisabled);

	/** BP hook to play an animation when a button is pressed. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "CitySample Button")
	void PlayPressAnimation();

	/** BP hook to play an animation when a button is released. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "CitySample Button")
	void PlayReleaseAnimation();

	/** Delegate fired when the button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "CitySample Button")
	FCitySampleButtonPromptOnPressed OnButtonPressedEvent;

	/** Delegate fired when the button is released. */
	UPROPERTY(BlueprintAssignable, Category = "CitySample Button")
	FCitySampleButtonPromptOnReleased OnButtonReleasedEvent;

	/** Delegate fired when the button highlight state changes. */
	UPROPERTY(BlueprintAssignable, Category = "CitySample Button")
	FCitySampleButtonPromptOnHighlightedEvent OnButtonHighlightedEvent;

	/** Delegate fired when the button disable state changes. */
	UPROPERTY(BlueprintAssignable, Category = "CitySample Button")
	FCitySampleButtonPromptOnDisabledEvent OnButtonDisabledEvent;

protected:	
	// Button Image

	/** Config properties applied to the bound "ButtonImage" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="ButtonImage != nullptr"), Category = "CitySample Button|Image")
	FCitySampleImageConfig ButtonImageConfig;

	/** Controls flavor specific config properties applied to the bound "ButtonImage" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="ButtonImage != nullptr"), Category = "CitySample Button|Image")
	TMap<ECitySampleControlsFlavor, FCitySampleImageConfig> ControlsFlavorImageConfigMap;

	/** Optional widget binding for an image that represents the button. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "CitySample Button|Image")
	class UImage* ButtonImage;

	// Button Prompt Text

	/** Config properties applied to the bound "PromptTextCenter" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="PromptTextCenter != nullptr"), Category = "CitySample Button|Prompt")
	FCitySampleTextConfig PromptTextConfigCenter;

	/** Config properties applied to the bound "PromptTextTop" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="PromptTextTop != nullptr"), Category = "CitySample Button|Prompt")
	FCitySampleTextConfig PromptTextConfigTop;

	/** Config properties applied to the bound "PromptTextLeft" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="PromptTextLeft != nullptr"), Category = "CitySample Button|Prompt")
	FCitySampleTextConfig PromptTextConfigLeft;

	/** Config properties applied to the bound "PromptTextRight" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="PromptTextRight != nullptr"), Category = "CitySample Button|Prompt")
	FCitySampleTextConfig PromptTextConfigRight;

	/** Config properties applied to the bound "PromptTextBottom" widget, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="PromptTextBottom != nullptr"), Category = "CitySample Button|Prompt")
	FCitySampleTextConfig PromptTextConfigBottom;

	/** Optional widget binding for a text block that can be configured. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "CitySample Button|Prompt")
	class UTextBlock* PromptTextCenter;

	/** Optional widget binding for a text block that can be configured. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "CitySample Button|Prompt")
	class UTextBlock* PromptTextTop;

	/** Optional widget binding for a text block that can be configured. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "CitySample Button|Prompt")
	class UTextBlock* PromptTextLeft;

	/** Optional widget binding for a text block that can be configured. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "CitySample Button|Prompt")
	class UTextBlock* PromptTextRight;

	/** Optional widget binding for a text block that can be configured. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category = "CitySample Button|Prompt")
	class UTextBlock* PromptTextBottom;

	// ~Begin UCitySamplePanel Interface
	virtual void NativeControlsFlavorChanged(const ECitySampleControlsFlavor NewControlsFlavor) override;
	// ~End UCitySamplePanel Interface

private:
	/** Whether the button is currently in a highlighted state. */
	UPROPERTY(EditAnywhere, Category = "CitySample Button")
	bool bIsHighlighted;

	/** Whether the button is currently disabled. */
	UPROPERTY(EditAnywhere, Category = "CitySample Button")
	bool bIsDisabled;

	/** Whether the button is currently in a pressed state. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySample Button")
	bool bIsPressed;

	/** 
	 * Whether to use a specified prompt as a label for the image.
	 * @note	This ensures the specified button prompt is configured using the image label text config.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "CitySample Button|Prompt")
	bool bUsePromptAsImageLabel;

	/** Specifies the prompt that will be used as the image label. */
	UPROPERTY(EditDefaultsOnly, meta=(EditCondition="bUsePromptAsImageLabel"), Category = "CitySample Button|Prompt")
	ECitySamplePromptTextType ImageLabelPrompt = ECitySamplePromptTextType::Center;

	/** Prompt text block look-up table. */
	const TMap<ECitySamplePromptTextType, class UTextBlock**> PromptTextMap =
	{
		{ ECitySamplePromptTextType::Center, &PromptTextCenter },
		{ ECitySamplePromptTextType::Top, &PromptTextTop },
		{ ECitySamplePromptTextType::Left, &PromptTextLeft },
		{ ECitySamplePromptTextType::Right, &PromptTextRight },
		{ ECitySamplePromptTextType::Bottom, &PromptTextBottom }
	};

	/** Prompt text config look-up table. */
	const TMap<ECitySamplePromptTextType, FCitySampleTextConfig*> PromptTextConfigMap =
	{
		{ ECitySamplePromptTextType::Center, &PromptTextConfigCenter },
		{ ECitySamplePromptTextType::Top, &PromptTextConfigTop },
		{ ECitySamplePromptTextType::Left, &PromptTextConfigLeft },
		{ ECitySamplePromptTextType::Right, &PromptTextConfigRight },
		{ ECitySamplePromptTextType::Bottom, &PromptTextConfigBottom }
	};
};
