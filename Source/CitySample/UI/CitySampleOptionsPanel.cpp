// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/CitySampleOptionsPanel.h"

#include "Components/ScrollBox.h"

#include "UI/CitySampleButtonPrompt.h"


UCitySampleOptionsPanel::UCitySampleOptionsPanel()
{
	InitialOptionIndex = INDEX_NONE;
	CurrentOptionIndex = INDEX_NONE;
	bWrapAround = true;
}

void UCitySampleOptionsPanel::NativeOnInitialized()
{
	InitOptionsPanel();

	// Calls BP event OnInitialized after native initialization
	Super::NativeOnInitialized();
}

void UCitySampleOptionsPanel::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (IsDesignTime())
	{
		InitOptionsPanel();
	}
}

void InitOptionsCache(TArray<UCitySampleButtonPrompt*>& OutOptions, const UPanelWidget* const InContainer)
{
	if (!InContainer)
	{
		return;
	}

	for (UWidget* const Child : InContainer->GetAllChildren())
	{
		if (UCitySampleButtonPrompt* const OptionSelector = Cast<UCitySampleButtonPrompt>(Child))
		{
			OutOptions.Add(OptionSelector);
		}
		else
		{
			InitOptionsCache(OutOptions, Cast<UPanelWidget>(Child));
		}
	}
}

void UCitySampleOptionsPanel::InitOptionsPanel()
{
	// Rebuild cached options
	Options.Empty();
	InitOptionsCache(Options, OptionsScroll);

	// Clamps initial option index within the valid index range
	InitialOptionIndex = !Options.IsEmpty() ? FMath::Clamp(InitialOptionIndex, 0, Options.Num() - 1) : INDEX_NONE;

	// If the initial option index is valid
	if (Options.IsValidIndex(InitialOptionIndex))
	{
		// Set the current option to the option at the initial option index
		SetCurrentOptionByIndexChecked(InitialOptionIndex);
	}
	else
	{
		ClearCurrentOption();
	}
}

UCitySampleButtonPrompt* UCitySampleOptionsPanel::SetCurrentOptionByIndexChecked(const int32 Index)
{
	if (ensure(Options.IsValidIndex(Index)))
	{
		UCitySampleButtonPrompt* const PreviousOption = GetCurrentOption();
		CurrentOptionIndex = Index;
		UCitySampleButtonPrompt* const CurrentOption = GetCurrentOption();

		// Let BP handle updates related to switching the currently selected option
		OnSetCurrentOption(PreviousOption, CurrentOption);
		return CurrentOption;
	}
	else
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("%s SetOptionByIndex failed: %d is an invalid index."), *GetName(), Index);
		return GetCurrentOption();
	}
}

UCitySampleButtonPrompt* UCitySampleOptionsPanel::SetCurrentOptionByIndexClamped(const int32 Index)
{
	return !Options.IsEmpty() ? SetCurrentOptionByIndexChecked(FMath::Clamp(Index, 0, Options.Num() - 1)) : nullptr;
}

UCitySampleButtonPrompt* UCitySampleOptionsPanel::SetCurrentOption(UCitySampleButtonPrompt* const Option)
{
	if (Option == nullptr)
	{
		ClearCurrentOption();
		return nullptr;
	}
	else
	{
		const int32 Index = Options.IndexOfByKey(Option);
		return (Index != INDEX_NONE) ? SetCurrentOptionByIndexChecked(Index) : GetCurrentOption();
	}
}

UCitySampleButtonPrompt* UCitySampleOptionsPanel::IncrementCurrentOption()
{
	int32 NewOptionIndex = CurrentOptionIndex + 1;

	if (NewOptionIndex >= Options.Num() && bWrapAround)
	{
		// Wrap new option index to first valid option
		NewOptionIndex = 0;
	}

	return SetCurrentOptionByIndexClamped(NewOptionIndex);
}

UCitySampleButtonPrompt* UCitySampleOptionsPanel::DecrementCurrentOption()
{
	int32 NewOptionIndex = CurrentOptionIndex - 1;

	if (NewOptionIndex < 0 && bWrapAround)
	{
		// Wrap new option index to last valid option
		NewOptionIndex = Options.Num() - 1;
	}

	return SetCurrentOptionByIndexClamped(NewOptionIndex);
}

void UCitySampleOptionsPanel::ClearCurrentOption()
{
	UCitySampleButtonPrompt* const CurrentOption = GetCurrentOption();
	CurrentOptionIndex = INDEX_NONE;
	OnSetCurrentOption(CurrentOption, nullptr);
}
