// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CitySampleControlsOverlay.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerInput.h"

#include "EnhancedInputSubsystemInterface.h"
#include "EnhancedInputSubsystems.h"

#include "Game/CitySamplePlayerController.h"
#include "Game/CitySampleGameState.h"
#include "UI/CitySampleButtonPrompt.h"
#include "UI/CitySampleControlsOverlayInterface.h"
#include "UI/CitySampleMenu.h"
#include "UI/CitySampleUIComponent.h"


DEFINE_LOG_CATEGORY_STATIC(LogCitySampleControlsOverlay, Log, All)

void FCitySampleControlsOverlayButtonConfig::Configure(UCitySampleButtonPrompt* const Button, class UVerticalBoxSlot* const Slot/*=nullptr*/, const FMargin SlotPadding/*=FMargin()*/) const
{
	// Set the button controls flavor image config mappings and configure the image, if applicable
	Button->ConfigureControlsFlavorImages(ControlsFlavorImageConfig);

	// Set the text config for the specified prompts and configure the text blocks to match
	for (const ECitySamplePromptTextType PromptType : Prompts)
	{
		Button->ConfigurePromptText(PromptType, PromptConfig);
	}

	if (Slot)
	{
		Slot->SetPadding(SlotPadding);
		Slot->SetHorizontalAlignment(SlotHorizontalAlignment);
		Slot->SetVerticalAlignment(SlotVerticalAlignment);
	}
}

void UCitySampleControlsOverlay::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	if (IsDesignTime())
	{
		// Visualize the KeyButtonConfigMap
		TArray<FKey> Keys;
		KeyButtonConfigMap.GetKeys(Keys);

		TMap<FKey, FText> TestControlsDescription;
		for (const FKey& Key : Keys)
		{
			TestControlsDescription.Add(Key, FText::FromString(Key.ToString()));
		}

		SetControlsDescription(TestControlsDescription);

		for (UCitySampleButtonPrompt* const Button : ButtonsCache)
		{
			Button->SetControlsFlavor(GetControlsFlavor());
		}
	}
}


void UCitySampleControlsOverlay::UpdatePanel(const float DeltaTime/*=0.0f*/, const UCitySampleUIComponent* const OwningCitySampleUI)
{
	if (const APlayerController* const PC = GetOwningPlayer())
	{
		if (bControlsUpdateSkipNextFrame)
		{
			// Give one frame buffer for input to be rebuilt
			bControlsUpdateSkipNextFrame = false;
		}
		else if (bPendingControlsUpdate)
		{
			// Update controls description with rebuilt input mappings
			SetControlsDescriptionFor(PC);
			bPendingControlsUpdate = false;
		}

		UpdatePressedKeys(PC->PlayerInput);
	}

	Super::UpdatePanel(DeltaTime, OwningCitySampleUI);
}

