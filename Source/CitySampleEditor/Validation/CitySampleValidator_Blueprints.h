// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CitySampleEditorValidator.h"
#include "CitySampleValidator_Blueprints.generated.h"

UCLASS()
class UCitySampleValidator_Blueprints : public UCitySampleEditorValidator
{
	GENERATED_BODY()

public:
	UCitySampleValidator_Blueprints();

	static void SetShouldLoadReferencingBlueprintsInEditor(bool bNewShouldLoadReferencingBlueprintsInEditor);

protected:
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;

private:
	static bool bShouldLoadReferencingBlueprintsInEditor;
};