// Copyright 2022 Just2Devs. All Rights Reserved.

#include "K2Node_Print.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "DFLDebugFL.h"
#include "DFLEditorCommands.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FormatText.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_TunnelBoundary.h"
#include "KismetCompiler.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Developer/ToolMenus/Public/ToolMenu.h"
#include "DebugFunctionLibraryLite/Public/DebugFunctionLibrarySettings.h"
#include "Runtime/Launch/Resources/Version.h"

namespace K2Node_PrintNodeHelper
{
	const FName WorldContextPin = "WorldContext";
	const FName StringPin = "String";
	const FName DurationPin = "Duration";
	const FName CallingFunctionPin = "CallingFunction";
}

#define LOCTEXT_NAMESPACE "K2Node_Print"

#if WITH_EDITOR
void UK2Node_Print::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UK2Node_Print, NodePrintLevel) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UK2Node_Print, NodePrintState) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UK2Node_Print, TickMethod))
		{
			ReconstructNode();
			RefreshNode();
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UK2Node_Print, OverridenMethod))
		{
			if (OverridenMethod == EK2NodePrintOverrideMethod::NoOverriden)
			{
				if (GetDurationPin()) RemovePin(GetDurationPin());
			}

			ReconstructNode();
			RefreshNode();
		}
	}
}
#endif

void UK2Node_Print::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create exec pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// Create then pin
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Create string pin
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, K2Node_PrintNodeHelper::StringPin)->DefaultValue = Settings->PrintNodeDefaultString;

	// Create duration pin if overriden duration is true
	if (OverridenMethod == EK2NodePrintOverrideMethod::OverrideDuration)
	{
#if ENGINE_MAJOR_VERSION == 5
		UEdGraphPin* DurationPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, K2Node_PrintNodeHelper::DurationPin);
#else
		UEdGraphPin* DurationPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Float, K2Node_PrintNodeHelper::DurationPin);
#endif
		DurationPin->DefaultValue = Settings ? FString::SanitizeFloat(Settings->PrintDuration) : "2.0";
		if (TickMethod == EK2NodePrintTickMethod::Tick) DurationPin->bHidden = true;
	}

	// Create arguments pins
	for (const FName& PinName : PinNames)
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinName);
	}
}

void UK2Node_Print::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	// Try to get the function that is calling this node, may fail if the node exec is connected to multiple nodes
	FName CallFunctionName = NAME_None;
	if (GetExecPin()->LinkedTo.Num() > 1)
	{
		CallFunctionName = FName("AMBIGUOUS");
	}
	else
	{
		const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
		MaxGetHeadNodeRecursiveIterations = Settings->MaxPrintNodeFunctionDiscoveryIterations;
		UEdGraphNode* HeadNode = GetHeadNode(GetExecPin());
		if (HeadNode)
		{
			if (Cast<UK2Node_FunctionEntry>(HeadNode)) CallFunctionName = Cast<UK2Node_FunctionEntry>(HeadNode)->FunctionReference.GetMemberName();
			else CallFunctionName = FName(HeadNode->GetNodeTitle(ENodeTitleType::EditableTitle).ToString());
		}
	}

	// Get target function
	const UFunction* TargetFunction = GetTargetFunction();
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	// Print function
	UK2Node_CallFunction* PrintFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	PrintFunction->SetFromFunction(TargetFunction);
	PrintFunction->AllocateDefaultPins();

	// Local variable for print duration
	UK2Node_TemporaryVariable* PrintDurationNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
#if ENGINE_MAJOR_VERSION == 5
	PrintDurationNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Real;
	PrintDurationNode->VariableType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
#else
	PrintDurationNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Float;
