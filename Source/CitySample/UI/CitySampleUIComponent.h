// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "UI/CitySamplePanel.h"
#include "Util/ICitySampleInputInterface.h"

#include "CitySampleUIComponent.generated.h"

class UInputMappingContext;

class ACitySamplePlayerController;
class UCitySamplePanel;
class UCitySampleMenu;
class UCitySampleControlsOverlay;
class UCitySampleSandboxUtilUI;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCitySampleOnOptionsMenuOpen, UCitySampleUIComponent* const, CitySampleUI, UCitySampleMenu* const, OptionsMenu);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCitySampleOnOptionsMenuClose, UCitySampleUIComponent* const, CitySampleUI, UCitySampleMenu* const, OptionsMenu);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCitySampleOnHideUI, bool, IsHiding);


/**
 * Component for managing the UI associated with a CitySamplePlayerController.
 */
UCLASS( Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent) )
class CITYSAMPLE_API UCitySampleUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCitySampleUIComponent();

protected:
	//~ Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent Interface

	UFUNCTION(BlueprintImplementableEvent)
	void OnInitialize();

public:
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void UpdateUI(const float DeltaTime=0.0f);

	/** 
	 * Toggles whether the UI base panel and all its child panels are hidden, effectively hiding the entire UI hierarchy.
	 *
	 * @note	Calling SetVisibility of this and any child panels after being hidden will not be tracked.
	 *			When going from hidden to not hidden, this will revert the visibility of the UI to the state it was in before hiding.

	 * @see		UCitySamplePanel::SetAllPanelsHidden
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void HideUI(const bool bShouldBeHidden=true);

	/**
	 * Whether the base panel and all child panels should be hidden after a call to UCitySamplePanel::SetAllPanelsHidden.
	 * @note	This does not reflect visibility of any panels handled through means other than UCitySamplePanel::SetAllPanelsHidden.
	 */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	bool IsHidingUI()
	{
		return BasePanel && BasePanel->IsHidingAllPanels();
	}

	/** Returns the root CitySamplePanel instance. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	UCitySamplePanel* GetBasePanel() const 
	{ 
		return BasePanel; 
	}

	/** Returns the options menu CitySamplePanel instance. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	UCitySampleMenu* GetOptionsMenu() const
	{ 
		return OptionsMenu; 
	}

	/** Returns the controls overlay CitySamplePanel instance used to display context relevant input options. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	UCitySampleControlsOverlay* GetControlsOverlay() const 
	{ 
		return ControlsOverlay; 
	}

	/** Returns the interaction prompt widget. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	UUserWidget* GetInteractionPrompt() const 
	{
		return PromptWidget; 
	}

	/** Convenience method to pass input to the BasePanel, if it exists. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI", meta=(AutoCreateRefTerm="CitySample UI|Input, ActionValue"))
	bool ProcessInputBool(const ECitySamplePanelInput& Input, const bool ActionValue, const float ElapsedTime=0);

	/** Convenience method to pass input to the BasePanel, if it exists. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI", meta=(AutoCreateRefTerm="CitySample UI|Input, ActionValue"))
	bool ProcessInputAxis1D(const ECitySamplePanelInput& Input, const float ActionValue, const float ElapsedTime=0);
	
	/** Convenience method to pass input to the BasePanel, if it exists. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI", meta=(AutoCreateRefTerm="CitySample UI|Input, ActionValue"))
	bool ProcessInputAxis2D(const ECitySamplePanelInput& Input, const FVector2D ActionValue, const float ElapsedTime=0);

	/** Returns whether the options menu is displayed. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	bool IsOptionsMenuActive() const
	{
		return bOptionsMenuActive;
	}

	/** Displays the options menu, if possible. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void SetOptionsMenuEnabled(const bool bEnabled);
	
	/** Displays the options menu, if possible. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	bool SetOptionsMenuActive(const bool bSetActive);

	/** Returns whether the controls overlay is displayed. @note It may be hidden separately. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	bool IsControlsOverlayActive() const
	{
		return bControlsOverlayActive;
	}

	/** Displays the controls overlay, if possible. @note It may be hidden separately. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	UCitySampleControlsOverlay* AddControlsOverlay();

	/** Remove the controls overlay from display. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void RemoveControlsOverlay();

	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void RequestControlsOverlayUpdate();

	/** Returns whether the interaction prompt is being displayed. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	bool IsInteractPromptActive() const
	{
		return bInteractPromptActive;
	}

	/** Displays the interaction prompt, if possible. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void AddInteractionPrompt(const class UCitySampleInteractionComponent* InteractionComp);

	/** Removes the interaction prompt from display. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	void RemoveInteractionPrompt();

	/** Event delegate for when the controls flavor for the UI has changed. */
	DECLARE_EVENT_OneParam(UCitySampleUIComponent, FControlsFlavorChangedEvent, const ECitySampleControlsFlavor);
	FControlsFlavorChangedEvent& OnControlsFlavorChanged()
	{
		return ControlsFlavorChangedEvent;
	}

	/** Returns the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintPure, Category = "CitySample UI")
	ECitySampleControlsFlavor GetControlsFlavor() const
	{
		return ControlsFlavor;
	}

	/** Sets the current controls flavor of the UI, if applicable. */
	UFUNCTION(BlueprintCallable, Category = "CitySample UI")
	ECitySampleControlsFlavor SetControlsFlavor(const ECitySampleControlsFlavor NewControlsFlavor);
	
	UPROPERTY(BlueprintAssignable, Category="CitySample UI|Options Menu")
	FCitySampleOnOptionsMenuOpen OnOptionsMenuOpen;
	
	UPROPERTY(BlueprintAssignable, Category="CitySample UI|Options Menu")
	FCitySampleOnOptionsMenuClose OnOptionsMenuClose;

	UPROPERTY(BlueprintAssignable, Category="CitySample UI|Hide UI")
	FCitySampleOnHideUI OnHideUI;

