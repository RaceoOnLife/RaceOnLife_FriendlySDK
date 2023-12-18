// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrivableVehicleComponent.generated.h"

class ACitySampleCharacter;
class ACitySampleVehicleBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCitySampleVehicleOnDriverEnter, UDrivableVehicleComponent* const, DrivableComponent, ACitySampleVehicleBase* const, Vehicle, APawn* const, Driver);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCitySampleVehicleOnDriverExit, UDrivableVehicleComponent* const, DrivableComponent, ACitySampleVehicleBase* const, Vehicle, APawn* const, Driver);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CITYSAMPLE_API UDrivableVehicleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDrivableVehicleComponent();

	UFUNCTION(BlueprintCallable)
	bool DriverGetIn(ACitySampleCharacter* NewDriver);

	UPROPERTY(BlueprintAssignable)
	FCitySampleVehicleOnDriverEnter OnDriverEnter;

	UFUNCTION(BlueprintCallable)
	void DriverGetOut();

	UPROPERTY(BlueprintAssignable)
	FCitySampleVehicleOnDriverExit OnDriverExit;

private:
	ACitySampleVehicleBase* GetOwningVehicle() const;

	UPROPERTY(Transient, BlueprintReadOnly, Meta=(AllowPrivateAccess="true"))
	ACitySampleCharacter* Driver;
};
