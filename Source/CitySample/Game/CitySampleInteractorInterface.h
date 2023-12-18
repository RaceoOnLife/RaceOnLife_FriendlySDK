// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "CitySampleInteractorInterface.generated.h"

class UCitySampleInteractionComponent;

UINTERFACE(Meta=(CannotImplementInterfaceInBlueprint="true"))
class CITYSAMPLE_API UCitySampleInteractorInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class CITYSAMPLE_API ICitySampleInteractorInterface : public IInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	UFUNCTION(BlueprintCallable)
	virtual void FinishInteraction() PURE_VIRTUAL(ICitySampleInteractorInterface::FinishInteraction, );

	UFUNCTION(BlueprintCallable)
	virtual void AbortInteraction() PURE_VIRTUAL(ICitySampleInteractorInterface::AbortInteraction, );

	UFUNCTION(BlueprintCallable)
	virtual class APawn* GetInteractingPawn() PURE_VIRTUAL(ICitySampleInteractorInterface::GetInteractingPawn, return nullptr; );

	UFUNCTION(BlueprintCallable)
	virtual void TryToInteract(UCitySampleInteractionComponent* Component) PURE_VIRTUAL(ICitySampleInteractorInterface::TryToInteract, );

	UFUNCTION(BlueprintCallable)
	virtual bool IsInteractingWith(UCitySampleInteractionComponent* Component) const PURE_VIRTUAL(ICitySampleInteractorInterface::IsInteractingWith, return false; );

	UFUNCTION(BlueprintCallable)
	virtual bool IsInteracting() const PURE_VIRTUAL(ICitySampleInteractorInterface::IsInteracting, return false; );  
};