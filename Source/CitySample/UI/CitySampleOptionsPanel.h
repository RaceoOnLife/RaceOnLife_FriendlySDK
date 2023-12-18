// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CitySamplePanel.h"
#include "CitySampleOptionsPanel.generated.h"

class UScrollBox;
class UCitySampleButtonPrompt;

/**
 * A CitySample Panel that represents a list of options that can be navigated through and set.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleOptionsPanel : public UCitySamplePanel
{
	GENERATED_BODY()
	
public:
	UCitySampleOptionsPanel();

	virtual void NativeOnInitialized() override;
	virtual void SynchronizeProperties() override;

private:
	/** Private helper used initialize the options panel according to initial properties. */
	void InitOptionsPanel();

public:
	/** Returns the currently selected option. */
	UFUNCTION(BlueprintPure, Category = "Options Panel")
	UCitySampleButtonPrompt* GetCurrentOption() const
	{
		return Options.IsValidIndex(CurrentOptionIndex) ? Options[CurrentOptionIndex] : nullptr;
	}

	/** Sets the currently selected option by index. Ensures a valid index and logs attempts with invalid indices. */
	UFUNCTION(BlueprintCallable, Category = "Options Panel")
	UCitySampleButtonPrompt* SetCurrentOptionByIndexChecked(const int32 Index);

	/** Sets the currently selected option by index. Clamps the given index to the nearest valid index. */
	UFUNCTION(BlueprintCallable, Category = "Options Panel")
	UCitySampleButtonPrompt* SetCurrentOptionByIndexClamped(const int32 Index);

	/** 
	 * Sets the currently selected option. If null, clears the selected option. 
	 * 
	 * @note	If not found in the list of options, does nothing. 
	 */
	UFUNCTION(BlueprintCallable, Category = "Options Panel")
	UCitySampleButtonPrompt* SetCurrentOption(UCitySampleButtonPrompt* const Option);

	/**
	 * Increments the current option index and calls SetCurrentOptionByIndexClamped.
	 *
	 * @note	If bWrapAround is set, this wraps around to the option at index 0 when incrementing past the last valid index.
	 */
	UFUNCTION(BlueprintCallable, Category = "Options Panel")
	UCitySampleButtonPrompt* IncrementCurrentOption();

	/**
	 * Decrements the current option index and calls SetCurrentOptionByIndexClamped.
	 *
	 * @note	If bWrapAround is set, this wraps around to the option at the last valid index when decrementing past index 0.
	 */
	UFUNCTION(BlueprintCallable, Category = "Options Panel")
	UCitySampleButtonPrompt* DecrementCurrentOption();

	/** Clears the current option, such that no option is currently selected. */
	UFUNCTION(BlueprintCallable, Category = "Options Panel")
	void ClearCurrentOption();

protected:
	// #todo:	When references to child widgets are fixed, it may be better to set the Options array from the editor.
	//			For now, options will be generated procedurally from the scroll box children.
	/** Optional scroll box widget binding used to generate the options array on initialization. */
	UPROPERTY(BlueprintReadOnly, Category = "Options Panel", meta = (BindWidgetOptional))
	UScrollBox* OptionsScroll;

	/** Array of widgets that represent the different options that can be navigated through and set. */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options Panel")
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Options Panel")
	TArray<UCitySampleButtonPrompt*> Options;

	/** Index of the option to initialize the options panel */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options Panel", meta=(ClampMin="-1"))
	int32 InitialOptionIndex;

	/** Whether IncrementCurrentOption/DecrementCurrentOption wraps around to the first/last value when exceeding valid indices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options Panel")
	bool bWrapAround;

	/** BP hook for updating the UI when the current option selection changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Options Panel")
	void OnSetCurrentOption(UCitySampleButtonPrompt* PreviousOption, UCitySampleButtonPrompt* NewOption);

private:
	/** Index of the currently selected option. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Options Panel")
	int32 CurrentOptionIndex;
};
