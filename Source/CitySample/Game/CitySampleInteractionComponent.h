// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "CitySampleInteractionComponent.generated.h"

class ACitySampleCharacter;
class ICitySampleInteractorInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, const TScriptInterface<ICitySampleInteractorInterface>&, InteractingChar);

/**
 * Component that can be added to Actors to allow for generic interactions.
 *
 * Prompts will be shown to Players to perform the interaction when they are in range. 
 */
UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CITYSAMPLE_API UCitySampleInteractionComponent : public USphereComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCitySampleInteractionComponent(const class FObjectInitializer& ObjectInitializer);

	bool TryToLockInteraction(const TScriptInterface<ICitySampleInteractorInterface>& Interactor);
	bool TryToReleaseInteraction(const TScriptInterface<ICitySampleInteractorInterface>& Interactor);

	/**
	 * Called to abort this interaction.
	 */
	UFUNCTION(BlueprintCallable)
	void AbortInteraction();

	/**
	 * Should be called to notify the interaction has been completed.
	 */
	UFUNCTION(BlueprintCallable)
	void FinishInteraction();

	/**
	 * Notifies that the interaction has started.
	 */
 	UFUNCTION(BlueprintImplementableEvent)
 	void OnInteractionStarted(const TScriptInterface<ICitySampleInteractorInterface>& Interactor);

	/**
	 * Notifies that the interaction was aborted.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnInteractionAborted(const TScriptInterface<ICitySampleInteractorInterface>& Interactor);

	/** Notifies that the interaction was completed. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnInteractionFinished(const TScriptInterface<ICitySampleInteractorInterface>& Interactor);

	UFUNCTION(BlueprintNativeEvent, DisplayName="CanInteractWith")
	bool K2_CanInteractWith(const TScriptInterface<ICitySampleInteractorInterface>& Interactor) const;
	
	bool CanInteractWith(const TScriptInterface<ICitySampleInteractorInterface>& Interactor) const;

	UPROPERTY(BlueprintAssignable)
	FOnInteract OnInteract;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetPromptText() const { return PromptText; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetInteractable(bool bNewInteractable)
	{
		bInteractable = bNewInteractable;
	}

	UFUNCTION(BlueprintPure, Category = "UI")
	bool GetInteractable() const
	{
		return bInteractable;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bHasVisiblePrompt = true;

	UFUNCTION(BlueprintCallable)
	void TeleportPawnToSafePlace(const FTransform& StartingTransform, APawn* Pawn, float ActorHalfHeight);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText PromptText = NSLOCTEXT("CitySampleInteractionComponent", "PromptText", "Interact");

private:

	UPROPERTY(transient)
	TScriptInterface<ICitySampleInteractorInterface> CurrentInteractor;

	bool bInteractable = true;
};
