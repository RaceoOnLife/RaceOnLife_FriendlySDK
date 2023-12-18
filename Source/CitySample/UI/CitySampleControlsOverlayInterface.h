// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "InputAction.h"

#include "CitySampleControlsOverlayInterface.generated.h"

/**
 * Interface for getting a controls description.
 * @see UCitySampleControlsOverlay
 */
UINTERFACE(BlueprintType)
class CITYSAMPLE_API UCitySampleControlsOverlayInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for getting a controls description.
 * @see UCitySampleControlsOverlay
 */
class CITYSAMPLE_API ICitySampleControlsOverlayInterface : public IInterface
{
	GENERATED_BODY()

public:
	/** 
	 *	Hook for derived classes to provide a description of the action 
	 *	that should be associated with a given input action.
	 *
	 *	@returns InputAction-String mappings that represent a control description.
	 * 
	 *	@note	The CitySampleUI component should default empty descriptions to the 
	 *			name of the input action when updating the controls overlay.
	 *
	 *	@see UCitySampleUIComponent::UpdateControlsOverlay, UCitySampleControlsOverlay
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Controls Overlay")
	TMap<UInputAction*, FText> GetInputActionDescriptions() const;

	/** 
	 *	Hook for derived classes to provide a description of the action 
	 *	that should be associated with a given input key.
	 *
	 *	@returns Key-String mappings that represent a control description.
	 * 
	 *	@note	Key description overrides take priority over input action descriptions.
	 *
	 *	@see UCitySampleUIComponent::UpdateControlsOverlay, UCitySampleControlsOverlay
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Controls Overlay")
	TMap<FKey, FText> GetInputKeyDescriptionOverrides() const;
};