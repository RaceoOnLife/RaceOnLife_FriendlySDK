// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "EngineMinimal.h"
#include "EngineStats.h"
#include "Engine/DataAsset.h"
#include "AITypes.h"

class IConsoleObject;

DECLARE_LOG_CATEGORY_EXTERN(LogCitySampleEditor, Log, All);

class FCitySample_EditorModule : public IModuleInterface
{
public:
	virtual void StartupModule();

private:
	TArray<IConsoleObject*> EditorCommands;
};