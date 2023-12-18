// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UI/CitySamplePanel.h"
#include "CitySampleControlsOverlayInterface.h"

#include "CitySampleButtonPrompt.h"
#include "CitySampleMenu.generated.h"


/**
 * Represents a menu tab made up of a selector, frame, and content widgets that are relevant when a tab is active.
 */
USTRUCT(BlueprintType)
struct FCitySampleMenuTabData
{
	GENERATED_BODY()

public:
	static const FCitySampleMenuTabData NullTab;

	// #todo: remove names and name lookup to init the pointers once the references can be set directly
	UPROPERTY(EditAnywhere)
	FName SelectorName;

	UPROPERTY(EditAnywhere)
	FName FrameName;

	UPROPERTY(EditAnywhere)
	FName ContentName;

	/** A button widget that represents the tab selection, i.e. an icon in a list that highlights when active. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly)
	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCitySampleButtonPrompt* Selector = nullptr;

	/** A widget that acts as the parent for the content, i.e. an overlay, canvas panel, or border that contains the content. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly)
	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UWidget* Frame = nullptr;

	/** The CitySamplePanel widget representing the tab's content. Can be passed the input from a call to ProcessInput. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly)
	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCitySamplePanel* Content = nullptr;

	/** Mapping of relevant InputActions to their string description for the controls overlay UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<UInputAction*, FText> InputActionDescriptions;

	/** Mapping of relevant input keys to their string description for the controls overlay UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FKey, FText> InputKeyDescriptionOverrides;
};

DECLARE_EVENT(UCitySampleMenu, FMenuOnTabChangedEvent);

/**
 * CitySample Panel base for a tabbed menu, enabling navigating between tabs that optionally contain CitySamplePanel content.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleMenu : public UCitySamplePanel, public ICitySampleControlsOverlayInterface
{
	GENERATED_BODY()
	
public:
	UCitySampleMenu();

	virtual void NativeOnInitialized() override;
	virtual void SynchronizeProperties() override;

	//~ Begin ICitySampleControlsOverlayInterface Interface

	/** Returns mapping of relevant InputActions to their string description for the controls overlay UI. */
	virtual TMap<UInputAction*, FText> GetInputActionDescriptions_Implementation() const override
	{
		return GetCurrentTab().InputActionDescriptions;
	};

	/** Returns mapping of relevant input keys to their string description for the controls overlay UI. */
	virtual TMap<FKey, FText> GetInputKeyDescriptionOverrides_Implementation() const override
	{
		return GetCurrentTab().InputKeyDescriptionOverrides;
	};

	//~ End ICitySampleControlsOverlayInterface Interface

	/** 
	 * Returns the widgets that make up the current tab, some or all widget refs may be null.
	 * 
	 * @note	All widget references are guaranteed to be null if the index is invalid, i.e. when there are no tabs. 
	 */
	UFUNCTION(BlueprintPure, Category = "UI")
	const FCitySampleMenuTabData& GetCurrentTab() const
	{
		return Tabs.IsValidIndex(CurrentTabIndex) ? Tabs[CurrentTabIndex] : FCitySampleMenuTabData::NullTab;
	}

	/** Gets the current tab index. Defaults to 0, even when there are no tabs. */
	UFUNCTION(BlueprintPure, Category = "UI")
	int32 GetCurrentTabIndex() const
	{
		return CurrentTabIndex;
	}

	/** Sets the current tab based on the given index, clamped to a valid index in the tabs array, or 0 if there are none. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	const FCitySampleMenuTabData& SetTab(const int32 Index);

	/** 
	 * Increments the current tab index and sets the tab to match.
	 * 
	 * @note	If bWrapAroundTabs is set, this wraps around to the first tab when incrementing from the final tab.
	 *			Otherwise clamps to the last tab index, or 0 if there are none, and sets the tab to match, if it exists. 
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	const FCitySampleMenuTabData& IncrementTab();
	
	/**
	 * Decrements the current tab index and sets the tab to match.
	 * 
	 * @note	If bWrapAroundTabs is set, this wraps around to the last tab when decrementing from the first tab.
	 *			Otherwise clamps to index 0 and sets the current tab to match, if it exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	const FCitySampleMenuTabData& DecrementTab();

	/**Simple event for C++ classes to be notified of a tab change*/
	FMenuOnTabChangedEvent OnTabChangedEvent;

	/** Whether IncrementTab/DecrementTab should wrap around to the tab at the first/last valid index, if possible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool bWrapAroundTabs;

protected:
	/** An array of tab data, specifically an array of widget references that make up a tab (selector, frame, and content). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TArray<FCitySampleMenuTabData> Tabs;

	/** BP hook to handle UI updates when switching between tabs. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnSetTab(const FCitySampleMenuTabData& PreviousTabData, const FCitySampleMenuTabData& NewTabData);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	int32 StartingTabIndex;

	/** If true, the current tab will have ZOrder=0, the previous tab will have ZOrder=1 and so on. Currently requires tabs to be wrapped with a Canvas Panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	bool bAdjustZOrderOfTabs;

private:
	/** The index of the current tab in the tabs array. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	int32 CurrentTabIndex;
};
