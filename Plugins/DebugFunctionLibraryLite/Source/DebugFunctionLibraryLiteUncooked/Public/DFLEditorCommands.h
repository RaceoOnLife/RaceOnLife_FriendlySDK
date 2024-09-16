// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"
#include "Runtime/Launch/Resources/Version.h"

class FDFLEditorCommandsImpl : public TCommands<FDFLEditorCommandsImpl>
{
public:

	FDFLEditorCommandsImpl()
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		: TCommands<FDFLEditorCommandsImpl>( TEXT("DFLEditor"), NSLOCTEXT("Contexts", "DFLEditor", "Debug Function Library"), NAME_None, FAppStyle::GetAppStyleSetName())
#else
		: TCommands<FDFLEditorCommandsImpl>( TEXT("DFLEditor"), NSLOCTEXT("Contexts", "DFLEditor", "Debug Function Library"), NAME_None, FEditorStyle::GetStyleSetName() )
#endif
	{
	}	

	virtual ~FDFLEditorCommandsImpl()
	{
	}

	DEBUGFUNCTIONLIBRARYLITEUNCOOKED_API virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> UpdateNodePrintLevel;
	TSharedPtr<FUICommandInfo> UpdateNodePrintState;
	TSharedPtr<FUICommandInfo> UpdateNodeEnableState;
	TSharedPtr<FUICommandInfo> ToggleNodeDurationState;
	TSharedPtr<FUICommandInfo> ToggleNodeOverride;
	TSharedPtr<FUICommandInfo> ToggleNodeTickMethod;
	
	TSharedPtr<FUICommandInfo> SelectAllDebugNodes;
	TSharedPtr<FUICommandInfo> SelectAllDrawDebugNodes;
	TSharedPtr<FUICommandInfo> SelectAllK2PrintNodes;
	
	TSharedPtr<FUICommandInfo> SelectAllOverridenDurationNodes;
	TSharedPtr<FUICommandInfo> SelectAllOverridenNodes;
	TSharedPtr<FUICommandInfo> SelectAllNoOverridenNodes;
	
	TSharedPtr<FUICommandInfo> SelectAllTickNodes;
	TSharedPtr<FUICommandInfo> SelectAllNoTickNodes;
	
	TSharedPtr<FUICommandInfo> SelectAllPrintLogNodes;
	TSharedPtr<FUICommandInfo> SelectAllPrintNodes;
	TSharedPtr<FUICommandInfo> SelectAllLogNodes;
	
	TSharedPtr<FUICommandInfo> SelectAllPrintMessageNodes;
	TSharedPtr<FUICommandInfo> SelectAllPrintWarningNodes;
	TSharedPtr<FUICommandInfo> SelectAllPrintErrorNodes;

	TSharedPtr<FUICommandInfo> EnableAllSelectedNodes;
	TSharedPtr<FUICommandInfo> DisableAllSelectedNodes;
	
	TSharedPtr<FUICommandInfo> AddDebugProperties;
	TSharedPtr<FUICommandInfo> EnableBlueprintDebug;
	TSharedPtr<FUICommandInfo> DisableBlueprintDebug;
	TSharedPtr<FUICommandInfo> EnableBlueprintPrintDebug;
	TSharedPtr<FUICommandInfo> DisableBlueprintPrintDebug;
	TSharedPtr<FUICommandInfo> EnableBlueprintLogDebug;
	TSharedPtr<FUICommandInfo> DisableBlueprintLogDebug;
	TSharedPtr<FUICommandInfo> EnableBlueprintDrawDebug;
	TSharedPtr<FUICommandInfo> DisableBlueprintDrawDebug;
};

class DEBUGFUNCTIONLIBRARYLITEUNCOOKED_API FDFLEditorCommands
{
public:
	static void Register();

	static const FDFLEditorCommandsImpl& Get();

	static void Unregister();
};
