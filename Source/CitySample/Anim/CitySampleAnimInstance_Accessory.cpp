// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimInstance_Accessory.h"

UCitySampleAnimInstance_Accessory::UCitySampleAnimInstance_Accessory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GaitCurveName = FName(TEXT("MoveData_Gait"));
	BlockReactCurveName = FName(TEXT("LayerData_BlockReact"));
}

void UCitySampleAnimInstance_Accessory::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Accessory_NativeUpdateAnimation);

	Gait = GetCurveValue(GaitCurveName);
	BlockReact = GetCurveValue(BlockReactCurveName);
	GaitLayerOverride = Gait * -1.f;
}