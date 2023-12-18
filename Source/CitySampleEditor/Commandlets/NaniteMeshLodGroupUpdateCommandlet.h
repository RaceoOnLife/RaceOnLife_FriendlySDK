// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Commandlets/Commandlet.h"
#include "NaniteMeshLodGroupUpdateCommandlet.generated.h"

UCLASS()
class UNaniteMeshLodGroupUpdateCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& FullCommandLine) override;
	// End UCommandlet Interface
};
