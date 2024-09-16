// Copyright 2022 Just2Devs. All Rights Reserved.

#include "DFLInputProcess.h"
#include "DFLEditorCommands.h"
#include "K2Node_Print.h"
#include "SGraphPanel.h"
#include "Kismet2/BlueprintEditorUtils.h"

static TSharedPtr<FDFLInputProcessor> DFLInputProcessorInstance;

FDFLInputProcessor::FDFLInputProcessor()
{
	Commands = MakeShareable(new FUICommandList());

	Commands->MapAction(
		FDFLEditorCommands::Get().UpdateNodePrintLevel,
		FExecuteAction::CreateRaw(this, &FDFLInputProcessor::UpdateNodePrintLevel),
		FCanExecuteAction::CreateRaw(this, &FDFLInputProcessor::HasPrintNodeSelected));

	Commands->MapAction(
		FDFLEditorCommands::Get().UpdateNodePrintState,
		FExecuteAction::CreateRaw(this, &FDFLInputProcessor::UpdateNodePrintState),
		FCanExecuteAction::CreateRaw(this, &FDFLInputProcessor::HasPrintNodeSelected));

	Commands->MapAction(
		FDFLEditorCommands::Get().UpdateNodeEnableState,
		FExecuteAction::CreateRaw(this, &FDFLInputProcessor::UpdateNodeEnableState),
		FCanExecuteAction::CreateRaw(this, &FDFLInputProcessor::HasNodeSelected));

	Commands->MapAction(
		FDFLEditorCommands::Get().ToggleNodeDurationState,
		FExecuteAction::CreateRaw(this, &FDFLInputProcessor::ToggleNodeDurationState),
		FCanExecuteAction::CreateRaw(this, &FDFLInputProcessor::HasNodeSelected));
	
	Commands->MapAction(
		FDFLEditorCommands::Get().ToggleNodeOverride,
		FExecuteAction::CreateRaw(this, &FDFLInputProcessor::ToggleNodeOverrideState),
		FCanExecuteAction::CreateRaw(this, &FDFLInputProcessor::HasNodeSelected));
	
	Commands->MapAction(
		FDFLEditorCommands::Get().ToggleNodeTickMethod,
		FExecuteAction::CreateRaw(this, &FDFLInputProcessor::ToggleNodeTickMethod),
		FCanExecuteAction::CreateRaw(this, &FDFLInputProcessor::HasNodeSelected));
}

FDFLInputProcessor::~FDFLInputProcessor()
{
}

void FDFLInputProcessor::Init()
{
	DFLInputProcessorInstance = MakeShareable(new FDFLInputProcessor());
	FSlateApplication::Get().RegisterInputPreProcessor(DFLInputProcessorInstance);
}

TSharedPtr<FDFLInputProcessor> FDFLInputProcessor::Get()
{
	return DFLInputProcessorInstance;
}

void FDFLInputProcessor::Cleanup()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(DFLInputProcessorInstance);
	}

	DFLInputProcessorInstance.Reset();
}

void FDFLInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}

bool FDFLInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	const TArray<UEdGraphNode*> SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() == 0)
	{
		return false;
	}

	if (Commands->ProcessCommandBindings(InKeyEvent.GetKey(), SlateApp.GetModifierKeys(), InKeyEvent.IsRepeat()))
	{
		 return true;
	}

	return false;
}

TSharedPtr<SGraphPanel> FDFLInputProcessor::GetGraph() const
{
	FSlateApplication& SlateApplication = FSlateApplication::Get();
	FWidgetPath CurrentWidget = SlateApplication.LocateWindowUnderMouse(SlateApplication.GetCursorPos(), SlateApplication.GetInteractiveTopLevelWindows());
	FScopedSwitchWorldHack SwitchWorldHack(CurrentWidget);
	for (int i = 0; i < CurrentWidget.Widgets.Num(); i++)
	{
		if (CurrentWidget.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return TSharedPtr<SGraphPanel>(StaticCastSharedRef<SGraphPanel>(CurrentWidget.Widgets[i].Widget));
		}
	}
	return nullptr;
}

TArray<UEdGraphNode*> FDFLInputProcessor::GetSelectedNodes() const
{
	TArray<UEdGraphNode*> Nodes;
	const TSharedPtr<SGraphPanel> Graph = GetGraph();
	if (!Graph || !Graph->GetGraphObj() || !Graph->GetGraphObj()->GetOuter()) return Nodes;

	UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetGraphObj()->GetOuter());
	if(!Blueprint) return Nodes;

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);

	for(const UEdGraph* LocalGraph : Graphs)
	{
		TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(LocalGraph);
		if(GraphEditor.Get())
		{
			FGraphPanelSelectionSet SelectionSet = GraphEditor->GetSelectedNodes();
			for(UObject* Object : SelectionSet)
			{
				if(Cast<UK2Node_Print>(Object)) Nodes.Add(Cast<UEdGraphNode>(Object));
			}
		}
	}

	return Nodes;
}

