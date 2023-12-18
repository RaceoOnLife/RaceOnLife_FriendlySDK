// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CitySamplePanel.h"

#include "Animation/UMGSequencePlayer.h"
#include "Components/PanelWidget.h"

#include "Game/CitySamplePlayerController.h"
#include "UI/CitySampleUIComponent.h"

DEFINE_LOG_CATEGORY(LogCitySampleUI);


void UCitySamplePanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UCitySampleUIComponent* const CitySampleUI = GetOwningCitySampleUIComponent())
	{
		NativeControlsFlavorChanged(CitySampleUI->GetControlsFlavor());
	}
}

void UCitySamplePanel::NativePreConstruct()
{
	if (IsDesignTime())
	{
		SetControlsFlavor(DesignerControlsFlavor);
	}

	Super::NativePreConstruct();
}

void UCitySamplePanel::NativeConstruct()
{
	if (UCitySampleUIComponent* const CitySampleUI = GetOwningCitySampleUIComponent())
	{
		const ECitySampleControlsFlavor NewControlsFlavor = CitySampleUI->GetControlsFlavor();
		if (ControlsFlavor != NewControlsFlavor)
		{
			NativeControlsFlavorChanged(NewControlsFlavor);
		}
		
		CitySampleUI->OnControlsFlavorChanged().AddUObject(this, &UCitySamplePanel::NativeControlsFlavorChanged);
	}

	Super::NativeConstruct();
}

void UCitySamplePanel::NativeDestruct()
{
	if (IsValid(Parent))
	{
		Parent->RemoveChildPanelChecked(this, true);
	}
	else
	{
		// Remove all child panels to prevent dangling parent references
		const TArray<UCitySamplePanel*> ChildPanelsCopy = ChildPanels;
		for (UCitySamplePanel* const Child : ChildPanelsCopy)
		{
			if (IsValid(Child) && Child->Parent == this)
			{
				RemoveChildPanelChecked(Child, true);
			}
		}
	}

	if (UCitySampleUIComponent* const CitySampleUI = GetOwningCitySampleUIComponent())
	{
		CitySampleUI->OnControlsFlavorChanged().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCitySamplePanel::UpdatePanel(const float DeltaTime/*=0.0f*/, const UCitySampleUIComponent* const OwningCitySampleUI/*=nullptr*/)
{
	const TArray<UCitySamplePanel*> ChildPanelsCopy = ChildPanels;
	for (UCitySamplePanel* const Child : ChildPanelsCopy)
	{
		if (IsValid(Child) && Child->Parent == this)
		{
			Child->UpdatePanel(DeltaTime, OwningCitySampleUI);
		}
	}

	// Let BP handle and updates
	ReceiveUpdatePanel(DeltaTime, OwningCitySampleUI);
}

UCitySamplePanel* UCitySamplePanel::CreateChildPanel(TSubclassOf<UCitySamplePanel> PanelClass, FName Name, UPanelWidget* ContainerWidget /*= nullptr*/, const bool bSkipAnimation/*=false*/)
{
	return AddChildPanel(CreateWidget<UCitySamplePanel>(this, PanelClass, MoveTemp(Name)), ContainerWidget, bSkipAnimation);
}

UCitySamplePanel* UCitySamplePanel::AddChildPanel(UCitySamplePanel* ChildPanel, UPanelWidget* ContainerWidget, bool bSkipAnimation)
{
	if (!ChildPanel)
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("ChildPanel is null, ignoring."));
		return nullptr;
	}

	if (ContainerWidget)
	{
		if (ContainerWidget != GetRootWidget() && !ContainerWidget->IsChildOf(this))
		{
			UE_LOG(LogCitySampleUI, Warning, 
				TEXT("AddChildPanel %s failed. %s must be a sub-widget of %s."), 
				*ChildPanel->GetName(), *ContainerWidget->GetName(), *GetName()
			);

			return nullptr;
		}

		if (!ContainerWidget->CanAddMoreChildren())
		{
			UE_LOG(LogCitySampleUI, Warning, 
				TEXT("AddChildPanel %s failed. %s cannot add more children."), 
				*ChildPanel->GetName(), *ContainerWidget->GetName()
			);

			return nullptr;
		}
	}

	if (UCitySamplePanel* PreviousParent = ChildPanel->Parent)
	{
		if (PreviousParent == this)
		{
			// The widget was already added. return it.
			// There is 2 cases where we would end up in this case:
			// 1. The Widget is already in that container and a new AddChildPanel is called again. In that case, ignore the add and return 
			//    the widget already there
			// 2. The widget was removed and re-added while the OutAnimation is running so remove is already postponed until the animation stops. 
			//    Set the PendingState to make sure the ChildPanel we be added when the animation stops. 

			if (AnimateOutPanelMapping.FindKey(ChildPanel) != nullptr)
			{
				ChildPanel->PendingStateData = { this, ContainerWidget, bSkipAnimation };
			}
			return ChildPanel;
		}
		else
		{
			// Store parameters for adding the child panel after removal from its parent
			ChildPanel->PendingStateData = { this, ContainerWidget, bSkipAnimation };

			PreviousParent->RemoveChildPanelChecked_Internal(ChildPanel, bSkipAnimation);

			// Above call recursively adds ChildPanel, so return immediately
			return ChildPanel;
		}
	}

	check(ChildPanel->Parent == nullptr && !ChildPanels.Contains(ChildPanel));

	if (ContainerWidget)
	{
		ContainerWidget->AddChild(ChildPanel);
		ChildPanel->bIsASubWidget = true;
	}
	else
	{
		ChildPanel->AddToViewport();
		ActivePanel = ChildPanel;
		ensure(!bIsASubWidget);
	}

	ChildPanel->Parent = this;
	ChildPanels.Add(ChildPanel);

	// Let BP handle any extra work after add
	ChildPanel->NativeOnAddedToPanel();
	NativeOnAddedChildPanel(ChildPanel);

	// Hide the new child panel, if all panels are hidden
	ChildPanel->SetAllPanelsHidden(bIsHidingAllPanels);

	// If a BP subclass provides an intro animation
	if (UUMGSequencePlayer* SequencePlayer = ChildPanel->AnimateIn())
	{
		// Map child to be added to the animation sequence player
		AnimateInPanelMapping.Add(SequencePlayer, ChildPanel);
		// Bind the final steps for adding the child panel to the animation finished event
		SequencePlayer->OnSequenceFinishedPlaying().AddUObject(this, &UCitySamplePanel::OnAnimateIn);

		if (bSkipAnimation)
		{
			SequencePlayer->Stop();
		}
	}
	else
	{
		// Otherwise, add child panel immediately
		AddChildPanelHelper(ChildPanel);
	}

	return ChildPanel;
}