#endif
	PrintDurationNode->AllocateDefaultPins();
	UEdGraphPin* PrintDurationOutPin = PrintDurationNode->GetVariablePin();
	check(PrintDurationOutPin);

	// Assignment statement to initialise local variable
	UK2Node_AssignmentStatement* PrintDurationInitialise = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	PrintDurationInitialise->AllocateDefaultPins();
	if (TickMethod == EK2NodePrintTickMethod::Tick)
	{
		const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
		PrintDurationInitialise->GetValuePin()->DefaultValue = Settings ? FString::SanitizeFloat(Settings->PrintDebugTickDuration) : TEXT("0.1");
	}
	else PrintDurationInitialise->GetValuePin()->DefaultValue = TEXT("-1");
	Schema->TryCreateConnection(PrintDurationOutPin, PrintDurationInitialise->GetVariablePin());
	UEdGraphPin* PrintDurationInitialiseExecPin = PrintDurationInitialise->GetExecPin();
	check(PrintDurationInitialiseExecPin);

	// Local variable for function call
	UK2Node_TemporaryVariable* FunctionCallingNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	FunctionCallingNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_String;
	FunctionCallingNode->AllocateDefaultPins();
	UEdGraphPin* FunctionCallOutPin = FunctionCallingNode->GetVariablePin();
	check(FunctionCallOutPin);

	// Assignment statement to initialise local variable
	UK2Node_AssignmentStatement* FunctionCallInitialise = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	FunctionCallInitialise->AllocateDefaultPins();
	FunctionCallInitialise->GetValuePin()->DefaultValue = CallFunctionName.ToString();
	Schema->TryCreateConnection(FunctionCallOutPin, FunctionCallInitialise->GetVariablePin());
	
	// Get auto world context pin in function library
	if (SourceGraph && Schema->IsStaticFunctionGraph(SourceGraph) && PrintFunction)
	{
		TArray<UK2Node_FunctionEntry*> EntryPoints;
		SourceGraph->GetNodesOfClass(EntryPoints);
		if (1 != EntryPoints.Num())
		{
			CompilerContext.MessageLog.Warning(*FText::Format(LOCTEXT("WrongEntryPointsNumFmt", "{0} entry points found while expanding node @@"), EntryPoints.Num()).ToString(), this);
		}

		UEdGraphPin* AutoWorldContextPin = EntryPoints[0]->GetAutoWorldContextPin();
		if (AutoWorldContextPin)
		{
			UEdGraphPin* PrintFunctionWorldContext = PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::WorldContextPin);
			const bool bConnected = Schema->TryCreateConnection(PrintFunctionWorldContext, AutoWorldContextPin);
			if (!bConnected)
			{
				CompilerContext.MessageLog.Warning(*LOCTEXT("DefaultToSelfNotConnected", "DefaultToSelf pin @@ from node @@ cannot be connected to @@").ToString(), PrintFunctionWorldContext, this, AutoWorldContextPin);
			}
		}
	}
	
	if (PinNames.Num() <= 0)
	{
		// Connect all pins
		CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *PrintDurationInitialiseExecPin);
		Schema->TryCreateConnection(PrintDurationInitialise->GetThenPin(), FunctionCallInitialise->GetExecPin());
		Schema->TryCreateConnection(FunctionCallInitialise->GetThenPin(), PrintFunction->GetExecPin());
		Schema->TryCreateConnection(PrintDurationOutPin, PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::DurationPin, EGPD_Input));
		Schema->TryCreateConnection(FunctionCallOutPin, PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::CallingFunctionPin, EGPD_Input));
		if ((OverridenMethod == EK2NodePrintOverrideMethod::OverrideDuration && TickMethod == EK2NodePrintTickMethod::NoTick) && GetDurationPin()) CompilerContext.MovePinLinksToIntermediate(*GetDurationPin(), *PrintDurationInitialise->GetValuePin());
		CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *PrintFunction->FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output));
		CompilerContext.MovePinLinksToIntermediate(*GetStringPin(), *PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::StringPin, EGPD_Input));
	}
	else
	{
		// Spawn format node
		UK2Node_FormatText* FormatTextNode = CompilerContext.SpawnIntermediateNode<UK2Node_FormatText>(this, SourceGraph);
		FormatTextNode->AllocateDefaultPins();
		for (int32 i = 0; i < PinNames.Num(); i++) FormatTextNode->AddArgumentPin(); // Add an argument pin on the format node for each of the pin names of the print node

		// Spawn string to text node
		UK2Node_CallFunction* StringToTextNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		const UFunction* StringToTextFunction = UKismetTextLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
		StringToTextNode->SetFromFunction(StringToTextFunction);
		StringToTextNode->AllocateDefaultPins();

		// Spawn text to string node
		UK2Node_CallFunction* TextToStringNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		const UFunction* TextToStringFunction = UKismetTextLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_TextToString));
		TextToStringNode->SetFromFunction(TextToStringFunction);
		TextToStringNode->AllocateDefaultPins();

		CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *PrintDurationInitialiseExecPin);
		Schema->TryCreateConnection(PrintDurationInitialise->GetThenPin(), FunctionCallInitialise->GetExecPin());
		Schema->TryCreateConnection(FunctionCallInitialise->GetThenPin(), PrintFunction->GetExecPin());
		Schema->TryCreateConnection(PrintDurationOutPin, PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::DurationPin, EGPD_Input));
		Schema->TryCreateConnection(FunctionCallOutPin, PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::CallingFunctionPin, EGPD_Input));
		if ((OverridenMethod == EK2NodePrintOverrideMethod::OverrideDuration && TickMethod == EK2NodePrintTickMethod::NoTick) && GetDurationPin()) CompilerContext.MovePinLinksToIntermediate(*GetDurationPin(), *PrintDurationInitialise->GetValuePin());
		CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *PrintFunction->FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output));

		// Connect the string pin to the string to text in string pin
		CompilerContext.MovePinLinksToIntermediate(*GetStringPin(), *StringToTextNode->FindPinChecked(FName("InString"), EGPD_Input));

		// Connect the string to text output pin to the format text pin
		Schema->TryCreateConnection(StringToTextNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output), FormatTextNode->GetFormatPin());

		// Make sure all the internal formatted text node argument pins are named exactly as the print node
		for (const FName ArgumentPinName : GetArgumentNames())
		{
			FormatTextNode->SetArgumentName(GetArgumentIndex(ArgumentPinName), ArgumentPinName);
		}

		// Move all the arguments pins to the intermediate format text node
		for (const FName ArgumentPinName : GetArgumentNames())
		{
			UEdGraphPin* ArgumentPin = FindPinChecked(ArgumentPinName, EGPD_Input);
			UEdGraphPin* FormattedNodeArgumentPin = FormatTextNode->FindPinChecked(ArgumentPinName, EGPD_Input);
			CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *FormattedNodeArgumentPin);
		}

		// Reconstruct the internal format text node to update the pin types
		FormatTextNode->ReconstructNode();

		// Connect the format result pin to the text to string pin
		Schema->TryCreateConnection(FormatTextNode->FindPinChecked(FName("Result"), EGPD_Output), TextToStringNode->FindPinChecked(FName("InText"), EGPD_Input));

		// Connect the text to string output pin to the actual print string pin
		Schema->TryCreateConnection(TextToStringNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue), PrintFunction->FindPinChecked(K2Node_PrintNodeHelper::StringPin, EGPD_Input));
	}

	BreakAllNodeLinks();
}

