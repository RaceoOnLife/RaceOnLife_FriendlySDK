// Copyright Epic Games, Inc.All Rights Reserved.

#pragma once

#include "MassActorPoolableInterface.h"

#include "CitySampleMassVehicleBase.generated.h"

class UMassTrafficVehicleComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ACitySampleMassVehicleBase : public AActor, public IMassActorPoolableInterface
{
	GENERATED_BODY()

public:
	ACitySampleMassVehicleBase(const FObjectInitializer& ObjectInitializer);

	// IMassActorPoolableInterface interface
	virtual bool CanBePooled_Implementation() override;
	virtual void PrepareForPooling_Implementation() override;
	virtual void PrepareForGame_Implementation() override;

	UFUNCTION(BlueprintCallable)
	void ApplyWheelMotionBlurNative(const TArray<UMaterialInstanceDynamic*>& MotionBlurMIDs,
		class UMassTrafficVehicleComponent* MassTrafficVehicleComponent,
		int32 WheelMotionBlurNumSpokes,
		float WheelMotionBlurStartBlurSpeed,
		float WheelMotionBlurMin,
		float WheelMotionBlurMax);

	TArray<float> CachedMotionBlurWheelAngle;
};