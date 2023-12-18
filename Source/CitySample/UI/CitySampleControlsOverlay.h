// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CitySampleTypesUI.h"
#include "UI/CitySamplePanel.h"
#include "CitySampleControlsOverlay.generated.h"

class UCitySampleButtonPrompt;

USTRUCT(BlueprintType)
struct FCitySampleControlsOverlayButtonConfig
{
	GENERATED_BODY()

public:
	/** A button class used to create the button widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UCitySampleButtonPrompt> Class;

	/** Image configs per controls flavor that should be applied to the button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ECitySampleControlsFlavor, FCitySampleImageConfig> ControlsFlavorImageConfig;

	/** Prompt config that should be applied to the specified prompts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCitySampleTextConfig PromptConfig;

	/** Set of prompts to which the prompt config should be applied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<ECitySamplePromptTextType> Prompts = { ECitySamplePromptTextType::Right };

	/** Horizontal alignment type applied to the vertical box slot that contains the button, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EHorizontalAlignment> SlotHorizontalAlignment = HAlign_Left;
	
	/** Vertical alignment type applied to the vertical box slot that contains the button, if applicable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EVerticalAlignment> SlotVerticalAlignment = VAlign_Center;

	/** Sets the button's configs and configures the image and prompt to match. Optionally configures a vertical box slot. */
	void Configure(UCitySampleButtonPrompt* const Button, class UVerticalBoxSlot* const Slot=nullptr, const FMargin SlotPadding=FMargin()) const;
};


/**
 * CitySample Panel for overlaying context appropriate controls onto the screen and updating button prompts.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleControlsOverlay : public UCitySamplePanel
{
	GENERATED_BODY()
	
public:
	/** Calls InitializeKeyButtonPromptMapping. */
	virtual void NativePreConstruct() override;

	// ~Begin UCitySamplePanel Interface
	virtual void UpdatePanel(const float DeltaTime = 0.0f, const UCitySampleUIComponent* const OwningCitySampleUI = nullptr) override;
	// ~End UCitySamplePanel Interface

	/** Marks the controls description as needing to be updated */
	UFUNCTION(BlueprintCallable, Category = "Controls Overlay")
	void SetControlsDirty()
	{
		// Mark controls as dirty, so the controls overlay knows to be updated
		bPendingControlsUpdate = true;
		// Skip one frame before updating to ensure input is rebuilt
		bControlsUpdateSkipNextFrame = true;
	}

	/**
	 *	Generates the current controls description for the given PlayerController and updates the controls overlay.
	 *
	 *	@note	Mappings are generated using controls descriptions provided (in order of priority) by:
	 *			the Pawn, the Playspace, and the PlayerController.
	 *
	 *			Ignores anything that does not implement ICitySampleControlsInterface.
	 *
	 *	@note	Empty control descriptions mapped to a provide input key will be defaulted to the name
	 *			of the action mapped to the key, using the PlayerInput component on the PlayerController.
	 *
	 *	@see ICitySampleControlsOverlayInterface
	*/	
	UFUNCTION(BlueprintCallable, Category = "Controls Overlay")
	void SetControlsDescriptionFor(const APlayerController* const PlayerController);

	/** Updates all key mapped button prompts with the given controls description. */
	UFUNCTION(BlueprintCallable, Category = "Controls Overlay")
	void SetControlsDescription(const TMap<FKey, FText>& ControlsDescription);

	/** Hook for BP to handle when the controls description has been set. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Controls Overlay")
	void OnControlsDescriptionSet(const TMap<FKey, FText>& ControlsDescription);

	/** Sets the appropriate input type and calls PlayPressAnimation for any buttons pressed.  */
	UFUNCTION(BlueprintCallable, Category = "Controls Overlay")
	void UpdatePressedKeys(const UPlayerInput* const PlayerInput);

	/** BP hook to do any extra work after updating pressed keys. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Controls Overlay")
	void OnUpdatePressedKeys(const UPlayerInput* const PlayerInput);

	/** Returns true when controls are not toggled off and there are active control descriptions. */
	UFUNCTION(BlueprintPure, Category = "Controls")
	bool IsShowingControls() const;

	/** Returns true if there are control descriptions for any gamepad input keys. */
	UFUNCTION(BlueprintPure, Category = "Controls")
	bool HasGamepadControls() const;

	/** Returns true if there are control descriptions for any non-gamepad input keys. */
	UFUNCTION(BlueprintPure, Category = "Controls")
	bool HasKeyboardMouseControls() const;

	/** Returns true if there are any control descriptions. */
	UFUNCTION(BlueprintPure, Category = "Controls")
	bool HasControls() const
	{
		return !ButtonsCache.IsEmpty();
	}

	/** Toggles whether the controls display should be dismissed or not. */
	UFUNCTION(BlueprintCallable, Category = "Controls Overlay")
	bool ToggleControlsDisplay(const bool bSkipAnimation=false);

	/** Sets whether the controls display should be dismissed or not. */
	UFUNCTION(BlueprintCallable, Category = "Controls Overlay")
	bool DismissControlsDisplay(const bool bDismiss=true, const bool bSkipAnimation=false);

	/** BP hook to handle when the controls display is dismissed or not. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Controls Overlay")
	bool ReceiveDismissControlsDisplay(const bool bDismiss=true, const bool bSkipAnimation=false);

protected:
	/**	Input key-button button prompt config map used to generate a set of possible controls. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls Overlay")
	TMap<FKey, FCitySampleControlsOverlayButtonConfig> KeyButtonConfigMap;

	/** Padding applied to the vertical box slot that contains the button, if applicable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls Overlay")
	FMargin ButtonsPadding;

	/** Vertical box container that will be used to add buttons for the controls description. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Controls Overlay")
	class UVerticalBox* ButtonsContainer;
	
	/** Cached list of keys corresponding to the buttons created for the controls description. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Controls Overlay")
	TArray<FKey> KeysCache;

	/** 
	* Cached list of buttons created for the controls description. 
	* Existing buttons are reconfigured when a new controls description is set and excess buttons are removed.
	*/
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Controls Overlay")
	TArray<UCitySampleButtonPrompt*> ButtonsCache;

	/** Whether the controls descriptions have been toggled off. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Controls Overlay")
	bool bControlsDisplayDismissed = false;

	/** Hook for BP to handle updating when the controls flavor changes, i.e. hiding buttons without the current controls flavor configuration. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Controls Overlay")
	void OnButtonControlsFlavorChanged(UCitySampleButtonPrompt* ButtonPrompt, const bool bHasControlsFlavor);
	void NativeOnButtonControlsFlavorChanged(UCitySamplePanel* const CitySamplePanel, const bool bHasControlsFlavor);

private:
	// Whether the controls need to be updated in the next frame
	bool bPendingControlsUpdate = false;
	bool bControlsUpdateSkipNextFrame = false;

#if WITH_EDITOR
private:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
};
