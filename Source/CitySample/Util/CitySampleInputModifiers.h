// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InputModifiers.h"

#include "CitySampleInputModifiers.generated.h"


/** CitySample Look Sensitivity
 *  Based on UInputModifierScalar, scales input by a set factor per axis, based on values in CitySampleGameMode
 */
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "CitySample Look Sensitivity"))
class UCitySampleInputModifierLookSensitivity : public UInputModifier
{
	GENERATED_BODY()

	friend class UCitySampleGameInstanceBase;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;

private:
	inline static FVector Scalar = FVector(1.0f, 1.0f, 1.0f);
};


/** CitySample Global Invert Axis
 *  Based on UInputModifierNegate, inverts input per axis, based on values in CitySampleGameMode
 */
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "CitySample Invert Axis"))
class UCitySampleInputModifierInvertAxis final : public UInputModifier
{
	GENERATED_BODY()

	friend class UCitySampleGameInstanceBase;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
	virtual FLinearColor GetVisualizationColor_Implementation(FInputActionValue SampleValue, FInputActionValue FinalValue) const override;

private:
	inline static bool bX = false;
	inline static bool bY = false;
	inline static bool bZ = false;
};