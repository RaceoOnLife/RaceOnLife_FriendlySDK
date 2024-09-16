// Copyright 2022 Just2Devs. All Rights Reserved.

#include "DebugFunctionLibraryLiteEditor.h"
#include "ContentBrowserModule.h"
#include "DFLEditorCommands.h"
#include "DFLEditorStyle.h"
#include "DFLInputProcess.h"

#define LOCTEXT_NAMESPACE "FDebugFunctionLibraryLiteEditorModule"

void FDebugFunctionLibraryLiteEditorModule::StartupModule()
{
	FDFLEditorCommands::Register();
	FDFLInputProcessor::Init();
	FDFLEditorStyle::Initialize();
}

void FDebugFunctionLibraryLiteEditorModule::ShutdownModule()
{
	FDFLEditorCommands::Unregister();
	FDFLInputProcessor::Cleanup();
	FDFLEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDebugFunctionLibraryLiteEditorModule, DebugFunctionLibraryLiteEditor)
