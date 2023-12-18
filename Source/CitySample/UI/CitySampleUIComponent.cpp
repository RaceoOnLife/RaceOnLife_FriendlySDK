// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/CitySampleUIComponent.h"

#include "GameFramework/PlayerInput.h"

#include "Game/CitySampleGameState.h"
#include "EnhancedInputSubsystems.h"

#include "UI/CitySampleMenu.h"
#include "UI/CitySampleControlsOverlayInterface.h"
#include "UI/CitySampleControlsOverlay.h"

#include "Game/CitySamplePlayerController.h"
#include "Game/CitySampleInteractionComponent.h"


UCitySampleUIComponent::UCitySampleUIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;

	bOptionsMenuEnabled = true;

	bOptionsMenuActive = false;
	bControlsOverlayActive = false;
	bInteractPromptActive = false;

	InputMappingPriority = 2;

	ControlsFlavor = ECitySampleControlsFlavor::Gamepad;
}

void UCitySampleUIComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Let BP handle any extra initialization
	OnInitialize();
}

void UCitySampleUIComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACitySamplePlayerController* Owner = GetOwner<ACitySamplePlayerController>())
	{
		if (Owner->IsLocalPlayerController() && Owner->Player)
		{
			if (BasePanelClass)
			{
				// Create and immediately display the base panel
				BasePanel = CreateWidget<UCitySamplePanel>(Owner, BasePanelClass, TEXT("BasePanel"));

				if (BasePanel)
				{
					BasePanel->AddToViewport();
					// Let BP handle extra UI changes
					OnBasePanelAdded();
				}

				// Create all default UI widgets/panels, if possible

				if (ControlsOverlayClass)
				{
					ControlsOverlay = CreateWidget<UCitySampleControlsOverlay>(Owner, ControlsOverlayClass, TEXT("ControlsOverlay"));
				}

				if (OptionsMenuClass)
				{
					OptionsMenu = CreateWidget<UCitySampleMenu>(Owner, OptionsMenuClass, TEXT("OptionsMenu"));

					if (ControlsOverlay)
					{
						OptionsMenu->OnTabChangedEvent.AddUObject(ControlsOverlay, &UCitySampleControlsOverlay::SetControlsDirty);
					}
				}

				if (PromptWidgetClass)
				{
					PromptWidget = CreateWidget(Owner, PromptWidgetClass, TEXT("Prompt"));
				}

				// Let BP handle extra UI changes
				OnCitySamplePanelsCreated();

				// Initialize the platform flavor
				ControlsFlavor = ECitySampleControlsFlavor::Gamepad;
				ControlsFlavorChangedEvent.Broadcast(ControlsFlavor);
			}

			// Force initial update to UI based on initial pawn, bind to APlayspacePlayerController::NotifyPawnChanged 
			OnPawnChanged(Owner, nullptr, Owner->GetPawn());
			Owner->NotifyPawnChanged.AddUObject(this, &UCitySampleUIComponent::OnPawnChanged);
		}
	}
	else
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("CitySample UI component owner must be an APlayerController!"));
	}
}

void UCitySampleUIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateUI(DeltaTime);
}

void UCitySampleUIComponent::UpdateUI(const float DeltaTime/*=0.0f*/)
{
	if (BasePanel)
	{
		BasePanel->UpdatePanel(DeltaTime, this);
	}

	// Let BP handle any extra updates
	ReceiveUpdateUI(DeltaTime);
}

void UCitySampleUIComponent::HideUI(const bool bShouldBeHidden/*=true*/)
{
	if (BasePanel)
	{
		BasePanel->SetAllPanelsHidden(bShouldBeHidden);

		OnHideUI.Broadcast(IsHidingUI());
	}
}

bool UCitySampleUIComponent::ProcessInputBool(const ECitySamplePanelInput& Input, const bool ActionValue, const float ElapsedTime)
{
	if (BasePanel)
	{
		return BasePanel->ProcessInput(Input, FVector2D(ActionValue, 0.0f), ElapsedTime);
	}

	return false;
}