UEdGraphNode* UK2Node_Print::GetHeadNode(UEdGraphPin* Pin)
{
	if (MaxGetHeadNodeRecursiveIterations <= 0) return nullptr;
	MaxGetHeadNodeRecursiveIterations--;

	if (Pin)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			UEdGraphPin* LinkedPin;
			if (Pin->LinkedTo.Num() > 1)
			{
				// Check that the pin connected to the current pin doesn't end up creating a circular dependence.
				// This can happen if the pin ultimately ends up leading back to the same node this pin belongs to
				// You can see an example of this in the ForEachLoop and ForLoop macros in blueprints.
				if (IsPinCircularDependent(Pin->LinkedTo[0])) LinkedPin = Pin->LinkedTo[1];
				else LinkedPin = Pin->LinkedTo[0];
			}
			else
			{
				LinkedPin = Pin->LinkedTo[0];
			}

			UEdGraphNode* GraphNode = LinkedPin->GetOwningNode();

			// Check if this pin's node is an input site node, this means we are at the beginning of a macro expansion graph. E.g. ForLoop entry
			UK2Node_TunnelBoundary* TunnelBoundaryInput = Cast<UK2Node_TunnelBoundary>(Pin->GetOwningNode());
			if (TunnelBoundaryInput && TunnelBoundaryInput->GetTunnelBoundaryType() == ETunnelBoundaryType::InputSite)
			{
				if (TunnelBoundaryInput->Pins[0] && TunnelBoundaryInput->Pins[0]->LinkedTo[0] && TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode() && TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0])
				{
					return GetHeadNode(TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0]);
				}
			}

			// Check if the linked pin's node is an input site node, this means we are at the beginning of a macro expansion graph. E.g. ForLoop entry
			TunnelBoundaryInput = Cast<UK2Node_TunnelBoundary>(GraphNode);
			if (TunnelBoundaryInput && TunnelBoundaryInput->GetTunnelBoundaryType() == ETunnelBoundaryType::InputSite)
			{
				if (TunnelBoundaryInput->Pins[0] && TunnelBoundaryInput->Pins[0]->LinkedTo[0] && TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode() && TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0])
				{
					UK2Node_TunnelBoundary* ConnectedTunnelBoundary = Cast<UK2Node_TunnelBoundary>(TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0]->LinkedTo[0]->GetOwningNode());
					if (ConnectedTunnelBoundary && ConnectedTunnelBoundary->GetTunnelBoundaryType() == ETunnelBoundaryType::OutputSite)
					{
						GLog->Log("Tunnel Boundary");
						if (ConnectedTunnelBoundary->Pins[0] && ConnectedTunnelBoundary->Pins[0]->LinkedTo[0] && ConnectedTunnelBoundary->Pins[0]->LinkedTo[0]->GetOwningNode())
						{
							// If this pin is valid we attempt to still walk through the macro by jumping from the output side node to the internals
							// of the macro.
							UEdGraphPin* MacroNodeExecPin = ConnectedTunnelBoundary->Pins[0]->LinkedTo[0]->GetOwningNode()->FindPin(UEdGraphSchema_K2::PN_Execute);
							if (MacroNodeExecPin) return GetHeadNode(MacroNodeExecPin);
						}
					}

					const UEdGraphPin* LinkedExecutionPin = TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0]->LinkedTo[0]->GetOwningNode()->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (LinkedExecutionPin) return GetHeadNode(TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0]);

					return TunnelBoundaryInput->Pins[0]->LinkedTo[0]->GetOwningNode()->Pins[0]->LinkedTo[0]->GetOwningNode();
				}
			}

			// If we can find the execution pin of the node keep walking up the chain of nodes
			UEdGraphPin* ConnectedNodeExecPin = GraphNode->FindPin(UEdGraphSchema_K2::PN_Execute);
			if (ConnectedNodeExecPin)
			{
				if (ConnectedNodeExecPin->LinkedTo.Num() > 0) return GetHeadNode(ConnectedNodeExecPin);
			}

			// Check if the node we are connected to is a tunnel output site, this means we are at the end of a macro expansion graph. E.g. ForLoop output
			const UK2Node_TunnelBoundary* TunnelBoundaryOutput = Cast<UK2Node_TunnelBoundary>(GraphNode);
			if (TunnelBoundaryOutput && TunnelBoundaryOutput->GetTunnelBoundaryType() == ETunnelBoundaryType::OutputSite)
			{
				if (TunnelBoundaryOutput->Pins[0] && TunnelBoundaryOutput->Pins[0]->LinkedTo[0] && TunnelBoundaryOutput->Pins[0]->LinkedTo[0]->GetOwningNode())
				{
					// If this pin is valid we attempt to still walk through the macro by jumping from the output side node to the internals
					// of the macro.
					UEdGraphPin* MacroNodeExecPin = TunnelBoundaryOutput->Pins[0]->LinkedTo[0]->GetOwningNode()->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (MacroNodeExecPin) return GetHeadNode(MacroNodeExecPin);
				}
			}

			return GraphNode;
		}
	}

	return nullptr;
}

void UK2Node_Print::AddInputPin()
{
	if(UEdGraphPin* StringPin = GetStringPin())
	{
		const FString StringPinValue = StringPin->DefaultValue;
		const int32 NewArgumentIndex = GetArgumentCount();
		const FString NewArgumentString = StringPinValue.IsEmpty() ? "{" + FString::FromInt(NewArgumentIndex) + "}" : " {" + FString::FromInt(NewArgumentIndex) + "}";
		StringPin->DefaultValue = StringPinValue + NewArgumentString;
	}

	PinDefaultValueChanged(GetStringPin());
}

UEdGraphPin* UK2Node_Print::GetExecPinFromThenPin(const UEdGraphPin* ThenPin)
{
	if (ThenPin)
	{
		if (const UEdGraphNode* GraphNode = ThenPin->GetOwningNode())
		{
			if (UEdGraphPin* ExecPin = GraphNode->FindPin(UEdGraphSchema_K2::PN_Execute))
			{
				return ExecPin;
			}
		}
	}

	return nullptr;
}

FString UK2Node_Print::GetNodeTitle(UEdGraphNode* Node)
{
	if (Node)
	{
		return Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
	}
	return "";
}

