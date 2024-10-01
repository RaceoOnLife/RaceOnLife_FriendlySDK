#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DroneBase.generated.h"

UCLASS()
class RACEONLIFE_API ADroneBase : public APawn
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:	
    ADroneBase();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void MoveUpward(float Value);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Movement")
    float MovementSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Movement")
    float LiftSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Movement")
    float TiltAngle;

    UFUNCTION(BlueprintCallable, Category = "Drone")
    float GetDroneCharge();

private:
    FVector CurrentVelocity;
    FRotator CurrentTilt;

    float DroneCharge;

    void ReduceDroneCharge();
};
