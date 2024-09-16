// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DebugFunctionLibrarySettings.generated.h"

UENUM()
enum class EPrintLogPrefix
{
	ClassFunction,
	Class,
	Function,
	None
};

UCLASS(Config=Game, DefaultConfig)
class DEBUGFUNCTIONLIBRARYLITE_API UDebugFunctionLibrarySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
#pragma region Globals
	/**
	 * The print node is capable of printing the actor and function it is being called from. To get the function the node is called from we
	 * recursively walk through the graph starting from the print node execution pin until we find the a node that doesn't have an execution pin.
	 * E.g. a BeginPlay node only has a then pin.
	 *
	 * Since we recursively walk up the chain of nodes there is a risk that in more complex graphs we may end up going too deep in the recursion
	 * calls. This value sets a limit for how many times we'll try to recursively find an end to our execution graph chain of nodes.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Global Settings", meta = (EditCondition="bGlobalDebug"))
	int32 MaxPrintNodeFunctionDiscoveryIterations = 1000;

	/**
	 * The default value of the string pin on the DFL Quick Print and DFL Print nodes.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Global Settings", meta = (EditCondition="bGlobalDebug"))
	FString PrintNodeDefaultString = "Hello";
#pragma endregion 
	
#pragma region Tick
	/**
	 * The duration float used on print nodes that are used on tick
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tick")
	float PrintDebugTickDuration = 0.05f;
#pragma endregion

#pragma region Print
	/**
	 * Should the quick print node print the class that called the node?
	 */
	UPROPERTY(Config, EditAnywhere, Category="Quick Print")
	bool bPrintClass = true;
	
	/**
	 * Should the quick print node call the function that called the node?
	 */
	UPROPERTY(Config, EditAnywhere, Category="Quick Print")
	bool bPrintFunction = true;
	
	/**
	 * Quick print node message colour.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Quick Print")
	FLinearColor PrintMessageColour = FColor::Cyan;
	
	/**
	 * Quick print node warning colour.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Quick Print")
	FLinearColor PrintWarningColour = FLinearColor::Yellow;
	
	/**
	 * Quick print node error colour.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Quick Print")
	FLinearColor PrintErrorColour = FLinearColor::Red;
	
	/**
	 * Quick print node default duration.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Quick Print")
	float PrintDuration = 2.0f;
#pragma endregion
};
