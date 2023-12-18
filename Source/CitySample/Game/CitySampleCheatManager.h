// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "CitySampleCheatManager.generated.h"

/**
 * Extension of the CheatManager class that enables custom console commands and debug functions for development use.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UCitySampleCheatManager();

	/** Re-orients a vehicle with the local Z-axis pointing up */
	UFUNCTION(exec, BlueprintCallable)
	virtual void FlipVehicle();

	/**
	 * @brief Loads ClassName and spawns an Actor of that class
	 * @param ClassName Class name of the Actor to be spawned
	 */
	virtual void Summon(const FString& ClassName) override;

	/** Hook to call summon from BP, ideally would just make Summon BP callable in UCheatManager. */
	UFUNCTION(BlueprintCallable, DisplayName = "Summon", Category = "CheatManager|Summon")
	void SummonBlueprintCallable(const FString& ClassName);

	/**
	 * @brief Loads ClassName and spawns an Actor of that class with an offset from the player
	 * @param ClassName Class name of the Actor to be spawned
	 * @param X Summon offset on the X-axis in camera space
	 * @param Y Summon offset on the Y-axis in camera space
	 * @param Z Summon offset on the Z-axis in camera space
	 */
	UFUNCTION(exec)
	virtual void SummonAtOffset(const FString& ClassName, float X, float Y, float Z);

	/**
	 * @brief Loads ClassName and spawns an Actor of that class with an offset from the player
	 * @param ClassName Class name of the Actor to be spawned
	 * @param Offset Summon offset on each axis in camera space
	 */
	UFUNCTION(BlueprintCallable, DisplayName = "Summon At Offset", Category = "CheatManager|Summon")
	virtual void SummonAtOffsetBlueprintCallable(const FString& ClassName, FVector Offset);

	/** Toggles whether the CitySample UI is hidden. */
	UFUNCTION(exec, BlueprintCallable)
	void CitySampleHideUI(const bool bShouldBeHidden);

	/** 
	 * Picks a random vehicle, sets it as the viewtarget. 
	 * @param bWatch true to pick a random vehicle and watch it, false to go back to the possessed pawn
	 */
	UFUNCTION(Exec, BlueprintCallable, Category = "CheatManager")
	virtual void WatchRandomVehicle(bool bWatch);

	UFUNCTION(Exec, BlueprintCallable, Category = "CheatManager")
	void TuneCurrentVehicle();

	/**
	 * Scales globally the count of crowd agents, will destroy all current crowd agents an
	 * re-spawn them to the new count.
	 * @param Scale 
	 */
	UFUNCTION(Exec, BlueprintCallable, Category = "CheatManager")
	void ScaleCrowdCount(const float Scale);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CheatManager")
	float TeleportOffsetStep;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CheatManager")
	float FlipVehicleMaxDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CheatManager|Summon")
	FVector SummonOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CheatManager|Summon")
	TMap<FString, TSoftClassPtr<AActor>> SummonShortNamesMap;
};