bool UCitySampleUIComponent::ProcessInputAxis1D(const ECitySamplePanelInput& Input, const float ActionValue, const float ElapsedTime/*=0*/)
{
	if (BasePanel)
	{
		return BasePanel->ProcessInput(Input, FVector2D(ActionValue, 0.0f), ElapsedTime);
	}

	return false;
}

bool UCitySampleUIComponent::ProcessInputAxis2D(const ECitySamplePanelInput& Input, const FVector2D ActionValue, const float ElapsedTime/*=0*/)
{
	if (BasePanel)
	{
		return BasePanel->ProcessInput(Input, ActionValue, ElapsedTime);
	}

	return false;
}

void UCitySampleUIComponent::SetOptionsMenuEnabled(const bool bEnabled)
{
	bOptionsMenuEnabled = bEnabled;

	if (IsOptionsMenuActive())
	{
		SetOptionsMenuActive(false);
	}
}

bool UCitySampleUIComponent::SetOptionsMenuActive(const bool bSetActive)
{
	if (BasePanel && OptionsMenu)
	{
		if (bSetActive)
		{
			if (bOptionsMenuEnabled)
			{
				BasePanel->AddChildPanel(OptionsMenu);
				AddInGameInputContext();
				bOptionsMenuActive = true;
				ReceiveOptionsMenuOpen(OptionsMenu);
				OnOptionsMenuOpen.Broadcast(this, OptionsMenu);
			}
		}
		else
		{
			RemoveInGameInputContext();
			bOptionsMenuActive = false;
			BasePanel->RemoveChildPanel(OptionsMenu);
			ReceiveOptionsMenuClose(OptionsMenu);
			OnOptionsMenuClose.Broadcast(this, OptionsMenu);
		}
	}
	else
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("%s failed: BasePanel or OptionsMenu widget doesn't exist!"), ANSI_TO_TCHAR(__func__));
	}

	return bOptionsMenuActive;
}

UCitySampleControlsOverlay* UCitySampleUIComponent::AddControlsOverlay()
{
	if (BasePanel && ControlsOverlay)
	{
		if (!bControlsOverlayActive)
		{
			BasePanel->AddChildPanel(ControlsOverlay);
			bControlsOverlayActive = true;
			OnAddControlsOverlay(ControlsOverlay);
		}
	}
	else
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("%s failed: BasePanel or ControlsOverlay widget doesn't exist!"), ANSI_TO_TCHAR(__func__));
	}

	return ControlsOverlay;
}

void UCitySampleUIComponent::RemoveControlsOverlay()
{
	if (BasePanel)
	{
		BasePanel->RemoveChildPanel(ControlsOverlay);
	}
	
	bControlsOverlayActive = false;
}

void UCitySampleUIComponent::RequestControlsOverlayUpdate()
{
	if (ControlsOverlay)
	{
		ControlsOverlay->SetControlsDirty();
	}
}

void UCitySampleUIComponent::AddInteractionPrompt(const UCitySampleInteractionComponent* InteractionComp)
{
	if (PromptWidget && InteractionComp->bHasVisiblePrompt)
	{
		PromptWidget->AddToViewport();
		bInteractPromptActive = true;
		OnAddInteractionPrompt(PromptWidget, InteractionComp);
	}
}

void UCitySampleUIComponent::RemoveInteractionPrompt()
{
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
	}
	
	bInteractPromptActive = false;
}

ECitySampleControlsFlavor UCitySampleUIComponent::SetControlsFlavor(const ECitySampleControlsFlavor NewControlsFlavor)
{
	if (ControlsFlavor != NewControlsFlavor)
	{
		ControlsFlavor = NewControlsFlavor;
		ControlsFlavorChangedEvent.Broadcast(NewControlsFlavor);
	}

	return ControlsFlavor;
}

void UCitySampleUIComponent::OnPawnChanged(AController* Controller, APawn* OldPawn, APawn* NewPawn)
{
	// If this fails then the Controller casts may fail and cause unexpected behavior
	check((Controller == GetOwner()) && (Controller->GetPawn() == NewPawn));

	// Let BP handle UI changes
	ReceivePawnChanged(Cast<ACitySamplePlayerController>(Controller), OldPawn, NewPawn);
}

