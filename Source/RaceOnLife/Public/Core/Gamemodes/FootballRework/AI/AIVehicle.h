

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "AIVehicle.generated.h"

/**
 * 
 */
UCLASS()
class RACEONLIFE_API AAIVehicle : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
    AAIVehicle();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);

private:
	UChaosWheeledVehicleMovementComponent* VehicleMovementComponent;
	
};
