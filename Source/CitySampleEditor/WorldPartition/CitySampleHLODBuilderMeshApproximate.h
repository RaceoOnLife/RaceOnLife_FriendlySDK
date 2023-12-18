// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HLODBuilderMeshApproximate.h"

#include "CitySampleHLODBuilderMeshApproximate.generated.h"

/**
 * Extend the MeshApproximate HLOD builder to feed CitySample specific data to the mesh generation process.
 * In order to have better texture space utilization, we use this class to add the collision meshes of
 * buildings to the actor approximation algorithm. This ensure buildings are processed as watertight blocks.
 */
UCLASS()
class UCitySampleHLODBuilderMeshApproximate : public UHLODBuilderMeshApproximate
{
	GENERATED_UCLASS_BODY()

public:
	virtual TArray<UActorComponent*> Build(const FHLODBuildContext& InHLODBuildContext, const TArray<UActorComponent*>& InSubComponents) const override;
};
