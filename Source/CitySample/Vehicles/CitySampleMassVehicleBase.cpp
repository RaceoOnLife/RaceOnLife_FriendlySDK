// Copyright Epic Games, Inc.All Rights Reserved.

#include "CitySampleMassVehicleBase.h"
#include "MassTrafficVehicleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MassAgentComponent.h"

ACitySampleMassVehicleBase::ACitySampleMassVehicleBase(const FObjectInitializer& ObjectInitializer)
{
	CreateDefaultSubobject<UMassAgentComponent>(TEXT("MassAgent"));
}

bool ACitySampleMassVehicleBase::CanBePooled_Implementation()
{
	return true;
}

void ACitySampleMassVehicleBase::PrepareForPooling_Implementation()
{
	SetActorEnableCollision(false);
}

void ACitySampleMassVehicleBase::PrepareForGame_Implementation()
{
	SetActorEnableCollision(true);
}

void ACitySampleMassVehicleBase::ApplyWheelMotionBlurNative(const TArray<UMaterialInstanceDynamic*>& MotionBlurMIDs,
														UMassTrafficVehicleComponent* MassTrafficVehicleComponent,
														int32 WheelMotionBlurNumSpokes,
														float WheelMotionBlurStartBlurSpeed,
														float WheelMotionBlurMin,
														float WheelMotionBlurMax)
{
	const float SpokesAngle360 = 360.0f / float(WheelMotionBlurNumSpokes);
	const float SpokesAngle180 = 180.0f / float(WheelMotionBlurNumSpokes);

	if (CachedMotionBlurWheelAngle.Num() < MotionBlurMIDs.Num())
	{
		CachedMotionBlurWheelAngle.AddZeroed(MotionBlurMIDs.Num() - CachedMotionBlurWheelAngle.Num());
	}

	if (MassTrafficVehicleComponent)
	{
		for (int i = 0; i < MotionBlurMIDs.Num(); i++)
		{
			if (MassTrafficVehicleComponent->WheelAngularVelocities.IsValidIndex(i))
			{
				if (UMaterialInstanceDynamic* MID = MotionBlurMIDs[i])
				{
					const float AbsAngularVelocity = FMath::Abs(MassTrafficVehicleComponent->WheelAngularVelocities[i]);
					float WheelAngle = 0;

					const float SpokesRemainder360 = FMath::Fmod(AbsAngularVelocity, SpokesAngle360);

					const float SpokesRemainder180 = FMath::Fmod(AbsAngularVelocity, SpokesAngle180);

					if (AbsAngularVelocity > WheelMotionBlurStartBlurSpeed)
					{
						if (SpokesRemainder360 <= 0.05f * SpokesAngle360)
						{
							WheelAngle = WheelMotionBlurMin;
						}
						else if (SpokesRemainder180 < 0.05f * SpokesAngle180)
						{
							WheelAngle = WheelMotionBlurMax;
						}
						else
						{
							WheelAngle = FMath::Lerp(WheelMotionBlurMin, WheelMotionBlurMax, SpokesRemainder180 / SpokesAngle180);
						}
					}

					if (FMath::Abs(CachedMotionBlurWheelAngle[i] - WheelAngle) > KINDA_SMALL_NUMBER)
					{
						static FName NAME_Angle(TEXT("Angle"));
						MID->SetScalarParameterValue(NAME_Angle, WheelAngle);
						CachedMotionBlurWheelAngle[i] = WheelAngle;
					}
				}
			}
		}
	}
}