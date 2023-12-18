// Copyright Epic Games, Inc. All Rights Reserved.

#include "DrivableVehicleComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include "Character/CitySampleCharacter.h"
#include "Vehicles/CitySampleVehicleBase.h"


// Sets default values for this component's properties
UDrivableVehicleComponent::UDrivableVehicleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UDrivableVehicleComponent::DriverGetIn(ACitySampleCharacter* NewDriver)
{
	if (Driver == nullptr)
	{
		ACitySampleVehicleBase* const Vehicle = GetOwningVehicle();
		APlayerController* const NewDriverController = NewDriver ? NewDriver->GetController<APlayerController>() : nullptr;

		if (Vehicle && NewDriverController && Vehicle->HasAuthority())
		{
			NewDriverController->Possess(Vehicle);

			Vehicle->SetSeatOccupant(ECitySampleVehicleSeat::Driver, NewDriver);

			Driver = NewDriver;
			OnDriverEnter.Broadcast(this, Vehicle, Driver);

			return true;
		}
	}

	return false;
}

void UDrivableVehicleComponent::DriverGetOut()
{
	if (Driver != nullptr)
	{
		ACitySampleVehicleBase* const Vehicle = GetOwningVehicle();
		if (Vehicle)
		{			
			Vehicle->SetSeatOccupant(ECitySampleVehicleSeat::Driver, nullptr);

			if (APlayerController* const VehicleController = Vehicle->GetController<APlayerController>())
			{
				VehicleController->Possess(Driver);
			}

			OnDriverExit.Broadcast(this, Vehicle, Driver);
			Driver = nullptr;
		}
	}
}

// internal
ACitySampleVehicleBase* UDrivableVehicleComponent::GetOwningVehicle() const
{
	return GetOwner<ACitySampleVehicleBase>();
}
