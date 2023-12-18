// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "EngineMinimal.h"
#include "EngineStats.h"
#include "Engine/DataAsset.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCitySampleAnimGraphRuntime, Log, All);

class FCitySample_AnimGraphRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};