// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MovieSceneObjectBindingID.h"
#include "ZoneGraphTypes.h"

#include "Game/CitySampleGameMode.h"
#include "Util/CitySampleInterpolators.h"

#include "CitySampleBlueprintLibrary.generated.h"

class ACitySampleCharacter;
class ACitySamplePlayerController;
class ACitySampleGameplayVolume;
class ACitySampleGameMode;
class UChaosVehicleMovementComponent;

/**
 * Collection of blueprint utility functions for the CitySample.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns formatted string containing the Engine changelist version and the build config. */
	UFUNCTION(BlueprintPure, Category = "CitySample")
	static FString GetVersionString();

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static void LogCitySampleDebugMessage(const FString& Message);

	/** Track a Named Event */
	UFUNCTION(BlueprintCallable, Category = "CitySample|Profiling")
	static void LogCSVEvent(const FString EventName);

	/** Returns the name of the active Device Profile */
	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FString GetActiveDeviceProfileName();

	/** Returns the name of the base Device Profile ignoring any overrides */
	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FString GetBaseDeviceProfileName();

	/** Attempts to find and replace the current device profile with the corresponding profile for the given mode. */
	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static void OverrideDeviceProfileForMode(EDeviceProfileOverrideMode NewMode);

	/** Restores the device profile in the case it was overridden. */
	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static void RestoreDefaultDeviceProfile();

	/** Sets the default loading settings based on current platform profile. */
	UFUNCTION(BlueprintCallable, Category = "CitySample|World Partition Streaming", meta = (WorldContext = "WorldContextObject"))
	static void ApplyDefaultLoadingSettings(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "CitySample", meta = (WorldContext = "WorldContextObject"))
	static ACitySamplePlayerController* GetCitySamplePlayerController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "CitySample", meta = (WorldContext = "WorldContextObject"))
	static ACitySampleCharacter* GetCitySamplePlayerCharacter(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "CitySample", meta = (WorldContext = "WorldContextObject"))
	static ACitySampleGameMode* GetCitySampleGameMode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "CitySample", meta = (WorldContext = "WorldContextObject"))
	static class ACitySampleGameState* GetCitySampleGameState(const UObject* WorldContextObject);

	/** Checks if an actor is a player-controlled pawn in the specific traversal state. Returns that CitySamplePlayerController if true, nullptr if false */
	UFUNCTION(BlueprintPure, Category = "CitySample")
	static ACitySamplePlayerController* IsPlayerOfTraversalType(const AActor* InActor, const EPlayerTraversalState State);

	/** Returns the WorldInfo actor, if it exists. */
	UFUNCTION(BlueprintPure, Category = "CitySample", meta = (WorldContext = "WorldContextObject"))
	static ACitySampleWorldInfo* GetWorldInfo(const UObject* const WorldContextObject);

	/** Whether the mass entity visualizer is active. */
	UFUNCTION(BlueprintPure, Category = "CitySample|Mass")
	static bool IsMassVisualizationEnabled();

	//////////////////////////////////////////////////////////////////////////
	// Zone Graph

	/**
	 * Given a point, we will first try to find the nearest pedestrian lane, and then we will try to find
	 * the nearest point on that lane. Useful for spawning actors.
	 *
	 * @param [in]	InPoint		The point we are using to find the lane.
	 * @param [in]	LaneName	The name associated with the ZoneGraphTag that we should search for.
	 *							When invalid, we'll query all available lane types.
	 * @param [out]	OutPoint	The point on the lane we found. Only valid if the method return success.
	 * @param [in]	Radius		The radius / bounds length from InPoint where we'll search for the lane.
	 *
	 * @return True if we found an appropriate lane and location, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample|Navigation", Meta = (WorldContext = "WorldContextObject"))
	static bool FindNearestLaneLocationByName(UObject* WorldContextObject, FTransform InPoint, FName LaneName, FTransform& OutPoint, float Radius);

	/**
	 * Given a point, we will first try to find the nearest pedestrian lane, and then we will try to find
	 * the nearest point on that lane. Useful for spawning actors.
	 *
	 * @param [in]	InPoint		The point we are using to find the lane.
	 * @param [in]	Tag	The tag associated with the ZoneGraphTag that we search for.
	 * @param [out]	OutPoint	The point on the lane we found. Only valid if the method return success.
	 * @param [in]	Radius		The radius / bounds length from InPoint where we'll search for the lane.
	 *
	 * @return True if we found an appropriate lane and location, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample|Navigation", Meta = (WorldContext = "WorldContextObject"))
	static bool FindNearestLaneLocationByTag(UObject* WorldContextObject, FTransform InPoint, FZoneGraphTag Tag, FTransform& OutPoint, float Radius);

	/**
	 * Given a point, we will first try to find the nearest pedestrian lane, and then we will try to find
	 * the nearest point on that lane. Useful for spawning actors.
	 *
	 * @param [in]	InPoint		The point we are using to find the lane.
	 * @param [in]	LaneNames	An array of names associated with the ZoneGraphTag that we should search for.
	 *							When empty, we'll query all available lane types.
	 * @param [out]	OutPoint	The point on the lane we found. Only valid if the method return success.
	 * @param [in]	Radius		The radius / bounds length from InPoint where we'll search for the lane.
	 *
	 * @return True if we found an appropriate lane and location, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample|Navigation", Meta = (WorldContext = "WorldContextObject"))
	static bool FindNearestLaneLocationByNames(UObject* WorldContextObject, FTransform InPoint, const TArray<FName>& LaneNames, FTransform& OutPoint, float Radius);

	/**
	 * Given a point, we will first try to find the nearest pedestrian lane, and then we will try to find
	 * the nearest point on that lane. Useful for spawning actors.
	 *
	 * @param [in]	InPoint		The point we are using to find the lane.
	 * @param [in]	Tags		An array of names associated with the ZoneGraphTag that we should search for.
	 *							When empty, we'll query all available lane types.
	 * @param [out]	OutPoint	The point on the lane we found. Only valid if the method return success.
	 * @param [in]	Radius		The radius / bounds length from InPoint where we'll search for the lane.
	 *
	 * @return True if we found an appropriate lane and location, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "CitySample|Navigation", Meta = (WorldContext = "WorldContextObject"))
	static bool FindNearestLaneLocationByTags(UObject* WorldContextObject, FTransform InPoint, const TArray<FZoneGraphTag>& Tags, FTransform& OutPoint, float Radius);
	
	//////////////////////////////////////////////////////////////////////////
	// Interpolators

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static float EvalAccelInterpolatorFloat(UPARAM(ref) FAccelerationInterpolatorFloat& Interpolator, float NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FVector EvalAccelInterpolatorVector(UPARAM(ref) FAccelerationInterpolatorVector& Interpolator, FVector NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FRotator EvalAccelInterpolatorRotator(UPARAM(ref) FAccelerationInterpolatorRotator& Interpolator, FRotator NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static float EvalIIRInterpolatorFloat(UPARAM(ref) FIIRInterpolatorFloat& Interpolator, float NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FVector EvalIIRInterpolatorVector(UPARAM(ref) FIIRInterpolatorVector& Interpolator, FVector NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FRotator EvaIIRInterpolatorRotator(UPARAM(ref) FIIRInterpolatorRotator& Interpolator, FRotator NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FVector EvalDoubleIIRInterpolatorVector(UPARAM(ref) FDoubleIIRInterpolatorVector& Interpolator, FVector NewGoal, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CitySample")
	static FRotator EvalDoubleIIRInterpolatorRotator(UPARAM(ref) FIIRInterpolatorRotator& Interpolator, FRotator NewGoal, float DeltaTime);
};
