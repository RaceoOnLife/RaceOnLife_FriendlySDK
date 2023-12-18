// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CitySampleMenu.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"


const FCitySampleMenuTabData FCitySampleMenuTabData::NullTab = FCitySampleMenuTabData();


UCitySampleMenu::UCitySampleMenu()
{
	bWrapAroundTabs = true;
	StartingTabIndex = 0;
	CurrentTabIndex = StartingTabIndex;
}

void UCitySampleMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (bAdjustZOrderOfTabs && !IsDesignTime())
	{
		//AUG-ABAIR: Assume Left->Right Z-Ordering / render layering by default
		for (int32 i = 0; i < Tabs.Num(); ++i)
		{
			UCanvasPanelSlot* TabCanvasSlot = Tabs[i].Frame ? Cast<UCanvasPanelSlot>(Tabs[i].Frame->Slot) : nullptr;
			if (TabCanvasSlot)
			{
				TabCanvasSlot->SetZOrder(Tabs.Num() - i - 1);
			}
		}
	}

	// Ensures the menu is initialized with the starting tab set
	SetTab(StartingTabIndex);
}

void UCitySampleMenu::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	for (FCitySampleMenuTabData& Tab : Tabs)
	{
		// Searches and sets the widget references making up each tab by name
		// #todo: This is only necessary because of an issue where references to child widgets are null on BP compile
		Tab.Selector = WidgetTree->FindWidget<UCitySampleButtonPrompt>(Tab.SelectorName);
		Tab.Frame = WidgetTree->FindWidget<UWidget>(Tab.FrameName);
		Tab.Content = WidgetTree->FindWidget<UCitySamplePanel>(Tab.ContentName);
	}

	if (IsDesignTime())
	{
		// Ensures the menu is initialized with the starting tab set
		SetTab(StartingTabIndex);
	}
}

const FCitySampleMenuTabData& UCitySampleMenu::SetTab(const int32 Index)
{
	int32 PreviousTabIndex = CurrentTabIndex;
	const FCitySampleMenuTabData PreviousTab = GetCurrentTab();
	CurrentTabIndex = !Tabs.IsEmpty() ? FMath::Clamp(Index, 0, Tabs.Num() - 1) : 0;
	const FCitySampleMenuTabData& CurrentTab = GetCurrentTab();

	if (bAdjustZOrderOfTabs && !IsDesignTime()) 
	{
		//AUG-ABAIR: If we want our tabs Z-ordered, the current is always on top, and the previous tab is always the penultimate
		TArray<UCanvasPanelSlot*> SlotsToZOrder;
		UCanvasPanelSlot* CurrentTabCanvasSlot = Tabs[CurrentTabIndex].Frame ? Cast<UCanvasPanelSlot>(Tabs[CurrentTabIndex].Frame->Slot) : nullptr;
		SlotsToZOrder.Add(CurrentTabCanvasSlot);
		UCanvasPanelSlot* PreviousTabCanvasSlot = Tabs[PreviousTabIndex].Frame ? Cast<UCanvasPanelSlot>(Tabs[PreviousTabIndex].Frame->Slot) : nullptr;
		SlotsToZOrder.Add(PreviousTabCanvasSlot);

		while(SlotsToZOrder.Num() < Tabs.Num())
		{
			UCanvasPanelSlot* NextSlot = nullptr;
			for (int32 i = 0; i < Tabs.Num(); ++i)
			{
				UCanvasPanelSlot* PotentialNextSlot = Tabs[i].Frame ? Cast<UCanvasPanelSlot>(Tabs[i].Frame->Slot) : nullptr;
				if (!SlotsToZOrder.Contains(PotentialNextSlot) && (NextSlot == nullptr || NextSlot->GetZOrder() < PotentialNextSlot->GetZOrder()))
				{
					NextSlot = PotentialNextSlot;
				}
			}

			SlotsToZOrder.Add(NextSlot);
		}

		int32 NextZOrder = Tabs.Num() - 1;
		for (UCanvasPanelSlot* SlotToOrder : SlotsToZOrder)
		{
			if(SlotToOrder)
			{
				SlotToOrder->SetZOrder(NextZOrder);
				NextZOrder--;
			}
		}
	}

	// Let BP handle changes in response to the event
	OnSetTab(PreviousTab, CurrentTab);

	OnTabChangedEvent.Broadcast();

	return CurrentTab;
}

const FCitySampleMenuTabData& UCitySampleMenu::IncrementTab()
{
	int32 NewTabIndex = CurrentTabIndex + 1;

	if (NewTabIndex >= Tabs.Num() && bWrapAroundTabs)
	{
		NewTabIndex = 0;
	}

	return SetTab(NewTabIndex);
}

const FCitySampleMenuTabData& UCitySampleMenu::DecrementTab()
{
	int32 NewTabIndex = CurrentTabIndex - 1;

	if (NewTabIndex < 0 && bWrapAroundTabs)
	{
		NewTabIndex = Tabs.Num() - 1;
	}

	return SetTab(NewTabIndex);
}