void UCitySamplePanel::RemoveChildPanel(UCitySamplePanel* ChildPanel, bool bSkipAnimation)
{
	if (ChildPanel && ChildPanel->Parent == this)
	{
		RemoveChildPanelChecked(ChildPanel, bSkipAnimation);
	}
}

void UCitySamplePanel::RemoveChildPanelChecked(UCitySamplePanel* ChildPanel, bool bSkipAnimation)
{
	check(ChildPanel && ChildPanels.Contains(ChildPanel));

	// No longer pending add, as the child panel is being removed explicitly
	ChildPanel->PendingStateData = FCitySamplePanelTransitionState();

	RemoveChildPanelChecked_Internal(ChildPanel, bSkipAnimation);
}

void UCitySamplePanel::SetAllPanelsHidden(const bool bShouldBeHidden/*=true*/)
{
	// Iterate through all child panels
	for (UCitySamplePanel* Child : ChildPanels)
	{
		Child->SetAllPanelsHidden(bShouldBeHidden);
	}

	if (bShouldBeHidden && !bIsHidingAllPanels)
	{
		// Cache the non-hidden visibility state
		CachedVisibility = GetVisibility();
		SetVisibility(ESlateVisibility::Hidden);
		bIsHidingAllPanels = true;
	}
	else if (!bShouldBeHidden && bIsHidingAllPanels)
	{
		SetVisibility(CachedVisibility);
		bIsHidingAllPanels = false;
	}
}

ACitySamplePlayerController* UCitySamplePanel::GetOwningCitySamplePlayer()
{
	return GetOwningPlayer<ACitySamplePlayerController>();
}

UCitySampleUIComponent* UCitySamplePanel::GetOwningCitySampleUIComponent()
{
	const ACitySamplePlayerController* const CitySamplePC = GetOwningPlayer<ACitySamplePlayerController>();
	return CitySamplePC ? CitySamplePC->GetCitySampleUIComponent() : nullptr;
}

void UCitySamplePanel::SetControlsFlavor(const ECitySampleControlsFlavor NewControlsFlavor)
{
	NativeControlsFlavorChanged(NewControlsFlavor);
}

void UCitySamplePanel::NativeControlsFlavorChanged(const ECitySampleControlsFlavor NewControlsFlavor)
{
	ControlsFlavor = NewControlsFlavor;

	// Let BP and delegates handle event
	OnControlsFlavorChanged(NewControlsFlavor);
	ControlsFlavorChangedEvent.Broadcast(this, HasControlsFlavor());
}