void UCitySampleControlsOverlay::SetControlsDescriptionFor(const APlayerController* const PlayerController)
{
	TMap<FKey, FText> ControlsDescription;
	TMap<UInputAction*, FText> InputActionDescriptions;

	if (PlayerController)
	{
		// Get CitySample UI Component
		const ACitySamplePlayerController* const CitySamplePlayerController = Cast<ACitySamplePlayerController>(PlayerController);
		const UCitySampleUIComponent* const CitySampleUIComponent = CitySamplePlayerController != nullptr ? CitySamplePlayerController->GetCitySampleUIComponent() : nullptr;

		// Determine if the options menu is open
		if (CitySampleUIComponent != nullptr && CitySampleUIComponent->IsOptionsMenuActive())
		{
			const UCitySampleMenu* const OptionsMenu = CitySampleUIComponent->GetOptionsMenu();
			if (OptionsMenu->Implements<UCitySampleControlsOverlayInterface>())
			{
				InputActionDescriptions.Append(ICitySampleControlsOverlayInterface::Execute_GetInputActionDescriptions(OptionsMenu));
				ControlsDescription.Append(ICitySampleControlsOverlayInterface::Execute_GetInputKeyDescriptionOverrides(OptionsMenu));
			}
		}
		else
		{
			// Initialize controls description using the PC, if it implements the controls overlay interface
			if (PlayerController->Implements<UCitySampleControlsOverlayInterface>())
			{
				InputActionDescriptions = ICitySampleControlsOverlayInterface::Execute_GetInputActionDescriptions(PlayerController);
				ControlsDescription = ICitySampleControlsOverlayInterface::Execute_GetInputKeyDescriptionOverrides(PlayerController);
			}

			// Update controls description using the pawn, if it implements the controls overlay interface
			const APawn* const Pawn = PlayerController->GetPawn();
			if (Pawn && Pawn->Implements<UCitySampleControlsOverlayInterface>())
			{
				InputActionDescriptions.Append(ICitySampleControlsOverlayInterface::Execute_GetInputActionDescriptions(Pawn));
				ControlsDescription.Append(ICitySampleControlsOverlayInterface::Execute_GetInputKeyDescriptionOverrides(Pawn));
			}
		}

		// Fill out any missing control descriptions for each key mapped to an input action with the action description
		if (const ULocalPlayer* const LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (const UEnhancedInputLocalPlayerSubsystem* PlayerInputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				for (TPair<UInputAction*, FText>& InputActionDescription : InputActionDescriptions)
				{
					ensure(InputActionDescription.Key);

					// Add each key mapped to the input action to the controls description
					const UInputAction* const InputAction = InputActionDescription.Key;
					for (const FKey& Key : PlayerInputSubsystem->QueryKeysMappedToAction(InputAction))
					{
						// If there was no override description
						if (!ControlsDescription.Contains(Key))
						{
							// Use the input action description. If the description is empty, use the name of the action
							const bool bHasDescription = !InputActionDescription.Value.IsEmpty();
							const FText& Description = bHasDescription ? InputActionDescription.Value : FText::FromName(InputAction->GetFName());
							ControlsDescription.Add(Key, Description);
						}
					}
				}
			}
		}
	}

	SetControlsDescription(ControlsDescription);
}

void UCitySampleControlsOverlay::SetControlsDescription(const TMap<FKey, FText>& ControlsDescription)
{
	check(ButtonsContainer);

	KeysCache.Empty(ControlsDescription.Num());

	int32 ButtonCacheIndex = 0;
	for (const TPair<FKey, FText>& ControlDescription : ControlsDescription)
	{
		// Skip controls without any valid description
		if (ControlDescription.Value.IsEmpty())
		{
			continue;
		}

		const FKey& Key = ControlDescription.Key;

		if (const FCitySampleControlsOverlayButtonConfig* const ButtonConfig = KeyButtonConfigMap.Find(Key))
		{
			if (ButtonsCache.IsValidIndex(ButtonCacheIndex))
			{
				if (UCitySampleButtonPrompt* const CachedButton = ButtonsCache[ButtonCacheIndex])
				{
					CachedButton->StopAllAnimations();
				}
			}
			else
			{			
				if (UCitySampleButtonPrompt* const NewButton = CreateWidget<UCitySampleButtonPrompt>(this, ButtonConfig->Class))
				{
					ButtonsCache.Add(NewButton);

					// Let BP handle the controls specific updates when the buttons change control flavors
					NewButton->OnControlsFlavorChanged().AddUObject(this, &UCitySampleControlsOverlay::NativeOnButtonControlsFlavorChanged);
				}
				else
				{
					UE_LOG(LogCitySampleControlsOverlay, Warning, TEXT("%s: Failed to create description for %s"), *GetName(), *Key.ToString());
					continue;
				}
			}

			check(ButtonsCache.IsValidIndex(ButtonCacheIndex));

			KeysCache.Add(Key);

			// Update the button config text to match the description string
			FCitySampleControlsOverlayButtonConfig UpdatedButtonConfig = *ButtonConfig; 
			UpdatedButtonConfig.PromptConfig.Text = ControlDescription.Value;

			UCitySampleButtonPrompt* const Button = ButtonsCache[ButtonCacheIndex];

			// Reposition the button at the end of the container by removing and re-adding
			// Then configure the button with the button config adjusted with the description string
			ButtonsContainer->RemoveChild(Button);
			UpdatedButtonConfig.Configure(Button, ButtonsContainer->AddChildToVerticalBox(Button), ButtonsPadding);

			// Let BP handle initial controls flavor specific updates
			OnButtonControlsFlavorChanged(Button, Button->HasControlsFlavor());

			++ButtonCacheIndex;
		}
	}

	// If there are more buttons cached than needed
	if (ButtonCacheIndex < ButtonsCache.Num())
	{
		const int32 ExcessStartIndex = ButtonCacheIndex;
		for (; ButtonCacheIndex < ButtonsCache.Num(); ++ButtonCacheIndex)
		{
			// Remove the button from the screen
			UCitySampleButtonPrompt* const Button = ButtonsCache[ButtonCacheIndex];
			check(IsValid(Button));
			Button->RemoveFromParent();
			Button->OnControlsFlavorChanged().RemoveAll(this);
		}

		// Trim the excess buttons from the cache list
		ButtonsCache.RemoveAt(ExcessStartIndex, ButtonsCache.Num() - ExcessStartIndex);
	}

	check(KeysCache.Num() == ButtonsCache.Num());

	// Let BP handle any extra changes, if needed
	OnControlsDescriptionSet(ControlsDescription);
}