protected:
	/** Updates the active UI hierarchy. Calls UpdatePanel on all child CitySample panels added to the base panel. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void ReceiveUpdateUI(const float DeltaTime=0.0f);

	/** Default class used to create the base panel on BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CitySample UI")
	TSubclassOf<UCitySamplePanel> BasePanelClass;

	/** BP event/function hook for UI changes when the base panel is displayed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void OnBasePanelAdded();

	/** Default class used to create the options menu on BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CitySample UI")
	TSubclassOf<UCitySampleMenu> OptionsMenuClass;

	/** BP event/function hook for UI changes when the options menu is displayed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void ReceiveOptionsMenuOpen(UCitySampleMenu* InOptionsMenu);
	
	/** BP event/function hook for UI changes when the options menu is removed from display. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void ReceiveOptionsMenuClose(UCitySampleMenu* InOptionsMenu);

	/** Default class used to create the controls overlay on BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CitySample UI")
	TSubclassOf<UCitySampleControlsOverlay> ControlsOverlayClass;

	/** BP event/function hook for UI changes when the controls overlay is displayed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void OnAddControlsOverlay(UCitySampleControlsOverlay* InControlsOverlay);

	/** Default class used to create the interaction prompt on BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CitySample UI")
	TSubclassOf<UUserWidget> PromptWidgetClass;

	/** BP event/function hook for UI changes when the interaction prompt is displayed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void OnAddInteractionPrompt(UUserWidget* Prompt, const UCitySampleInteractionComponent* InteractionComp);

	/** BP event/function hook for UI changes when the default CitySample panels have been created. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void OnCitySamplePanelsCreated();

	/** BP event/function hook for UI changes when a new pawn is set. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CitySample UI")
	void ReceivePawnChanged(ACitySamplePlayerController* CitySamplePC, APawn* OldPawn, APawn* InPawn);

private:
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySample UI")
	ECitySampleControlsFlavor ControlsFlavor;

	/** Event delegate that fires when the controls flavor has been changed. */
	FControlsFlavorChangedEvent ControlsFlavorChangedEvent;

	/** Handles UI changes when a new pawn is set. */
	void OnPawnChanged(AController* Controller, APawn* OldPawn, APawn* NewPawn);

	/** Root CitySamplePanel instance. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySample UI|Panels")
	UCitySamplePanel* BasePanel = nullptr;

	/** Options menu CitySamplePanel instance. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySample UI|Panels")
	UCitySampleMenu* OptionsMenu = nullptr;

	/** Controls overlay CitySamplePanel instance used to display context relevant input options. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySample UI|Panels")
	UCitySampleControlsOverlay* ControlsOverlay = nullptr;
	
	/** Interaction prompt widget. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "CitySample UI|Panels")
	UUserWidget* PromptWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Options Menu")
	bool bOptionsMenuEnabled;

	// Active UI panel flags
	bool bOptionsMenuActive;
	bool bControlsOverlayActive;
	bool bInteractPromptActive;

public:
	void SetupInputBindings();

	void AddInGameInputContext();
	void RemoveInGameInputContext();

private:
	void AddInputContext(ACitySamplePlayerController* CitySamplePC, UInputMappingContext* InputMappingContext);
	void RemoveInputContext(ACitySamplePlayerController* CitySamplePC, UInputMappingContext* InputMappingContext);

	void UpActionBinding(const struct FInputActionInstance& ActionInstance);
	void DownActionBinding(const struct FInputActionInstance& ActionInstance);
	void LeftActionBinding(const struct FInputActionInstance& ActionInstance);
	void RightActionBinding(const struct FInputActionInstance& ActionInstance);
	void TogglePrevActionBinding(const struct FInputActionInstance& ActionInstance);
	void ToggleNextActionBinding(const struct FInputActionInstance& ActionInstance);
	void ConfirmActionBinding(const struct FInputActionInstance& ActionInstance);
	void CancelActionBinding(const struct FInputActionInstance& ActionInstance);
	void HideUIActionBinding();

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input")
	class UInputMappingContext* InGameInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input")
	int32 InputMappingPriority;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* UpAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* DownAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* LeftAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* RightAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* TogglePrevAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* ToggleNextAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* ConfirmAction;

	UPROPERTY(EditDefaultsOnly, Category="CitySample UI|Input|Actions")
	class UInputAction* CancelAction;

	UPROPERTY(EditDefaultsOnly, Category = "CitySample UI|Input|Actions")
	class UInputAction* HideUIAction;
};