// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "CitySampleAssetManager.generated.h"


/**
 * See docs https://docs.unrealengine.com/4.26/en-US/ProductionPipelines/AssetManagement/
 *
 * We set this as our AssetManager in Config/DefaultEngine.ini.
 */
UCLASS(config = Game)
class UCitySampleAssetManager : public UAssetManager
{
	GENERATED_BODY()

private:

	UCitySampleAssetManager();

	/** State/Bundle to always load on client (shared among all sub-modes) */
	static const FName LoadStateClient;

	//~ Begin UObject Interface
	virtual void PostInitProperties() override; 
	//~ End UObject Interface
	
	//~ Begin UAssetManager Interface
	virtual void PostInitialAssetScan() override;
	virtual bool ShouldScanPrimaryAssetType(FPrimaryAssetTypeInfo& TypeInfo) const override;

#if WITH_EDITOR
protected:
	virtual void PreBeginPIE(bool bStartSimulate) override;
	virtual void EndPIE(bool bStartSimulate) override;
	//~ End UAssetManager Interface
private:

	bool bInPreBeginPIE = false;
	bool bAlreadyLoadedForPIE = false;
#endif
	
	/** Bundle states for current platform */
	UPROPERTY()
	TArray<FName> PlatformBundleState;

	/** Current default bundle state, of in game or menu */
	UPROPERTY()
	TArray<FName> DefaultBundleState;

public:

	TSharedPtr<FStreamableHandle> PreloadItemDefinitions();
	void UnloadItemDefinitions();
};