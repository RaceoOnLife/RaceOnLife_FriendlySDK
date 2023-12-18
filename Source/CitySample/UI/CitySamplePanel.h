// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "CoreMinimal.h"

#include "CitySampleTypesUI.h"
#include "CitySamplePanel.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCitySampleUI, Log, All)

class UCitySampleUIComponent;

UENUM(BlueprintType)
enum class ECitySamplePanelInput : uint8
{
	None,
	Up,
	Down,
	Left,
	Right,
	TogglePrev,
	ToggleNext,
	Confirm,
	Reset
};

USTRUCT()
struct FCitySamplePanelTransitionState
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	UCitySamplePanel* ParentPendingAdd = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	UPanelWidget* ContainerWidget = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	bool bSkipAnimation = false;
};

/**
 * Basic UI element that provides a simple interface for hierarchically
 * adding and removing UCitySamplePanel derived BP widgets dynamically at runtime.
 */
UCLASS()
class CITYSAMPLE_API UCitySamplePanel : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// ~Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	// ~End UUserWidget Interface

	/** Virtual method for implementing panel specific updates, may be called during tick by UCitySampleUIComponent or manually, if desired. */
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	virtual void UpdatePanel(const float DeltaTime=0.0f, const UCitySampleUIComponent* const OwningCitySampleUI=nullptr);

	/** 
	 * Creates a panel of the given class type, adds it as a child panel, and adds it to the viewport, or the specified container.
	 *
	 * @param	PanelClass		Class used to create the new CitySamplePanel.
	 * @param	Name			Name for the new CitySamplePanel.
	 * @param	ContainerWidget	Widget that the CitySamplePanel will be added to as a sub-widget, optional.
	 * @param	bSkipAnimation	Whether to skip any animation, optional.
	 *
	 * @note	If no container is specified, then the CitySamplePanel will be added to the viewport directly.
	 * 
	 * @returns	Reference to the newly created and added CitySamplePanel instance.
	*/
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	UCitySamplePanel* CreateChildPanel(TSubclassOf<UCitySamplePanel> PanelClass, FName Name, UPanelWidget* ContainerWidget = nullptr, const bool bSkipAnimation=false);

	/**
	 * Adds a CitySamplePanel instance as a child panel and adds it to the viewport, or the specified container.
	 *
	 * @param	ChildPanel		CitySamplePanel instance to be added to this panel as a child.
	 * @param	ContainerWidget	Widget that the CitySamplePanel will be added to as a sub-widget, optional.
	 * @param	bSkipAnimation	Whether to skip any animation, optional.
	 *
	 * @note	If no container is specified, then the CitySamplePanel will be added to the viewport directly.
	 *
	 * @returns Reference to the newly added child CitySamplePanel instance.
	*/
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	UCitySamplePanel* AddChildPanel(UCitySamplePanel* ChildPanel, UPanelWidget* ContainerWidget = nullptr, bool bSkipAnimation = false);

	/** Explicitly removes the CitySamplePanel as a child of this one, removing it from the screen, if applicable. */
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	void RemoveChildPanel(UCitySamplePanel* ChildPanel, bool bSkipAnimation = false);

	/** 
	 * Explicitly removes the CitySamplePanel as a child of this one, removing it from the screen, if applicable.
	 * Checks that the given CitySamplePanel to be removed is not null and is a valid child.
	*/
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	void RemoveChildPanelChecked(UCitySamplePanel* ChildPanel, bool bSkipAnimation = false);

	/** Returns the parent CitySamplePanel, if this panel is a child. Otherwise, nullptr. */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	UCitySamplePanel* GetParentPanel() const 
	{ 
		return Parent; 
	}

	/** Returns the list of dynamically added CitySamplePanel instances contained by this one. */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	const TArray<UCitySamplePanel*>& GetChildPanels() const 
	{ 
		return ChildPanels; 
	}

	/** 
	 * Returns the child CitySamplePanel most recently added to the viewport, if any children were added to the viewport.
	 * Otherwise returns this, if there were none or all children were added as sub-widgets. */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	UCitySamplePanel* GetActivePanel() 
	{ 
		return !ChildPanels.IsEmpty() ? ChildPanels.Last() : this ; 
	}

	/** 
	 * Toggles whether the panel and all its child panels are hidden, effectively hiding the entire hierarchy.
	 *
	 * @note	Calling SetVisibility on the parent panel and any child panels after being hidden will not be tracked.
	 *			When going from hidden to not hidden, this will revert the visibility of the UI to the state it was in before hiding.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	void SetAllPanelsHidden(const bool bShouldBeHidden=true);

	/** 
	 * Whether this panel and all child panels should be hidden after a call to SetAllPanelsHidden.
	 * @note	This does not reflect visibility of any panels handled through means other than SetAllPanelsHidden.
	 */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	bool IsHidingAllPanels() const
	{
		return bIsHidingAllPanels;
	}

	/**
	 * Hook for a BP event/function to process an input type forwarded from the PlayerController
	 * 
	 * @param	Input Enum value representing the input event type to be processed
	 * @return	Should return true when handled and false otherwise.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "CitySamplePanel|Input", meta=(AutoCreateRefTerm="Input, ActionValue"))
	bool ProcessInput(const ECitySamplePanelInput& Input, const FVector2D& ActionValue, const float ElapsedTime=0);

	/** Returns the owning CitySamplePlayerController, if applicable. */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	class ACitySamplePlayerController* GetOwningCitySamplePlayer();

	/** Returns the owning player's CitySampleUIComponent, if applicable. */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	UCitySampleUIComponent* GetOwningCitySampleUIComponent();

	/** Returns the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintPure, Category = "CitySamplePanel")
	ECitySampleControlsFlavor GetControlsFlavor() const
	{
		return ControlsFlavor;
	}

	/** Sets the controls flavor. */
	UFUNCTION(BlueprintCallable, Category = "CitySamplePanel")
	void SetControlsFlavor(const ECitySampleControlsFlavor NewControlsFlavor);
	
	/** Whether the panel is able to be configured for the current controls flavor. */
	UFUNCTION(BlueprintPure, Category = "CitySample Button")
	virtual	bool HasControlsFlavor() const
	{
		return true;
	}

	/** Event delegate for when the controls flavor for the UI has changed. */
	DECLARE_EVENT_TwoParams(UCitySamplePanel, FCitySamplePanelControlsFlavorChangedEvent, UCitySamplePanel* const, const bool);
	FCitySamplePanelControlsFlavorChangedEvent& OnControlsFlavorChanged()
	{
		return ControlsFlavorChangedEvent;
	}

