// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CitySamplePanel.h"
#include "Vehicles/CitySampleDrivingState.h"
#include "CitySampleDrivingUI.generated.h"

UENUM(BlueprintType)
enum class ECitySampleSpeedUnits : uint8
{
	MilesPerHour,
	KilometersPerHour,
	MetersPerSecond,
};

USTRUCT(BlueprintType)
struct CITYSAMPLE_API FCitySampleDrivingStateDescription
{
	GENERATED_BODY()

public:
	static const FString ParkGearDescription;
	static const FString ReverseGearDescription;
	static const FString NeutralGearDescription;
	static const FString DriveGearDescription;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBrakeOn = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHandbrakeOn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RPM = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Gear = ParkGearDescription;
};

/**
 * 
 */
UCLASS()
class CITYSAMPLE_API UCitySampleDrivingUI : public UCitySamplePanel
{
	GENERATED_BODY()
	
public:
	UCitySampleDrivingUI();

	// ~Begin UWidget Interface
	virtual void NativeOnInitialized() override;
	// ~End UWidget Interface

	// ~Begin UCitySamplePanel Interface
	virtual void NativeOnAddedToPanel() override;
	virtual void NativeOnRemoveFromPanel() override;
	virtual void UpdatePanel(const float DeltaTime = 0.0f, const UCitySampleUIComponent* const OwningCitySampleUI = nullptr) override;
	// ~End UCitySamplePanel Interface

	UFUNCTION(BlueprintPure, Category = "UI")
	const FCitySampleDrivingStateDescription& GetDrivingStateDescription() const { return DrivingStateDescription; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateDrivingStateDescription(const FCitySampleDrivingState& DrivingState);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetDrivingStateDescription() { DrivingStateDescription = FCitySampleDrivingStateDescription(); }

	UFUNCTION(BlueprintPure, Category = "UI")
	ECitySampleSpeedUnits GetSpeedUnits() const { return SpeedUnits; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetSpeedUnits(ECitySampleSpeedUnits Units) { SpeedUnits = Units; }

	/** Hook for BP event/function to update UI with the newly updated driving state description. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpdateDrivingStateDescription(const FCitySampleDrivingStateDescription& Description);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateMap(const APlayerController* const PlayerController);

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetTopSpeedRatio() const
	{
		return DrivingStateDescription.Speed / TopSpeed;
	}

	/** Returns float as text with appropriate formatting for the driving UI. */
	UFUNCTION(BlueprintPure, Category = "UI")
	static FText FloatToDescriptionFormat(const float Float, const int32 MinDigits=3, const int32 MaxDigits=4);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/** Should be called by the owner of the CitySampleDrivingUI Instance when formally creating the widget instances*/
	void SetUpDebugUI();

	/** Should be called by the owner of the CitySampleDrivingUI Instance when adding the instance to the viewport*/
	void AddDebugUI();

	/** Should be called by the owner of the CitySampleDrivingUI Instance when removing the instance from the viewport*/
	void RemoveDebugUI();
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	float TopSpeed;

	/** Panel class used to add the Vehicle Center of Mass Debug Display if needed */
	UPROPERTY(EditAnywhere, Category = "Debug")
	TSubclassOf<UCitySamplePanel> VehicleCOMDebugClass;

private:
	float GetSpeedDescription(const float Speed);
	FString GetGearDescription(const FCitySampleDrivingState& DrivingState);

	UPROPERTY(VisibleAnywhere, Category = "UI")
	FCitySampleDrivingStateDescription DrivingStateDescription;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	ECitySampleSpeedUnits SpeedUnits;

	/** Vehicle COM Debug Panel Instance */
	UPROPERTY(Transient)
	UCitySamplePanel* VehicleCOMDebug = nullptr;
};
