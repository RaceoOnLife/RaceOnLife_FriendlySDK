// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/CitySampleHoverDrone.h"
#include "EnhancedInputSubsystemInterface.h"

void ACitySampleHoverDrone::PawnClientRestart()
{
	// Intentionally skip AHoverDronePawn::PawnClientRestart because we
	// want to control when input contexts are added and removed in CitySamplePC.
	AHoverDronePawnBase::PawnClientRestart();
}

void ACitySampleHoverDrone::AddInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface)
{
	SubsystemInterface->AddMappingContext(InputMappingContext, InputMappingPriority);
}

void ACitySampleHoverDrone::RemoveInputContext_Implementation(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface)
{
	SubsystemInterface->RemoveMappingContext(InputMappingContext);
}