void UCitySamplePanel::RemoveChildPanelChecked_Internal(UCitySamplePanel* ChildPanel, bool bSkipAnimation /*= false*/)
{
	// If the child panel is currently being animated out, then skip the animation, if desired, or do nothing.
	if (UUMGSequencePlayer* const* OutroPlayer = AnimateOutPanelMapping.FindKey(ChildPanel))
	{
		if (bSkipAnimation)
		{
			// Skip animation for any remaining children first
			const TArray<UCitySamplePanel*> TempGrandchildren = ChildPanel->ChildPanels;
			for (UCitySamplePanel* Grandchild : TempGrandchildren)
			{
				ChildPanel->RemoveChildPanelChecked_Internal(Grandchild, bSkipAnimation);
			}

			(*OutroPlayer)->Stop();
		}

		return;
	}

	// If the child panel is currently being animated in, then skip the animation and finish removing child.
	if (UUMGSequencePlayer* const* IntroPlayer = AnimateInPanelMapping.FindKey(ChildPanel))
	{
		(*IntroPlayer)->Stop();
	}

	// Remove all panels beneath this node in the hierarchy, starting from the bottom
	const TArray<UCitySamplePanel*> TempGrandchildren = ChildPanel->ChildPanels;
	for (UCitySamplePanel* Grandchild : TempGrandchildren)
	{
		ChildPanel->RemoveChildPanelChecked_Internal(Grandchild, bSkipAnimation);
	}

	// Let BP handle any extra work after removal
	ChildPanel->NativeOnRemoveFromPanel();
	NativeOnRemoveChildPanel(ChildPanel);

	if (ChildPanel == ActivePanel)
	{
		// Reset the active panel to the most recently added, non-subwidget, child
		ActivePanel = GetNewActivePanel();
	}

	// If a BP subclass provided an outro animation
	if (UUMGSequencePlayer* SequencePlayer = ChildPanel->AnimateOut())
	{
		// Map child to be removed to the animation sequence player
		AnimateOutPanelMapping.Add(SequencePlayer, ChildPanel);
		// Bind the final steps for removing the child panel to the animation finished event
		SequencePlayer->OnSequenceFinishedPlaying().AddUObject(this, &UCitySamplePanel::OnAnimateOut);
		
		if (bSkipAnimation)
		{
			SequencePlayer->Stop();
		}
	}
	else
	{
		// Otherwise, remove child immediately
		RemoveChildPanelHelper(ChildPanel);
	}
}

void UCitySamplePanel::AddChildPanelHelper(UCitySamplePanel* ChildPanel)
{
	check(ChildPanels.Contains(ChildPanel));

	// Let BP handle any extra work post animation
	ChildPanel->OnFinishedAnimateIn();
	OnChildFinishedAnimateIn(ChildPanel);
}

void UCitySamplePanel::RemoveChildPanelHelper(UCitySamplePanel* ChildPanel)
{
	check(ChildPanel->Parent == this && ChildPanels.Contains(ChildPanel));

	// Remove the CitySamplePanel from the viewport or parent widget
	ChildPanel->RemoveFromParent();

	// Remove the CitySamplePanel from the list of children
	ChildPanel->Parent = nullptr;
	ChildPanels.Remove(ChildPanel);
	ChildPanel->bIsASubWidget = false;

	// Let BP handle any extra work post animation
	ChildPanel->OnFinishedAnimateOut();
	OnChildFinishedAnimateOut(ChildPanel);

	// If the CitySamplePanel is pending add after removal
	if (ChildPanel->PendingStateData.ParentPendingAdd)
	{
		// Add the CitySamplePanel to the pending parent
		const FCitySamplePanelTransitionState& Parameters = ChildPanel->PendingStateData;
		Parameters.ParentPendingAdd->AddChildPanel(ChildPanel, Parameters.ContainerWidget, Parameters.bSkipAnimation);
	}

	// Reset stored transition state data now that the transition is complete
	ChildPanel->PendingStateData = FCitySamplePanelTransitionState();
}

void UCitySamplePanel::OnAnimateIn(UUMGSequencePlayer& SequencePlayer)
{
	check(AnimateInPanelMapping.Contains(&SequencePlayer));

	UCitySamplePanel* ChildPanel = AnimateInPanelMapping[&SequencePlayer];
	AnimateInPanelMapping.Remove(&SequencePlayer);
	SequencePlayer.OnSequenceFinishedPlaying().RemoveAll(this);

	AddChildPanelHelper(ChildPanel);
}

void UCitySamplePanel::OnAnimateOut(UUMGSequencePlayer& SequencePlayer)
{
	check(AnimateOutPanelMapping.Contains(&SequencePlayer));

	UCitySamplePanel* ChildPanel = AnimateOutPanelMapping[&SequencePlayer];
	AnimateOutPanelMapping.Remove(&SequencePlayer);
	SequencePlayer.OnSequenceFinishedPlaying().RemoveAll(this);

	RemoveChildPanelHelper(ChildPanel);
}

UCitySamplePanel* UCitySamplePanel::GetNewActivePanel()
{
	// Iterate child panels in order of newest to oldest children
	for (int32 Index = ChildPanels.Num() - 1; Index >= 0; --Index)
	{
		UCitySamplePanel* ChildPanel = ChildPanels[Index];

		// Return the newest child that was added as an independent widget
		if (IsValid(ChildPanel) && !ChildPanel->bIsASubWidget && (ChildPanel != ActivePanel))
		{
			return ChildPanel;
		}
	}

	return this;
}
