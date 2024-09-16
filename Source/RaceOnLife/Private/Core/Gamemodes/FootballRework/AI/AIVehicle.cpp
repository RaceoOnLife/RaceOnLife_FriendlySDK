


#include "Core/Gamemodes/FootballRework/AI/AIVehicle.h"

void AAIVehicle::MoveForward(float Value)
{
    if (VehicleMovementComponent)
    {
        VehicleMovementComponent->SetThrottleInput(Value);
    }
}

void AAIVehicle::MoveRight(float Value)
{
    if (VehicleMovementComponent)
    {
        VehicleMovementComponent->SetSteeringInput(Value);
    }
}

AAIVehicle::AAIVehicle()
{
    
}

void AAIVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AAIVehicle::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AAIVehicle::MoveRight);
}
