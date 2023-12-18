// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"

#include "CitySampleCrowdSettings.generated.h"

class UMassEntityConfigAsset;

UCLASS(config=Mass, defaultconfig, meta=(DisplayName="Crowd"))
class CITYSAMPLE_API UCitySampleCrowdSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	UFUNCTION(BlueprintCallable)
	static UCitySampleCrowdSettings* GetMutable()
	{
		return GetMutableDefault<UCitySampleCrowdSettings>();
	}

	UFUNCTION(BlueprintCallable)
	static const UCitySampleCrowdSettings* Get()
	{
		return GetDefault<UCitySampleCrowdSettings>();
	}

	UMassEntityConfigAsset* GetAgentConfig() const { return AgentConfig; }
	UStaticMesh* GetISMFarLodMeshOverride() const;

	/* Mass LOD significance at which to use a single ISM */
	UPROPERTY(config, EditAnywhere, Category = "LOD", meta = (ClampMin = "0.0001", ClampMax = "4.0", UIMin = "0.0001", UIMax = "4.0"))
	float ISMFarLodSignificanceThreshold = 3.0f;

	UPROPERTY(config, EditAnywhere, Category="Crowd")
	bool bForceMassCrowdToAverage = true;

	UPROPERTY(config, EditAnywhere, Category="Crowd")
	bool bHideAccessoriesForMassCrowd = true;

	// Instead of randomizing the crowd characters, mass agents will attempt to use the default set of options from the template actor.
	UPROPERTY(config, EditAnywhere, Category="Crowd")
	bool bMassCrowdShouldUseActorDefaultOptions = false;

	UPROPERTY(config, EditAnywhere, Category = "Crowd")
	TSoftObjectPtr<UMassEntityConfigAsset> AgentConfigAsset;

private:
	UPROPERTY()
	UMassEntityConfigAsset* AgentConfig;

	UPROPERTY(config, EditAnywhere, Category = "LOD")
	TSoftObjectPtr<UStaticMesh> ISMFarLodMeshOverride;
};
