// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCrowdSettings.h"
#include "MassEntityConfigAsset.h"

TAutoConsoleVariable<int32> CVarCrowdMinLOD(
	TEXT("Crowd.MinLOD"),
	0,
	TEXT("Set the Minimum LOD for Crowd Characters. Enforced via a LOD Sync Component"),
	ECVF_Scalability);

void UCitySampleCrowdSettings::PostInitProperties() 
{
	Super::PostInitProperties();

	if (AgentConfigAsset.IsPending())
	{
		//AgentConfigAsset.LoadSynchronous();
	}
	AgentConfig = AgentConfigAsset.Get();
}

#if WITH_EDITOR
void UCitySampleCrowdSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (AgentConfigAsset.IsPending())
	{
		//AgentConfigAsset.LoadSynchronous();
	}
	AgentConfig = AgentConfigAsset.Get();
}
#endif // WITH_EDITOR

UStaticMesh* UCitySampleCrowdSettings::GetISMFarLodMeshOverride() const
{
	return ISMFarLodMeshOverride.IsNull() ? nullptr : ISMFarLodMeshOverride.LoadSynchronous();
}