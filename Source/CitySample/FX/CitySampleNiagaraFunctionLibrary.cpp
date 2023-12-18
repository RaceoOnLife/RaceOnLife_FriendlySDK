// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleNiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraWorldManager.h"

UCitySampleNiagaraFunctionLibrary::UCitySampleNiagaraFunctionLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UCitySampleNiagaraFunctionLibrary::PinNiagaraSkelMeshConnectivity(const UObject* WorldContextObject, USkeletalMesh* SkeletalMesh, int32 LodIndex)
{
	if (!SkeletalMesh || !SkeletalMesh->IsValidLODIndex(LodIndex))
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World != nullptr)
	{
		if (FNiagaraWorldManager* WorldManager = FNiagaraWorldManager::Get(World))
		{
			FNDI_SkeletalMesh_GeneratedData& GeneratedData = WorldManager->EditGeneratedData<FNDI_SkeletalMesh_GeneratedData>();

			TWeakObjectPtr<USkeletalMesh> SkelMeshPtr = SkeletalMesh;

			FSkeletalMeshConnectivityUsage Usage(false, true);
			FSkeletalMeshConnectivityHandle Handle = GeneratedData.GetCachedConnectivity(SkelMeshPtr, LodIndex, Usage, false);

			Handle.PinAndInvalidateHandle();
		}
	}
}

void UCitySampleNiagaraFunctionLibrary::PinNiagaraSkelMeshUvMapping(const UObject* WorldContextObject, USkeletalMesh* SkeletalMesh, int32 LodIndex, int32 UvSetIndex)
{
	if (!SkeletalMesh || !SkeletalMesh->IsValidLODIndex(LodIndex))
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World != nullptr)
	{
		if (FNiagaraWorldManager* WorldManager = FNiagaraWorldManager::Get(World))
		{
			FNDI_SkeletalMesh_GeneratedData& GeneratedData = WorldManager->EditGeneratedData<FNDI_SkeletalMesh_GeneratedData>();

			TWeakObjectPtr<USkeletalMesh> SkelMeshPtr = SkeletalMesh;

			FMeshUvMappingUsage Usage(false, true);
			FSkeletalMeshUvMappingHandle Handle = GeneratedData.GetCachedUvMapping(SkelMeshPtr, LodIndex, UvSetIndex, Usage, false);

			Handle.PinAndInvalidateHandle();
		}
	}
}

void UCitySampleNiagaraFunctionLibrary::SetSkeletalMeshDataInterfaceWholeMeshLOD(UNiagaraComponent* NiagaraSystem, const FString& OverrideName, int32 WholeMeshLOD)
{
	if (!NiagaraSystem)
	{
		//UE_LOG(LogNiagara, Warning, TEXT("NiagaraSystem in \"Set Skeletal Mesh Data Interface Sampling Regions\" is NULL, OverrideName \"%s\", skipping."), *OverrideName);
		return;
	}

	UNiagaraDataInterfaceSkeletalMesh* SkeletalMeshInterface = UNiagaraFunctionLibrary::GetSkeletalMeshDataInterface(NiagaraSystem, OverrideName);
	if (!SkeletalMeshInterface)
	{
		//UE_LOG(LogNiagara, Warning, TEXT("UNiagaraFunctionLibrary::SetSkeletalMeshDataInterfaceWholeMeshLOD: Did not find a matching Skeletal Mesh Data Interface variable named \"%s\" in the User variables of NiagaraSystem \"%s\" ."), *OverrideName, *GetFullNameSafe(NiagaraSystem));
		return;
	}

	SkeletalMeshInterface->SetWholeMeshLODFromBlueprints(WholeMeshLOD);
}
