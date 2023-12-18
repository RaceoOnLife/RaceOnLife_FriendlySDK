// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleInteractionComponent.h"

#include "Character/CitySampleCharacter.h"
#include "Game/CitySampleInteractorInterface.h"
#include "Util/CitySampleBlueprintLibrary.h"
#include "Util/CitySampleTypes.h"


// Sets default values for this component's properties
UCitySampleInteractionComponent::UCitySampleInteractionComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetGenerateOverlapEvents(false);
	BodyInstance.SetCollisionProfileNameDeferred(CitySampleColisionProfile_Interaction);
}

void UCitySampleInteractionComponent::FinishInteraction()
{
	if (CurrentInteractor != nullptr)
	{
		CurrentInteractor->FinishInteraction();
	}
}

void UCitySampleInteractionComponent::AbortInteraction()
{
	if (CurrentInteractor != nullptr)
	{
		CurrentInteractor->AbortInteraction();
	}
}

bool UCitySampleInteractionComponent::TryToLockInteraction(const TScriptInterface<ICitySampleInteractorInterface>& Interactor)
{
	const bool bCanInteract = CanInteractWith(Interactor);
	if (bCanInteract)
	{
		CurrentInteractor = Interactor;
	}

	return bCanInteract;
}

bool UCitySampleInteractionComponent::TryToReleaseInteraction(const TScriptInterface<ICitySampleInteractorInterface>& Interactor)
{
	if (Interactor != nullptr && Interactor == CurrentInteractor)
	{
		CurrentInteractor = nullptr;
		return true;
	}

	return false;
}

bool UCitySampleInteractionComponent::K2_CanInteractWith_Implementation(const TScriptInterface<ICitySampleInteractorInterface>& Interactor) const
{
	return true;
}

bool UCitySampleInteractionComponent::CanInteractWith(const TScriptInterface<ICitySampleInteractorInterface>& Interactor) const
{
	if (CurrentInteractor != nullptr)
	{
		return false;
	}

	if (Interactor->IsInteracting())
	{
		return false;
	}

	if (!bInteractable)
	{
		return false;
	}

	return K2_CanInteractWith(Interactor);
}

void UCitySampleInteractionComponent::TeleportPawnToSafePlace(const FTransform& StartingTransform, APawn* Pawn, float ActorHalfHeight)
{
	if (Pawn)
	{
		static FName NAME_Pedestrian("Pedestrian");
		static FName NAME_Traffic("Traffic");
	
		FTransform OutputTransform;

		FVector NewLocation = StartingTransform.GetTranslation();
		FRotator NewRotation = StartingTransform.GetRotation().Rotator();
		NewRotation.Roll = NewRotation.Pitch = 0.0f;

		const float LaneSearchDistance = 10000.0f;
		if (UCitySampleBlueprintLibrary::FindNearestLaneLocationByName(this, StartingTransform, NAME_Pedestrian, OutputTransform, LaneSearchDistance))
		{
			NewLocation = OutputTransform.GetTranslation();
			NewLocation.Z += ActorHalfHeight;
		}
		else if (UCitySampleBlueprintLibrary::FindNearestLaneLocationByName(this, StartingTransform, NAME_Traffic, OutputTransform, LaneSearchDistance))
		{
			NewLocation = OutputTransform.GetTranslation();
			NewLocation.Z += ActorHalfHeight;
		}
		else
		{
			if (UWorld* World = GetWorld())
			{
				if (AGameModeBase* GM = World->GetAuthGameMode())
				{
					if (AActor* PS = GM->FindPlayerStart(Pawn->GetController()))
					{
						NewLocation = PS->GetActorLocation();
					}
				}
			}
		}

		Pawn->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::ResetPhysics);
	}
}