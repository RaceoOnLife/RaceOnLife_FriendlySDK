// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Commandlets/Commandlet.h"

#include "AssetHardReferencesCommandlet.generated.h"

UCLASS()
class CITYSAMPLEEDITOR_API UAssetHardReferencesCommandlet : public UCommandlet
{
	GENERATED_BODY()
	
public:
	UAssetHardReferencesCommandlet();
	
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& FullCommandLine) override;
	// End UCommandlet Interface
};