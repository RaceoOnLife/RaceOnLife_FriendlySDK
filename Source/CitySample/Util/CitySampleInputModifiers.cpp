// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleInputModifiers.h"

#include "EnhancedPlayerInput.h"


/*
 * CitySample Look Sensitivity
 */

FInputActionValue UCitySampleInputModifierLookSensitivity::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	// Don't try and scale bools
	if (ensureMsgf(CurrentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Scale modifier doesn't support boolean values.")))
	{
		return CurrentValue.Get<FInputActionValue::Axis3D>() * Scalar;
	}

	return CurrentValue;
};

/*
 * CitySample Invert Axis
 */

FInputActionValue UCitySampleInputModifierInvertAxis::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	const FInputActionValue::Axis3D RawValue = CurrentValue.Get<FInputActionValue::Axis3D>();
	return FInputActionValue::Axis3D(bX ? -RawValue.X : RawValue.X, bY ? -RawValue.Y : RawValue.Y, bZ ? -RawValue.Z : RawValue.Z);
}


FLinearColor UCitySampleInputModifierInvertAxis::GetVisualizationColor_Implementation(FInputActionValue SampleValue, FInputActionValue FinalValue) const
{
	const FInputActionValue::Axis3D Sample = SampleValue.Get<FInputActionValue::Axis3D>();
	const FInputActionValue::Axis3D Final = FinalValue.Get<FInputActionValue::Axis3D>();
	return FLinearColor(Sample.X != Final.X ? 1.f : 0.f, Sample.Y != Final.Y ? 1.f : 0.f, Sample.Z != Final.Z ? 1.f : 0.f);
}