void UCitySampleUIComponent::SetupInputBindings()
{
	if (const APlayerController* const PC = GetOwner<APlayerController>())
	{
		if (PC->IsLocalController())
		{
			if (UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(PC->InputComponent))
			{
				InputComponent->BindAction(UpAction, ETriggerEvent::Triggered, this, &ThisClass::UpActionBinding);
				InputComponent->BindAction(DownAction, ETriggerEvent::Triggered, this, &ThisClass::DownActionBinding);
				InputComponent->BindAction(LeftAction, ETriggerEvent::Triggered, this, &ThisClass::LeftActionBinding);
				InputComponent->BindAction(RightAction, ETriggerEvent::Triggered, this, &ThisClass::RightActionBinding);
				InputComponent->BindAction(TogglePrevAction, ETriggerEvent::Triggered, this, &ThisClass::TogglePrevActionBinding);
				InputComponent->BindAction(ToggleNextAction, ETriggerEvent::Triggered, this, &ThisClass::ToggleNextActionBinding);
				InputComponent->BindAction(ConfirmAction, ETriggerEvent::Triggered, this, &ThisClass::ConfirmActionBinding);
				InputComponent->BindAction(CancelAction, ETriggerEvent::Triggered, this, &ThisClass::CancelActionBinding);
				InputComponent->BindAction(HideUIAction, ETriggerEvent::Started, this, &ThisClass::HideUIActionBinding);
			}
			else
			{
				UE_LOG(LogCitySampleUI, Warning, TEXT("%s SetupInputBindings failed: input component not of type UEnhancedInputComponent."), *GetName());
			}
		}
	}
	else
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("%s SetupInputBindings failed: owner must be an APlayerController!"), *GetName());
	}
}

/*Some context on the Input related functions below:
  Whenever we add a UI-centric Input context it's safe to assume that we want to remove game-centric input contexts.
  However, when removing UI screens we can't assume we want game-centric inputs back 
  This is due to transitions between non gameplay states that are preceded by the closing of a UI Menu.*/
void UCitySampleUIComponent::AddInGameInputContext()
{
	if (ACitySamplePlayerController* const CitySamplePC = GetOwner<ACitySamplePlayerController>())
	{
		CitySamplePC->RemoveInputContext();
		CitySamplePC->RemovePawnInputContext(CitySamplePC->GetPawn());

		AddInputContext(CitySamplePC, InGameInputMappingContext);
	}
}

void UCitySampleUIComponent::RemoveInGameInputContext()
{
	if (ACitySamplePlayerController* const CitySamplePC = GetOwner<ACitySamplePlayerController>())
	{
		RemoveInputContext(CitySamplePC, InGameInputMappingContext);
	}
}

void UCitySampleUIComponent::AddInputContext(ACitySamplePlayerController* CitySamplePC, UInputMappingContext* InputMappingContext)
{
	if (ULocalPlayer* LocalPlayer = CitySamplePC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(InputMappingContext, InputMappingPriority);
		}
	}
}

void UCitySampleUIComponent::RemoveInputContext(ACitySamplePlayerController* CitySamplePC, UInputMappingContext* InputMappingContext)
{
	if (ULocalPlayer* LocalPlayer = CitySamplePC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}
}

void UCitySampleUIComponent::UpActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::Up, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::DownActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::Down, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::LeftActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::Left, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::RightActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::Right, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::TogglePrevActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::TogglePrev, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::ToggleNextActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::ToggleNext, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::ConfirmActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::Confirm, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::CancelActionBinding(const FInputActionInstance& ActionInstance)
{
	ProcessInputBool(ECitySamplePanelInput::Reset, ActionInstance.GetValue().Get<bool>(), ActionInstance.GetElapsedTime());
}

void UCitySampleUIComponent::HideUIActionBinding()
{
	if (BasePanel != nullptr)
	{
		HideUI(!BasePanel->IsHidingAllPanels());
	}
}

