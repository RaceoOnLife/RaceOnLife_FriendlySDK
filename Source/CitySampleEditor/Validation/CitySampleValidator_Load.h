// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CitySampleEditorValidator.h"
#include "CitySampleValidator_Load.generated.h"

UCLASS()
class UCitySampleValidator_Load : public UCitySampleEditorValidator
{
	GENERATED_BODY()

public:
	UCitySampleValidator_Load();

	virtual bool IsEnabled() const override;

	static bool GetLoadWarningsAndErrorsForPackage(const FString& PackageName, TArray<FString>& OutWarningsAndErrors);

protected:
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
	
private:
	static TArray<FString> InMemoryReloadLogIgnoreList;
};