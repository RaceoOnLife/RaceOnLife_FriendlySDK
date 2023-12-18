// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "UI/CitySampleUIComponent.h"
#include "UI/CitySamplePanel.h"

#include "CitySampleUIComponentInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UCitySampleUIComponentInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for actors to respond to hook into UI component events to specialize the UI.
 */
class CITYSAMPLE_API ICitySampleUIComponentInterface
{
	GENERATED_BODY()

public:
	/** Hook for adding an overlay to the CitySampleUI component when a pawn is set. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	UCitySamplePanel* AddOverlay(UCitySampleUIComponent* CitySampleUI, const bool bSkipAnimation=false);
	virtual UCitySamplePanel* AddOverlay_Implementation(UCitySampleUIComponent* CitySampleUI, const bool bSkipAnimation=false);

	/** Hook for removing an overlay from the CitySampleUI component when a pawn is set. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	void RemoveOverlay(UCitySampleUIComponent* CitySampleUI, const bool bSkipAnimation=false);
	virtual void RemoveOverlay_Implementation(UCitySampleUIComponent* CitySampleUI, const bool bSkipAnimation=false);
};
