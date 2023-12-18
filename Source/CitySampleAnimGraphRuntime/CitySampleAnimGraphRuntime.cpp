// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimGraphRuntime.h"
#include "UnrealEd.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "CitySampleAnimGraphRuntime"

DEFINE_LOG_CATEGORY(LogCitySampleAnimGraphRuntime);

IMPLEMENT_GAME_MODULE(FCitySample_AnimGraphRuntimeModule, CitySampleAnimGraphRuntime);

void FCitySample_AnimGraphRuntimeModule::StartupModule()
{
}

void FCitySample_AnimGraphRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