protected:
	/** Hook for a BP event/function during a call to UpdatePanel. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel")
	void ReceiveUpdatePanel(const float DeltaTime, const UCitySampleUIComponent* const OwningCitySampleUI);

	/** Hook for a BP event/function when a child panel is added to this panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel")
	void OnAddedChildPanel(UCitySamplePanel* ChildPanel);
	
	/** Hook for native function when a child panel is added to this panel */
	virtual void NativeOnAddedChildPanel(UCitySamplePanel* ChildPanel)
	{
		OnAddedChildPanel(ChildPanel);
	}
		
	/** Hook for a BP event/function when this panel is added as a child to a parent panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel")
	void OnAddedToPanel();
	
	/** Hook for a native function when this panel is added as a child to a parent panel */
	virtual void NativeOnAddedToPanel()
	{
		OnAddedToPanel();
	}

	/** Hook for a BP event/function when a child panel is about to be removed from this panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel")
	void OnRemoveChildPanel(UCitySamplePanel* ChildPanel);
	
	/** Hook for a native function when a child panel is about to be removed from this panel */
	virtual void NativeOnRemoveChildPanel(UCitySamplePanel* ChildPanel)
	{
		OnRemoveChildPanel(ChildPanel);
	}

	/** Hook for a BP event/function when this panel is about to be removed from a parent panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel")
	void OnRemoveFromPanel();
	
	/** Hook for a native function when this panel is about to be removed from a parent panel */
	virtual void NativeOnRemoveFromPanel()
	{
		OnRemoveFromPanel();
	}

	/**
	 * Hook for a BP event/function to play an intro animation
	 * 
	 * @returns	UUMGSequencePlayer for the played intro animation
	 * @note	OnAddedToPanel and OnAddChildPanel events will be bound to the animation finish event.
	 *			However, if nullptr is returned, the events will fire immediately.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel|Animation")
	UUMGSequencePlayer* AnimateIn();

	/**
	 * BP hook to handle when a child panel finishes animating in.
	 *
	 * @note	Called immediately if AnimateIn returns nullptr or the animation was skipped.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel|Animation")
	void OnChildFinishedAnimateIn(UCitySamplePanel* ChildPanel);

	/**
	 * BP hook to handle when the panel finishes animating in.
	 *
	 * @note	Called immediately if AnimateIn returns nullptr, or the animation was skipped.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel|Animation")
	void OnFinishedAnimateIn();
	
	/**
	 * Hook for a BP event/function to play an outro animation.
	 * 
	 * @note	OnRemovedFromPanel and OnChildPanelRemoved events will be bound to the animation finish event.
	 *			However, if nullptr is returned, the events will fire immediately.
	 * 
	 * @returns	UUMGSequencePlayer for the played outro animation
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel|Animation")
	UUMGSequencePlayer* AnimateOut();

	/**
	 * BP hook to handle when a child panel finishes animating out. 
	 * 
	 * @note	Called immediately if AnimateOut returns nullptr or the animation was skipped.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel|Animation")
	void OnChildFinishedAnimateOut(UCitySamplePanel* ChildPanel);

	/** 
	 * BP hook to handle when the panel finishes animating out. 
	 * 
	 * @note	Called immediately if AnimateOut returns nullptr, or the animation was skipped. 
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel|Animation")
	void OnFinishedAnimateOut();

	/** Hook for a BP event/function when the controls flavor for the UI is changed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySamplePanel")
	void OnControlsFlavorChanged(const ECitySampleControlsFlavor NewControlsFlavor);
	virtual void NativeControlsFlavorChanged(const ECitySampleControlsFlavor NewControlsFlavor);

private:
	/** Removes child from parent without invalidating stored parameters for any pending add. Meant for use internally. */
	void RemoveChildPanelChecked_Internal(UCitySamplePanel* ChildPanel, bool bSkipAnimation = false);

	void AddChildPanelHelper(UCitySamplePanel* ChildPanel);
	void RemoveChildPanelHelper(UCitySamplePanel* ChildPanel);

	/** Function to be bound to an intro animation finished event, if a sequence was provided by AnimateIn */
	void OnAnimateIn(UUMGSequencePlayer& SequencePlayer);
	/** Function to be bound to an outro animation finished event, if a sequence was provided by AnimateOut */
	void OnAnimateOut(UUMGSequencePlayer& SequencePlayer);

	UCitySamplePanel* GetNewActivePanel();

	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySamplePanel")
	ECitySampleControlsFlavor ControlsFlavor = ECitySampleControlsFlavor::Gamepad;

	/** The controls flavor applied to buttons in designer view. */
	UPROPERTY(EditAnywhere, Category = "CitySamplePanel|Debug")
	ECitySampleControlsFlavor DesignerControlsFlavor = ECitySampleControlsFlavor::Gamepad;

	/** Event delegate that fires when the controls flavor has been changed. */
	FCitySamplePanelControlsFlavorChangedEvent ControlsFlavorChangedEvent;

	/** CitySamplePanel instance that contains this one, if dynamically added using CitySamplePanel methods. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "CitySamplePanel")
	UCitySamplePanel* Parent = nullptr;

	/** List of CitySamplePanel instances this panel contains, if added dynamically using CitySamplePanel methods. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "CitySamplePanel")
	TArray<UCitySamplePanel*> ChildPanels;

	/** The most recently added FrosyPanel instance that was not added as a sub-widget, or itself. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "CitySamplePanel")
	UCitySamplePanel* ActivePanel = this;

	/** Whether the CitySamplePanel was added as a sub-widget. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "CitySamplePanel")
	bool bIsASubWidget = false;

	/** Whether the CitySamplePanel is hiding itself and all child panels. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "CitySamplePanel")
	bool bIsHidingAllPanels = false;

	/** Cached slate visibility value while the CitySamplePanel is hidden. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "CitySamplePanel")
	ESlateVisibility CachedVisibility;

	// Animation-Panel mappings for actively animated child panels
	UPROPERTY(Transient)
	TMap<UUMGSequencePlayer*, UCitySamplePanel*> AnimateInPanelMapping;
	UPROPERTY(Transient)
	TMap<UUMGSequencePlayer*, UCitySamplePanel*> AnimateOutPanelMapping;

	// Data needed to add the CitySamplePanel to a new parent after being removed from the current parent
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySamplePanel|Animation")
	FCitySamplePanelTransitionState PendingStateData;
};