bool FDFLInputProcessor::HasNodeSelected() const
{
	return GetSelectedNodes().Num() > 0;
}

UBlueprint* FDFLInputProcessor::GetBlueprint() const
{
	const TSharedPtr<SGraphPanel> Graph = GetGraph();
	if (!Graph || !Graph->GetGraphObj() || !Graph->GetGraphObj()->GetOuter()) return nullptr;

	UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetGraphObj()->GetOuter());
	return Blueprint;
}

TArray<UEdGraphNode*> FDFLInputProcessor::GetSelectedPrintNodes() const
{
	TArray<UEdGraphNode*> Nodes;
	
	UBlueprint* Blueprint = GetBlueprint();
	if(!Blueprint) return Nodes;

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);

	for(const UEdGraph* LocalGraph : Graphs)
	{
		TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(LocalGraph);
		if(GraphEditor.Get())
		{
			FGraphPanelSelectionSet SelectionSet = GraphEditor->GetSelectedNodes();
			for(UObject* Object : SelectionSet)
			{
				if(Cast<UK2Node_Print>(Object)) Nodes.Add(Cast<UEdGraphNode>(Object));
			}
		}
	}

	return Nodes;
}

bool FDFLInputProcessor::HasPrintNodeSelected() const
{
	return GetSelectedPrintNodes().Num() > 0;
}

void FDFLInputProcessor::UpdateNodePrintLevel() const
{
	TArray<UEdGraphNode*> SelectedNodes = GetSelectedPrintNodes();
	for (UEdGraphNode* Node : SelectedNodes)
	{
		if (UK2Node_Print* PrintNode = Cast<UK2Node_Print>(Node))
		{
			PrintNode->CycleNodePrintLevel();
		}
	}
}

void FDFLInputProcessor::UpdateNodePrintState() const
{
	TArray<UEdGraphNode*> SelectedNodes = GetSelectedPrintNodes();
	for (UEdGraphNode* Node : SelectedNodes)
	{
		if (UK2Node_Print* PrintNode = Cast<UK2Node_Print>(Node))
		{
			PrintNode->CycleNodePrintState();
		}
	}
}

void FDFLInputProcessor::UpdateNodeEnableState() const
{
	TArray<UEdGraphNode*> SelectedNodes = GetSelectedNodes();
	for (UEdGraphNode* Node : SelectedNodes)
	{
		if(Cast<UK2Node_Print>(Node))
		{
			UEdGraphNode* GraphNode = Cast<UEdGraphNode>(Node);
			const ENodeEnabledState EnableState = GraphNode->GetDesiredEnabledState();
			GraphNode->SetEnabledState(EnableState == ENodeEnabledState::DevelopmentOnly ? ENodeEnabledState::Disabled : ENodeEnabledState::DevelopmentOnly);
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void FDFLInputProcessor::ToggleNodeDurationState() const
{
	TArray<UEdGraphNode*> SelectedNodes = GetSelectedNodes();
	for (UEdGraphNode* Node : SelectedNodes)
	{
		if (UK2Node_Print* PrintNode = Cast<UK2Node_Print>(Node))
		{
			PrintNode->SetOverrideMethod(PrintNode->OverridenMethod == EK2NodePrintOverrideMethod::NoOverriden ? EK2NodePrintOverrideMethod::OverrideDuration : EK2NodePrintOverrideMethod::NoOverriden);
		}
	}
}

void FDFLInputProcessor::ToggleNodeOverrideState() const
{
	TArray<UEdGraphNode*> SelectedNodes = GetSelectedNodes();
	for (UEdGraphNode* Node : SelectedNodes)
	{
	}
}

void FDFLInputProcessor::ToggleNodeTickMethod() const
{
	TArray<UEdGraphNode*> SelectedNodes = GetSelectedNodes();
	for (UEdGraphNode* Node : SelectedNodes)
	{
		if (UK2Node_Print* PrintNode = Cast<UK2Node_Print>(Node))
		{
			PrintNode->UpdateTickMethod(PrintNode->TickMethod == EK2NodePrintTickMethod::NoTick ? EK2NodePrintTickMethod::Tick : EK2NodePrintTickMethod::NoTick);
		}
	}
}
