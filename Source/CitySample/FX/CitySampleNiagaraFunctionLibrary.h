// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NiagaraCommon.h"
#include "UObject/ObjectMacros.h"
#include "CitySampleNiagaraFunctionLibrary.generated.h"

class UNiagaraSystem;
class USkeletalMesh;

/**
* A C++ and Blueprint accessible library of utility functions for accessing Niagara simulations
*/
UCLASS()
class UCitySampleNiagaraFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	/**
	* Pins connectivity information for the specified skeletal mesh to the lifetime of the provided world.
	*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (Keywords = "CitySample Niagara System", WorldContext = "WorldContextObject"))
	static void PinNiagaraSkelMeshConnectivity(const UObject* WorldContextObject, USkeletalMesh* SkeletalMesh, int32 LodIndex);

	/**
	* Pins UV mapping information for the specified skeletal mesh to the lifetime of the provided world.
	*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (Keywords = "CitySample Niagara System", WorldContext = "WorldContextObject"))
	static void PinNiagaraSkelMeshUvMapping(const UObject* WorldContextObject, USkeletalMesh* SkeletalMesh, int32 LodIndex, int32 UvSetIndex);

	/** Set the desired LOD level to sample from. */
	UFUNCTION(BlueprintCallable, Category = Niagara)
	static void SetSkeletalMeshDataInterfaceWholeMeshLOD(UNiagaraComponent* NiagaraSystem, const FString& OverrideName, int32 WholeMeshLOD);
};
