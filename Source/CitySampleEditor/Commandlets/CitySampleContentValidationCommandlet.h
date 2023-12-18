// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Commandlets/Commandlet.h"
#include "CitySampleContentValidationCommandlet.generated.h"

class IAssetRegistry;

UCLASS()
class UCitySampleContentValidationCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface

private:
	/** Helper functions */
	bool GetAllChangedFiles(IAssetRegistry& AssetRegistry, const FString& P4CmdString, const TArray<FString>& ExcludedDirectories, TArray<FString>& OutChangedPackageNames, TArray<FString>& DeletedPackageNames, TArray<FString>& OutChangedCode, TArray<FString>& OutChangedOtherFiles) const;
	void GetAllPackagesInPath(IAssetRegistry& AssetRegistry, const FString& InPathString, TArray<FString>& OutPackageNames) const;
	void GetAllPackagesOfType(const FString& OfTypeString, TArray<FString>& OutPackageNames) const;
	bool LaunchP4(const FString& Args, TArray<FString>& Output, int32& OutReturnCode) const;
	void GetChangedAssetsForCode(IAssetRegistry& AssetRegistry, const FString& InChangedHeader, TArray<FString>& OutChangedPackageNames) const;
};
