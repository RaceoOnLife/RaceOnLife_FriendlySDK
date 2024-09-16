// Copyright 2022 Just2Devs. All Rights Reserved.

#include "DFLEditorCommands.h"

#define LOCTEXT_NAMESPACE "DFLEditorCommands"

void FDFLEditorCommandsImpl::RegisterCommands()
{
	UI_COMMAND(SelectAllDebugNodes, "Select Debug Nodes", "Select all debug nodes", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllDrawDebugNodes, "Select Draw Debug Nodes", "Select all draw debug nodes", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllK2PrintNodes, "Select Print Nodes", "Select all print nodes", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(SelectAllTickNodes, "Select Tick Nodes", "Select all nodes running on tick", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllNoTickNodes, "Select No Tick Nodes", "Select all nodes not running on tick", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(SelectAllOverridenDurationNodes, "Select Overriden Duration Nodes", "Select all nodes with overriden duration", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllOverridenNodes, "Select Overriden Nodes", "Select all nodes overriden", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllNoOverridenNodes, "Select No Overriden Nodes", "Select all nodes that are not overriden", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(SelectAllPrintLogNodes, "Select Print & Log Nodes", "Select all print & log nodes", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllPrintNodes, "Select Print Nodes", "Select all print nodes", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllLogNodes, "Select Log Nodes", "Select all log nodes", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SelectAllPrintMessageNodes, "Select Print Message Nodes", "Select all print message nodes.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllPrintWarningNodes, "Select Print Warning Nodes", "Select all print warning nodes.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectAllPrintErrorNodes, "Select Print Error Nodes", "Select all print error nodes.", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(EnableAllSelectedNodes, "Enable Selected Nodes", "Enable selected nodes", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisableAllSelectedNodes, "Disable Selected Nodes", "Disable selected nodes", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(AddDebugProperties, "Add Debug Properties", "Add debug properties", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnableBlueprintDebug, "Enable Blueprint Debug", "Enable blueprint debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisableBlueprintDebug, "Disable Blueprint Debug", "Disable blueprint debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnableBlueprintPrintDebug, "Enable Blueprint Print Debug", "Enable blueprint print debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisableBlueprintPrintDebug, "Disable Blueprint Print Debug", "Disable blueprint print debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnableBlueprintLogDebug, "Enable Blueprint Log Debug", "Enable blueprint log debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisableBlueprintLogDebug, "Disable Blueprint Log Debug", "Disable blueprint log debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnableBlueprintDrawDebug, "Enable Blueprint Draw Debug", "Enable blueprint draw debug", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisableBlueprintDrawDebug, "Disable Blueprint Draw Debug", "Disable blueprint draw debug", EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(UpdateNodePrintLevel, "Update Node Print Level", "Update node print level", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::R))
	UI_COMMAND(UpdateNodePrintState, "Update Node Print State", "Update node print state", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::G))
	UI_COMMAND(UpdateNodeEnableState, "Update Node Enable State", "Update node enable state", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::E));
	UI_COMMAND(ToggleNodeDurationState, "Toggle Node Duration", "Toggle node duration", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::D));
	UI_COMMAND(ToggleNodeOverride, "Toggle Node Override", "Toggle node override", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Q));
	UI_COMMAND(ToggleNodeTickMethod, "Toggle Node Tick Method", "Toggle the node tick method. Whether it runs on tick or not.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::T));
}

void FDFLEditorCommands::Register()
{
	return FDFLEditorCommandsImpl::Register();
}

const FDFLEditorCommandsImpl& FDFLEditorCommands::Get()
{
	return FDFLEditorCommandsImpl::Get();
}

void FDFLEditorCommands::Unregister()
{
	return FDFLEditorCommandsImpl::Unregister();
}

#undef LOCTEXT_NAMESPACE