bool UK2Node_Print::IsPinCircularDependent(UEdGraphPin* Pin)
{
	if (Pin && Pin->GetOwningNode() && Pin->LinkedTo.IsValidIndex(0))
	{
		UEdGraphPin* CurrentPin = Pin;
		for (int32 i = 0; i < 100; i++)
		{
			if (CurrentPin && CurrentPin->LinkedTo.IsValidIndex(0))
			{
				if (CurrentPin->LinkedTo[0]->GetOwningNode() && CurrentPin->LinkedTo[0]->GetOwningNode() == Pin->GetOwningNode()) return true;

				CurrentPin = CurrentPin->LinkedTo[0];
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

FText UK2Node_Print::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::MenuTitle)
	{
		return LOCTEXT("K2Print_MenuNodeTitle", "DFL Print");
	}
	else
	{
		if (NodePrintState == EK2NodePrintState::PrintLog)
		{
			return LOCTEXT("K2Print_NodeTitle", "Print & Log");
		}
		else if (NodePrintState == EK2NodePrintState::Print)
		{
			return LOCTEXT("K2Print_NodeTitle", "Print");
		}
		else
		{
			return LOCTEXT("K2Print_NodeTitle", "Log");
		}
	}
}

FText UK2Node_Print::GetTooltipText() const
{
	const FTextFormat TextFormat(FText::FromString("DFL Print let's you print and log choosing the print state and print level.\nYou can print a formatted string by using {} to denote format arguments. E.g. \"Print variable value {5}.\"\nArgument types may be Byte, Integer, Float, Text, String, Name, Boolean, Object or ETextGender.\n\nPrint State: {1} or {0}.\nPrint Level: {2} or {0}.\nOverride Duration: {3} or {0}.\nTick Mode: {6} or {0}\nEnable/Disable: {4} or {0}"));
	const TSharedRef<const FInputChord> PrintStateInputChord = FDFLEditorCommands::Get().UpdateNodePrintState->GetActiveChord(EMultipleKeyBindingIndex::Primary);
	const TSharedRef<const FInputChord> PrintLevelInputChord = FDFLEditorCommands::Get().UpdateNodePrintLevel->GetActiveChord(EMultipleKeyBindingIndex::Primary);
	const TSharedRef<const FInputChord> ToggleNodeOverrideDurationInputChord = FDFLEditorCommands::Get().ToggleNodeDurationState->GetActiveChord(EMultipleKeyBindingIndex::Primary);
	const TSharedRef<const FInputChord> ToggleEnableInputChord = FDFLEditorCommands::Get().UpdateNodeEnableState->GetActiveChord(EMultipleKeyBindingIndex::Primary);
	const TSharedRef<const FInputChord> ToggleTickModeInputChord = FDFLEditorCommands::Get().ToggleNodeTickMethod->GetActiveChord(EMultipleKeyBindingIndex::Primary);
	const FText FormatExample = FText::FromString("{0}");
	return FText::Format(TextFormat, FText::FromString(FString("right click node")), PrintStateInputChord->GetInputText(), PrintLevelInputChord->GetInputText(), ToggleNodeOverrideDurationInputChord->GetInputText(), ToggleEnableInputChord->GetInputText(), FormatExample, ToggleTickModeInputChord->GetInputText());
}

FSlateIcon UK2Node_Print::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = Super::GetNodeTitleColor();
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FName UK2Node_Print::GetCornerIcon() const
{
	if (TickMethod == EK2NodePrintTickMethod::Tick)
	{
		return TEXT("Graph.Node.Loop");
	}

	return Super::GetCornerIcon();
}

FLinearColor UK2Node_Print::GetNodeTitleColor() const
{
	if (NodePrintLevel == EK2NodePrintLevel::Message)
	{
		return Super::GetNodeTitleColor();
	}
	else if (NodePrintLevel == EK2NodePrintLevel::Warning)
	{
		return FLinearColor::Yellow;
	}
	else
	{
		return FLinearColor::Red;
	}
}

void UK2Node_Print::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	if (Pin.PinName == K2Node_PrintNodeHelper::DurationPin)
	{
		HoverTextOut = "The overriden duration of the print.";
		return;
	}

	if (Pin.PinName == K2Node_PrintNodeHelper::StringPin)
	{
		HoverTextOut = "The string to print. You can format the string by using {}. E.g. \"Print variable value {0}\"";
		return;
	}

	Super::GetPinHoverText(Pin, HoverTextOut);
}

void UK2Node_Print::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const UClass* ActionKey = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

UK2Node::ERedirectType UK2Node_Print::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	ERedirectType RedirectType = ERedirectType_None;

	// if the pin names do match
	if (NewPin->PinName.ToString().Equals(OldPin->PinName.ToString(), ESearchCase::CaseSensitive))
	{
		// Make sure we're not dealing with a menu node
		UEdGraph* OuterGraph = GetGraph();
		if (OuterGraph && OuterGraph->Schema)
		{
			const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
			if (!K2Schema || K2Schema->IsSelfPin(*NewPin) || K2Schema->ArePinTypesCompatible(OldPin->PinType, NewPin->PinType))
			{
				RedirectType = ERedirectType_Name;
			}
			else
			{
				RedirectType = ERedirectType_None;
			}
		}
	}
	else
	{
		// try looking for a redirect if it's a K2 node
		if (const UK2Node* Node = Cast<UK2Node>(NewPin->GetOwningNode()))
		{
			// if you don't have matching pin, now check if there is any redirect param set
			TArray<FString> OldPinNames;
			GetRedirectPinNames(*OldPin, OldPinNames);

			FName NewPinName;
			RedirectType = ShouldRedirectParam(OldPinNames, /*out*/ NewPinName, Node);

			// make sure they match
			if ((RedirectType != ERedirectType_None) && (!NewPin->PinName.ToString().Equals(NewPinName.ToString(), ESearchCase::CaseSensitive)))
			{
				RedirectType = ERedirectType_None;
			}
		}
	}

	return RedirectType;
}

