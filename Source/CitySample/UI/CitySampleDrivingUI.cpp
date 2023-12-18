// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/CitySampleDrivingUI.h"
#include "VehicleUtility.h"
#include "Kismet/KismetTextLibrary.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "Kismet/GameplayStatics.h"
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#include "Vehicles/CitySampleVehicleBase.h"


const FString FCitySampleDrivingStateDescription::ParkGearDescription = TEXT("P");
const FString FCitySampleDrivingStateDescription::ReverseGearDescription = TEXT("R");
const FString FCitySampleDrivingStateDescription::NeutralGearDescription = TEXT("N");
const FString FCitySampleDrivingStateDescription::DriveGearDescription = TEXT("D");


UCitySampleDrivingUI::UCitySampleDrivingUI()
{
	TopSpeed = 200.0f;
}

void UCitySampleDrivingUI::NativeOnInitialized()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SetUpDebugUI();
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	Super::NativeOnInitialized();
}

void UCitySampleDrivingUI::NativeOnAddedToPanel()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	AddDebugUI();
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	Super::NativeOnAddedToPanel();
}

void UCitySampleDrivingUI::NativeOnRemoveFromPanel()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	RemoveDebugUI();
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	Super::NativeOnRemoveFromPanel();
}

void UCitySampleDrivingUI::UpdatePanel(const float DeltaTime /*= 0.0f*/, const UCitySampleUIComponent* const OwningCitySampleUI/*=nullptr*/)
{
	if (const APlayerController* const PC = GetOwningPlayer<APlayerController>())
	{
		// If driving a vehicle, update the driving UI with the current driving state
		if (ACitySampleVehicleBase* Vehicle = PC->GetPawn<ACitySampleVehicleBase>())
		{
			const UCitySamplePanel* const ParentPanel = GetParentPanel();
			const bool ShouldBeHidden = Vehicle->ShouldHideDrivingUI() || (ParentPanel && ParentPanel->IsHidingAllPanels());

			if (IsHidingAllPanels() != ShouldBeHidden)
			{
				SetAllPanelsHidden(ShouldBeHidden);
			}

			UpdateDrivingStateDescription(Vehicle->GetDrivingState());
		}
	}

	Super::UpdatePanel(DeltaTime, OwningCitySampleUI);
}

void UCitySampleDrivingUI::UpdateDrivingStateDescription(const FCitySampleDrivingState& DrivingState)
{
	// When not moving or speed is negative, brake is treated as reverse and not "on"
	DrivingStateDescription.bBrakeOn = (DrivingState.ForwardSpeed > KINDA_SMALL_NUMBER) && (DrivingState.Brake > KINDA_SMALL_NUMBER);
	DrivingStateDescription.bHandbrakeOn = DrivingState.bHandbrakeOn;
	DrivingStateDescription.Speed = GetSpeedDescription(DrivingState.ForwardSpeed);
	DrivingStateDescription.RPM = DrivingState.RPM;
	DrivingStateDescription.Gear = GetGearDescription(DrivingState);

	OnUpdateDrivingStateDescription(DrivingStateDescription);
}

float UCitySampleDrivingUI::GetSpeedDescription(const float Speed)
{
	float SpeedDescription = Speed;

	switch (SpeedUnits)
	{
	case ECitySampleSpeedUnits::MilesPerHour:
		SpeedDescription = Chaos::CmSToMPH(SpeedDescription);
		break;
	case ECitySampleSpeedUnits::KilometersPerHour:
		SpeedDescription = Chaos::CmSToKmH(SpeedDescription);
		break;
	case ECitySampleSpeedUnits::MetersPerSecond:
		SpeedDescription = Chaos::CmToM(SpeedDescription);
		break;

	default:
		break;
	}

	return FMath::Abs(SpeedDescription);
}

FString UCitySampleDrivingUI::GetGearDescription(const FCitySampleDrivingState& DrivingState)
{
	// Initialize gear description to the current gear, then update if necessary
	FString GearDescription = DrivingStateDescription.Gear;

	// If throttling, set driving gear
	if (DrivingState.Throttle > KINDA_SMALL_NUMBER)
	{
		if (DrivingState.bAutomatic)
		{
			GearDescription = FCitySampleDrivingStateDescription::DriveGearDescription;
		}
		else
		{
			GearDescription = FString::FromInt(DrivingState.Gear);
		}
	}
	// If reversing, set reverse gear
	else if (DrivingState.Brake > KINDA_SMALL_NUMBER && DrivingState.ForwardSpeed < KINDA_SMALL_NUMBER)
	{
		GearDescription = FCitySampleDrivingStateDescription::ReverseGearDescription;
	}
	// If there is no input, no movement, and the handbrake is on, set the gear to park
	// #todo Tolerance is increased due to non-zero speeds while seemingly still from the vehicle movement component
	else if (FMath::IsNearlyZero(DrivingState.ForwardSpeed, 0.1f))
	{
		if (DrivingState.bHandbrakeOn)
		{
			GearDescription = FCitySampleDrivingStateDescription::ParkGearDescription;
		}
	}
	// If there is no input, but there is movement, then set the gear to neutral
	else
	{
		GearDescription = FCitySampleDrivingStateDescription::NeutralGearDescription;
	}

	return GearDescription;
}

FText UCitySampleDrivingUI::FloatToDescriptionFormat(const float Float, const int32 MinDigits, const int32 MaxDigits)
{
	FNumberFormattingOptions NumberFormatOptions;
	NumberFormatOptions.AlwaysSign = false;
	NumberFormatOptions.UseGrouping = true;
	NumberFormatOptions.RoundingMode = ERoundingMode::HalfFromZero;
	NumberFormatOptions.MinimumIntegralDigits = MinDigits;
	NumberFormatOptions.MaximumIntegralDigits = MaxDigits;
	NumberFormatOptions.MinimumFractionalDigits = 0;
	NumberFormatOptions.MaximumFractionalDigits = 0;

	return FText::AsNumber(Float, &NumberFormatOptions);
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

void UCitySampleDrivingUI::SetUpDebugUI()
{
	if (VehicleCOMDebug == nullptr)
	{
		VehicleCOMDebug = CreateWidget<UCitySamplePanel>(this, VehicleCOMDebugClass, TEXT("VehicleCOMDebug"));
	}
}

void UCitySampleDrivingUI::AddDebugUI()
{
	if (VehicleCOMDebug != nullptr)
	{
		if (const APlayerController* const PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (const ACitySampleVehicleBase* Vehicle = Cast<ACitySampleVehicleBase>(PC->GetPawn()))
			{
				if (Vehicle->ShouldDrawCenterOfMassDebug())
				{
					AddChildPanel(VehicleCOMDebug);
				}
			}
		}
	}
}

void UCitySampleDrivingUI::RemoveDebugUI()
{
	if (VehicleCOMDebug != nullptr)
	{
		RemoveChildPanel(VehicleCOMDebug);
	}
}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
