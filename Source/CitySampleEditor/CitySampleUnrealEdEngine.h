// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdEngine.h"

#include "CitySampleUnrealEdEngine.generated.h"

UCLASS()
class UCitySampleUnrealEdEngine : public UUnrealEdEngine
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config)
	TArray<FString> SourceControlStatusBranches;

	virtual void Init(IEngineLoop* InEngineLoop) override;
	void UpdateSourceControlSettings();
};