bool UK2Node_Print::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// Argument input pins may only be connected to Byte, Integer, Float, Text, and ETextGender pins...
	const UEdGraphPin* FormatPin = GetStringPin();
	if (MyPin != FormatPin && MyPin != GetDurationPin() && MyPin != GetExecPin() && MyPin->Direction == EGPD_Input)
	{
		const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
		const FName& OtherPinCategory = OtherPin->PinType.PinCategory;

		FName FloatCategory;
#if ENGINE_MAJOR_VERSION == 5
		FloatCategory = UEdGraphSchema_K2::PC_Real;
#else
		FloatCategory = UEdGraphSchema_K2::PC_Float;
#endif

		bool bIsValidType = false;
		if (OtherPinCategory == UEdGraphSchema_K2::PC_Int || OtherPinCategory == FloatCategory || OtherPinCategory == UEdGraphSchema_K2::PC_Text ||
			(OtherPinCategory == UEdGraphSchema_K2::PC_Byte && !OtherPin->PinType.PinSubCategoryObject.IsValid()) ||
			OtherPinCategory == UEdGraphSchema_K2::PC_Boolean || OtherPinCategory == UEdGraphSchema_K2::PC_String || OtherPinCategory == UEdGraphSchema_K2::PC_Name || OtherPinCategory == UEdGraphSchema_K2::PC_Object)
		{
			bIsValidType = true;
		}
		else if (OtherPinCategory == UEdGraphSchema_K2::PC_Byte || OtherPinCategory == UEdGraphSchema_K2::PC_Enum)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			static UEnum* TextGenderEnum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/Engine.ETextGender"), /*ExactClass*/true);
#else
			static UEnum* TextGenderEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ETextGender"), /*ExactClass*/true);
#endif
			if (OtherPin->PinType.PinSubCategoryObject == TextGenderEnum)
			{
				bIsValidType = true;
			}
		}

		if (!bIsValidType)
		{
			OutReason = LOCTEXT("Error_InvalidArgumentType", "Format arguments may only be Byte, Integer, Float, Text, String, Name, Boolean, Object, or ETextGender.").ToString();
			return true;
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

FText UK2Node_Print::GetMenuCategory() const
{
	return LOCTEXT("K2Print_MenuCategory", "Debug Function Library | Print Debug");
}

void UK2Node_Print::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	RefreshNode();

	UEdGraphPin* FormatPin = GetStringPin();

	Modify();

	// Clear all pins.
	if (Pin == FormatPin && !FormatPin->DefaultTextValue.IsEmpty())
	{
		PinNames.Empty();
		GetSchema()->TrySetDefaultText(*FormatPin, FText::GetEmpty());

		for (auto It = Pins.CreateConstIterator(); It; ++It)
		{
			UEdGraphPin* CheckPin = *It;
			if (CheckPin != FormatPin && CheckPin != GetDurationPin() && CheckPin->Direction == EGPD_Input)
			{
				CheckPin->Modify();
#if ENGINE_MAJOR_VERSION == 5
				CheckPin->MarkAsGarbage();
#else
				CheckPin->MarkPendingKill();
#endif
				Pins.Remove(CheckPin);
				--It;
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}

	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);
}

void UK2Node_Print::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	const UEdGraphPin* FormatPin = GetStringPin();
	if (Pin == FormatPin && FormatPin->LinkedTo.Num() == 0)
	{
		TArray<FString> ArgumentParams;
		FText::GetFormatPatternParameters(FText::FromString(FormatPin->GetDefaultAsString()), ArgumentParams);

		PinNames.Reset();

		for (const FString& Param : ArgumentParams)
		{
			const FName ParamName(*Param);
			if (!FindArgumentPin(ParamName))
			{
				CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ParamName);
			}
			PinNames.Add(ParamName);
		}

		for (auto It = Pins.CreateIterator(); It; ++It)
		{
			UEdGraphPin* CheckPin = *It;
			if (CheckPin != GetExecPin() && CheckPin != FormatPin && CheckPin != GetDurationPin() && CheckPin->Direction == EGPD_Input)
			{
				const bool bIsValidArgPin = ArgumentParams.ContainsByPredicate([&CheckPin](const FString& InPinName)
				{
					return InPinName.Equals(CheckPin->PinName.ToString(), ESearchCase::CaseSensitive);
				});

				if (!bIsValidArgPin)
				{
#if ENGINE_MAJOR_VERSION == 5
				CheckPin->MarkAsGarbage();
#else
					CheckPin->MarkPendingKill();
#endif
					It.RemoveCurrent();
				}
			}
		}

		GetGraph()->NotifyGraphChanged();
	}
}

void UK2Node_Print::PinTypeChanged(UEdGraphPin* Pin)
{
	// Potentially update an argument pin type
	SynchronizeArgumentPinType(Pin);

	Super::PinTypeChanged(Pin);
}

void UK2Node_Print::RefreshNode()
{
	UpdateFunctionName();
	Modify();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_Print::PostReconstructNode()
{
	Super::PostReconstructNode();

	for (const FName PinName : PinNames)
	{
		UEdGraphPin* Pin = FindPin(PinName, EGPD_Input);
		if (Pin) SynchronizeArgumentPinType(Pin);
	}
}

void UK2Node_Print::CycleNodePrintLevel()
{
	uint8 NodePrintLevelValue = static_cast<uint8>(NodePrintLevel);
	if (NodePrintLevelValue > GetNodePrintStateEnumSize() - 2)
	{
		NodePrintLevelValue = 0;
	}
	else
	{
		NodePrintLevelValue++;
	}

	NodePrintLevel = static_cast<EK2NodePrintLevel>(NodePrintLevelValue);

	RefreshNode();
}

void UK2Node_Print::CycleNodePrintState()
{
	uint8 PrintStateValue = static_cast<uint8>(NodePrintState);
	if (PrintStateValue > GetNodePrintStateEnumSize() - 2) PrintStateValue = 0;
	else PrintStateValue++;

	NodePrintState = static_cast<EK2NodePrintState>(PrintStateValue);

	RefreshNode();
}

void UK2Node_Print::UpdateFunctionName()
{
	if (NodePrintLevel == EK2NodePrintLevel::Message)
	{
		if (NodePrintState == EK2NodePrintState::PrintLog) TargetFunctionName = "DFLQuickPrintLogMessage";
		else if (NodePrintState == EK2NodePrintState::Print) TargetFunctionName = "DFLQuickPrintMessage";
		else if (NodePrintState == EK2NodePrintState::Log) TargetFunctionName = "DFLQuickLogMessage";
	}

	if (NodePrintLevel == EK2NodePrintLevel::Warning)
	{
		if (NodePrintState == EK2NodePrintState::PrintLog) TargetFunctionName = "DFLQuickPrintLogWarning";
		else if (NodePrintState == EK2NodePrintState::Print) TargetFunctionName = "DFLQuickPrintWarning";
		else if (NodePrintState == EK2NodePrintState::Log) TargetFunctionName = "DFLQuickLogWarning";
	}

	if (NodePrintLevel == EK2NodePrintLevel::Error)
	{
		if (NodePrintState == EK2NodePrintState::PrintLog) TargetFunctionName = "DFLQuickPrintLogError";
		else if (NodePrintState == EK2NodePrintState::Print) TargetFunctionName = "DFLQuickPrintError";
		else if (NodePrintState == EK2NodePrintState::Log) TargetFunctionName = "DFLQuickLogError";
	}
}

void UK2Node_Print::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	// Set the node to development only if the function specifies that
	check(!HasUserSetTheEnabledState());

	SetEnabledState(ENodeEnabledState::DevelopmentOnly, false);
}

