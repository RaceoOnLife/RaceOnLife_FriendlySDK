// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "CitySampleGameMode.generated.h"


UCLASS()
class CITYSAMPLE_API ACitySampleGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ACitySampleGameMode();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	/** Checks several variables to determine whether or not to use the fly-in city intro when beginning play */
	UFUNCTION(BlueprintPure, Category = "Sandbox")
	bool UseSandboxIntro() const;

protected:

	/** List of data layers to unload when the performance mode console variable is enabled */
	UPROPERTY(EditDefaultsOnly, Category = "Data Layer Management")
	TArray<FName> DataLayersToDisableInPerformanceMode;

	/** If true, this gamemode will attempt to play an intro sequence when beginning play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
	bool bUseSandboxIntro = false;


private:

	/** Delegate handle for our binding to the OnSyncLoadPackage core delegate */
	FDelegateHandle OnSyncLoadPackageHandle;

	/** Optionally logs the name of the package that the sync load is loading */
	void OnSyncLoadPackage(const FString& PackageName);

};