void UCitySampleControlsOverlay::UpdatePressedKeys(const UPlayerInput* const PlayerInput)
{
	if (IsShowingControls())
	{
		check(KeysCache.Num() == ButtonsCache.Num());

		for (int32 Index = 0; Index < KeysCache.Num(); ++Index)
		{
			UCitySampleButtonPrompt* const Button = ButtonsCache[Index];

			// If a mapped input key was pressed
			if (IsValid(Button) && PlayerInput->WasJustPressed(KeysCache[Index]))
			{
				// Play the press animation for the button prompt
				Button->PlayPressAnimation();
			}
		}
	}

	// Let BP handle any extra work
	OnUpdatePressedKeys(PlayerInput);
}

bool UCitySampleControlsOverlay::IsShowingControls() const
{
	if (HasControls())
	{
		for (const UCitySampleButtonPrompt* const Button : ButtonsCache)
		{
			if (Button->IsVisible())
			{
				return true;
			}
		}
	}

	return false;
}

bool UCitySampleControlsOverlay::HasGamepadControls() const
{
	for (const FKey& Key : KeysCache)
	{
		if (Key.IsGamepadKey())
		{
			return true;
		}
	}

	return false;
}

bool UCitySampleControlsOverlay::HasKeyboardMouseControls() const
{
	for (const FKey& Key : KeysCache)
	{
		if (!Key.IsGamepadKey())
		{
			return true;
		}
	}

	return false;
}

bool UCitySampleControlsOverlay::ToggleControlsDisplay(const bool bSkipAnimation/*=false*/)
{
	return DismissControlsDisplay(!bControlsDisplayDismissed, bSkipAnimation);
}

bool UCitySampleControlsOverlay::DismissControlsDisplay(const bool bDismiss/*=true*/, const bool bSkipAnimation/*=false*/)
{
	return bControlsDisplayDismissed = ReceiveDismissControlsDisplay(bDismiss, bSkipAnimation);
}

void UCitySampleControlsOverlay::NativeOnButtonControlsFlavorChanged(UCitySamplePanel* const CitySamplePanel, const bool bHasControlsFlavor)
{
	if (UCitySampleButtonPrompt* const ButtonPrompt = Cast<UCitySampleButtonPrompt>(CitySamplePanel))
	{
		OnButtonControlsFlavorChanged(ButtonPrompt, bHasControlsFlavor);
	}
}


#if WITH_EDITOR
void UCitySampleControlsOverlay::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//if (PropertyName == GET_MEMBER_NAME_CHECKED(UCitySampleControlsOverlay, Property))
	//{
	//}
}
#endif // WITH_EDITOR