#pragma region Formatting
FName UK2Node_Print::GetUniquePinName() const
{
	FName NewPinName;
	int32 i = 0;
	while (true)
	{
		NewPinName = *FString::FromInt(i++);
		if (!FindPin(NewPinName))
		{
			break;
		}
	}
	return NewPinName;
}

void UK2Node_Print::SynchronizeArgumentPinType(UEdGraphPin* Pin) const
{
	const UEdGraphPin* FormatPin = GetStringPin();
	if (Pin != FormatPin && Pin != GetExecPin() && Pin != GetDurationPin() && Pin->Direction == EGPD_Input)
	{
		const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());

		bool bPinTypeChanged = false;
		if (Pin->LinkedTo.Num() == 0)
		{
			static const FEdGraphPinType WildcardPinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Wildcard, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());

			// Ensure wildcard
			if (Pin->PinType != WildcardPinType)
			{
				Pin->PinType = WildcardPinType;
				bPinTypeChanged = true;
			}
		}
		else
		{
			const UEdGraphPin* ArgumentSourcePin = Pin->LinkedTo[0];

			// Take the type of the connected pin
			if (Pin->PinType != ArgumentSourcePin->PinType)
			{
				Pin->PinType = ArgumentSourcePin->PinType;
				bPinTypeChanged = true;
			}
		}

		if (bPinTypeChanged)
		{
			// Let the graph know to refresh
			GetGraph()->NotifyGraphChanged();

			UBlueprint* Blueprint = GetBlueprint();
			if (!Blueprint->bBeingCompiled)
			{
				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
				Blueprint->BroadcastChanged();
			}
		}
	}
}

UEdGraphPin* UK2Node_Print::FindArgumentPin(const FName InPinName) const
{
	const UEdGraphPin* FormatPin = GetStringPin();
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin != FormatPin && Pin != GetDurationPin() && Pin->Direction != EGPD_Output && Pin->PinName.ToString().Equals(InPinName.ToString(), ESearchCase::CaseSensitive))
		{
			return Pin;
		}
	}

	return nullptr;
}

void UK2Node_Print::SetArgumentName(int32 InIndex, FName InName)
{
	PinNames[InIndex] = InName;

	ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_Print::RemoveArgument(int32 InIndex)
{
	const FScopedTransaction Transaction(NSLOCTEXT("K2Node_Print", "RemoveArgumentPin", "Remove Argument Pin"));
	Modify();

	if (UEdGraphPin* ArgumentPin = FindArgumentPin(PinNames[InIndex]))
	{
		Pins.Remove(ArgumentPin);
#if ENGINE_MAJOR_VERSION == 5
		ArgumentPin->MarkAsGarbage();
#else
		ArgumentPin->MarkPendingKill();
#endif
	}
	PinNames.RemoveAt(InIndex);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	GetGraph()->NotifyGraphChanged();
}

FText UK2Node_Print::GetArgumentName(int32 InIndex) const
{
	if (InIndex < PinNames.Num())
	{
		return FText::FromName(PinNames[InIndex]);
	}
	return FText::GetEmpty();
}

TArray<FName> UK2Node_Print::GetArgumentNames()
{
	TArray<FName> ArgumentPins;

	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin != GetStringPin() && Pin != GetDurationPin() && Pin != GetExecPin() && Pin != FindPin(K2Node_PrintNodeHelper::WorldContextPin, EGPD_Input) && Pin->Direction == EGPD_Input)
		{
			ArgumentPins.Add(Pin->PinName);
		}
	}

	return ArgumentPins;
}

void UK2Node_Print::AddArgumentPin()
{
	const FScopedTransaction Transaction(NSLOCTEXT("K2Node_Print", "AddArgumentPin", "Add Argument Pin"));
	Modify();

	const FName PinName(GetUniquePinName());
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinName);
	PinNames.Add(PinName);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	GetGraph()->NotifyGraphChanged();
}

int32 UK2Node_Print::GetArgumentIndex(const FName PinName)
{
	TArray<UEdGraphPin*> ArgumentPins;
	for (int32 i = 0; i < GetArgumentNames().Num(); i++)
	{
		if (GetArgumentNames()[i] == PinName) return i;
	}

	return INDEX_NONE;
}
#pragma endregion

#pragma region GetFunctionPins
UFunction* UK2Node_Print::GetTargetFunction() const
{
	return UDFLDebugFL::StaticClass()->FindFunctionByName(TargetFunctionName);
}

UEdGraphPin* UK2Node_Print::GetThenPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
}

UEdGraphPin* UK2Node_Print::GetStringPin() const
{
	return FindPinChecked(K2Node_PrintNodeHelper::StringPin, EGPD_Input);
}

UEdGraphPin* UK2Node_Print::GetDurationPin() const
{
	return FindPin(K2Node_PrintNodeHelper::DurationPin, EGPD_Input);
}
#pragma endregion

#pragma region ContextMenuAction
void UK2Node_Print::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	const FToolMenuInsert PrintNodeActionsMenuInsert("EdGraphSchemaNodeActions", EToolMenuInsertType::Before);
	FToolMenuSection& PrintNodeMenuActionSection = Menu->AddSection("K2NodePrintMenuAction", LOCTEXT("PrintNodeMenuActionHeader", "Print Node Actions"), PrintNodeActionsMenuInsert);
	PrintNodeMenuActionSection.AddSubMenu("Print Node Actions", LOCTEXT("PrintNodeActionsHeader", "Print Node Actions"), FText(), FNewToolMenuDelegate::CreateLambda([&](UToolMenu* PrintNodeActionMenu)
	{
		FToolMenuSection& PrintNodeActionSection = PrintNodeActionMenu->AddSection("K2NodePrintNodeAction", LOCTEXT("PrintNodeActionHeader", "Print Node Actions"));
		FToolMenuSection& PrintNodeStateSection = PrintNodeActionMenu->AddSection("K2NodePrintState", LOCTEXT("PrintNodeStateHeader", "Print Node State"));
		FToolMenuSection& PrintNodeLevelSection = PrintNodeActionMenu->AddSection("K2NodePrintLevel", LOCTEXT("PrintNodeLevelHeader", "Print Node Level"));

		if (OverridenMethod == EK2NodePrintOverrideMethod::OverrideDuration) const_cast<UK2Node_Print*>(this)->CreateNodeDisableDuration(PrintNodeActionSection);
		else const_cast<UK2Node_Print*>(this)->CreateNodeEnableDuration(PrintNodeActionSection);

		if (TickMethod == EK2NodePrintTickMethod::Tick)
		{
			const_cast<UK2Node_Print*>(this)->CreateNodeNoTickEnable(PrintNodeActionSection);
		}
		else
		{
			const_cast<UK2Node_Print*>(this)->CreateNodeTickEnable(PrintNodeActionSection);
		}

		if (GetDesiredEnabledState() == ENodeEnabledState::DevelopmentOnly)
		{
			const_cast<UK2Node_Print*>(this)->CreateNodeDisableState(PrintNodeActionSection);
		}
		else
		{
			const_cast<UK2Node_Print*>(this)->CreateNodeEnableState(PrintNodeActionSection);
		}

		if (NodePrintState == EK2NodePrintState::PrintLog)
		{
			const_cast<UK2Node_Print*>(this)->CreateNodePrintState(PrintNodeStateSection);
			const_cast<UK2Node_Print*>(this)->CreateNodeLogState(PrintNodeStateSection);
		}
		else if (NodePrintState == EK2NodePrintState::Print)
		{
			const_cast<UK2Node_Print*>(this)->CreateNodePrintLogState(PrintNodeStateSection);
			const_cast<UK2Node_Print*>(this)->CreateNodeLogState(PrintNodeStateSection);
		}
		else
		{
			const_cast<UK2Node_Print*>(this)->CreateNodePrintLogState(PrintNodeStateSection);
			const_cast<UK2Node_Print*>(this)->CreateNodePrintState(PrintNodeStateSection);
		}

		if (NodePrintLevel == EK2NodePrintLevel::Message)
		{
			const_cast<UK2Node_Print*>(this)->CreateNodePrintWarningLevel(PrintNodeLevelSection);
			const_cast<UK2Node_Print*>(this)->CreateNodePrintErrorLevel(PrintNodeLevelSection);
		}
		else if (NodePrintLevel == EK2NodePrintLevel::Warning)
		{
			const_cast<UK2Node_Print*>(this)->CreateNodePrintMessageLevel(PrintNodeLevelSection);
			const_cast<UK2Node_Print*>(this)->CreateNodePrintErrorLevel(PrintNodeLevelSection);
		}
		else
		{
			const_cast<UK2Node_Print*>(this)->CreateNodePrintMessageLevel(PrintNodeLevelSection);
			const_cast<UK2Node_Print*>(this)->CreateNodePrintWarningLevel(PrintNodeLevelSection);
		}
	}));
}

void UK2Node_Print::CreateNodePrintMessageLevel(FToolMenuSection& Section)
{
	const FText MessagePrintLevelMenuEntryTitle = LOCTEXT("MessagePrintLevelTitle", "Print Level Message");
	const FText MessagePrintLevelMenuEntryTooltip = LOCTEXT("MessagePrintLevelTooltip", "Set the print level of this node to message.");

	Section.AddMenuEntry(
		"Message Print Level",
		MessagePrintLevelMenuEntryTitle,
		MessagePrintLevelMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateNodePrintLevel(EK2NodePrintLevel::Message);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodePrintWarningLevel(FToolMenuSection& Section)
{
	const FText WarningPrintLevelMenuEntryTitle = LOCTEXT("WarningPrintLevelTitle", "Print Level Warning");
	const FText WarningPrintLevelMenuEntryTooltip = LOCTEXT("WarningPrintLevelTooltip", "Set the print level of this node to warning.");

	Section.AddMenuEntry(
		"Warning Print Level",
		WarningPrintLevelMenuEntryTitle,
		WarningPrintLevelMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateNodePrintLevel(EK2NodePrintLevel::Warning);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodePrintErrorLevel(FToolMenuSection& Section)
{
	const FText ErrorPrintLevelMenuEntryTitle = LOCTEXT("ErrorPrintLevelTitle", "Print Level Error");
	const FText ErrorPrintLevelMenuEntryTooltip = LOCTEXT("ErrorPrintLevelTooltip", "Set the print level of this node to error.");

	Section.AddMenuEntry(
		"Error Print Level",
		ErrorPrintLevelMenuEntryTitle,
		ErrorPrintLevelMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateNodePrintLevel(EK2NodePrintLevel::Error);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodePrintLogState(FToolMenuSection& Section)
{
	const FText PrintLogStateMenuEntryTitle = LOCTEXT("PrintLogStateTitle", "Print & Log State");
	const FText PrintLogStateMenuEntryTooltip = LOCTEXT("PrintLogStateTooltip", "Set the node level to print and log.");

	Section.AddMenuEntry(
		"Print Log State",
		PrintLogStateMenuEntryTitle,
		PrintLogStateMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateNodePrintState(EK2NodePrintState::PrintLog);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodePrintState(FToolMenuSection& Section)
{
	const FText PrintStateMenuEntryTitle = LOCTEXT("PrintStateTitle", "Print State");
	const FText PrintStateMenuEntryTooltip = LOCTEXT("PrintStateTooltip", "Set the node level to print.");

	Section.AddMenuEntry(
		"Print State",
		PrintStateMenuEntryTitle,
		PrintStateMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateNodePrintState(EK2NodePrintState::Print);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeLogState(FToolMenuSection& Section)
{
	const FText LogStateMenuEntryTitle = LOCTEXT("LogStateTitle", "Log State");
	const FText LogStateMenuEntryTooltip = LOCTEXT("LogStateTooltip", "Set the node level to log.");

	Section.AddMenuEntry(
		"Log State",
		LogStateMenuEntryTitle,
		LogStateMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateNodePrintState(EK2NodePrintState::Log);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeEnableState(FToolMenuSection& Section)
{
	const FText EnableStateMenuEntryTitle = LOCTEXT("EnableStateTitle", "Enable");
	const FText EnableStateMenuEntryTooltip = LOCTEXT("EnableStateTooltip", "Enable the node.");

	Section.AddMenuEntry(
		"Enable State",
		EnableStateMenuEntryTitle,
		EnableStateMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				SetEnabledState(ENodeEnabledState::DevelopmentOnly);
				RefreshNode();
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeDisableState(FToolMenuSection& Section)
{
	const FText DisableStateMenuEntryTitle = LOCTEXT("DisableStateTitle", "Disable");
	const FText DisableStateMenuEntryTooltip = LOCTEXT("DisableStateTooltip", "Disable the node.");

	Section.AddMenuEntry(
		"Disable State",
		DisableStateMenuEntryTitle,
		DisableStateMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				SetEnabledState(ENodeEnabledState::Disabled);
				RefreshNode();
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeEnableDuration(FToolMenuSection& Section)
{
	const FText EnableDurationMenuEntryTitle = LOCTEXT("EnableDurationTitle", "Override Print Duration");
	const FText EnableDurationMenuEntryTooltip = LOCTEXT("EnableDurationTooltip", "Override the print node duration instead of using the debug properties' one.");

	Section.AddMenuEntry(
		"Enable Print Duration",
		EnableDurationMenuEntryTitle,
		EnableDurationMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				OverridenMethod = EK2NodePrintOverrideMethod::OverrideDuration;
				ReconstructNode();
				RefreshNode();
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeDisableDuration(FToolMenuSection& Section)
{
	const FText DisableDurationMenuEntryTitle = LOCTEXT("DisableDurationTitle", "Use debug properties print duration");
	const FText DisableDurationMenuEntryTooltip = LOCTEXT("DisableDurationTooltip", "Use the default debug properties print duration.");

	Section.AddMenuEntry(
		"Disable Print Duration",
		DisableDurationMenuEntryTitle,
		DisableDurationMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				OverridenMethod = EK2NodePrintOverrideMethod::NoOverriden;
				RemovePin(GetDurationPin());
				ReconstructNode();
				RefreshNode();
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeTickEnable(FToolMenuSection& Section)
{
	const FText TickMethodMenuEntryTitle = LOCTEXT("TickMethodTitle", "Run on tick");
	const FText TickMethodMenuEntryTooltip = LOCTEXT("TickMethodTooltip", "Changes this node tick mode to run on tick.");

	Section.AddMenuEntry(
		"Run on tick",
		TickMethodMenuEntryTitle,
		TickMethodMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateTickMethod(EK2NodePrintTickMethod::Tick);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::CreateNodeNoTickEnable(FToolMenuSection& Section)
{
	const FText NoTickMethodMenuEntryTitle = LOCTEXT("NoTickMethodTitle", "Don't run on tick");
	const FText NoTickMethodMenuEntryTooltip = LOCTEXT("NoTickMethodTooltip", "Changes this node tick method to no tick.");

	Section.AddMenuEntry(
		"Don't run on tick",
		NoTickMethodMenuEntryTitle,
		NoTickMethodMenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([&]()
			{
				UpdateTickMethod(EK2NodePrintTickMethod::NoTick);
			}),
			FCanExecuteAction(),
			FIsActionChecked())
	);
}

void UK2Node_Print::UpdateNodePrintLevel(const EK2NodePrintLevel NewNodePrintLevel)
{
	NodePrintLevel = NewNodePrintLevel;
	RefreshNode();
}

void UK2Node_Print::UpdateNodePrintState(const EK2NodePrintState NewNodePrintState)
{
	NodePrintState = NewNodePrintState;
	RefreshNode();
}

void UK2Node_Print::UpdateTickMethod(const EK2NodePrintTickMethod NewTickMethod)
{
	TickMethod = NewTickMethod;
	if (TickMethod == EK2NodePrintTickMethod::Tick && GetDurationPin()) GetDurationPin()->bHidden = true;
	else if (TickMethod == EK2NodePrintTickMethod::NoTick && GetDurationPin()) GetDurationPin()->bHidden = false;
	RefreshNode();
}

void UK2Node_Print::SetOverrideMethod(EK2NodePrintOverrideMethod InOverrideMethod)
{
	if (OverridenMethod == EK2NodePrintOverrideMethod::OverrideDuration)
	{
		TickMethod = EK2NodePrintTickMethod::NoTick;
		RemovePin(GetDurationPin());
	}

	OverridenMethod = InOverrideMethod;
	ReconstructNode();
	RefreshNode();
}

int32 UK2Node_Print::GetNodePrintStateEnumSize() const
{
	const FString EnumName = "/Script/DebugFunctionLibraryLiteUncooked.EK2NodePrintState";
	UEnum* Enum = GetEnumFromName(EnumName);
	check(Enum != nullptr);
	return Enum->GetMaxEnumValue();
}

int32 UK2Node_Print::GetNodePrintLevelEnumSize() const
{
	const FString EnumName = "/Script/DebugFunctionLibraryLiteUncooked.EK2NodePrintLevel";
	UEnum* Enum = GetEnumFromName(EnumName);
	check(Enum != nullptr)
	return Enum->GetMaxEnumValue();
}

UEnum* UK2Node_Print::GetEnumFromName(FString EnumName) const
{
	return FindObject<UEnum>(nullptr, *EnumName);
}
#pragma endregion

#undef LOCTEXT_NAMESPACE
