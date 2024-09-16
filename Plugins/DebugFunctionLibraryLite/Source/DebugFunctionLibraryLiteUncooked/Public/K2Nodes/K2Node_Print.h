// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_Print.generated.h"

UENUM(BlueprintType)
enum class EK2NodePrintState : uint8
{
	PrintLog,
	Print,
	Log
};

UENUM(BlueprintType)
enum class EK2NodePrintLevel : uint8
{
	Message,
	Warning,
	Error
};

UENUM(BlueprintType)
enum class EK2NodePrintOverrideMethod : uint8
{
	NoOverriden,
	OverrideDuration,
};

UENUM(BlueprintType)
enum class EK2NodePrintTickMethod : uint8
{
	NoTick,
	Tick
};

UCLASS(BlueprintType)
class DEBUGFUNCTIONLIBRARYLITEUNCOOKED_API UK2Node_Print : public UK2Node, public IK2Node_AddPinInterface
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FName GetCornerIcon() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	//~ End UEdGraphNode Interface.
	
	//~ Begin UK2Node Interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual FText GetMenuCategory() const override;
	virtual void PostPlacedNewNode() override;
	virtual bool ShouldShowNodeProperties() const override { return true; };
	virtual void PostReconstructNode() override;
	//~ End UK2Node Interface

	// IK2Node_AddPinInterface interface
	virtual void AddInputPin() override;
	// End of IK2Node_AddPinInterface interface
	
	void CycleNodePrintLevel();
	void CycleNodePrintState();
	void SetOverrideMethod(EK2NodePrintOverrideMethod InOverrideMethod);
	void UpdateTickMethod(EK2NodePrintTickMethod NewTickMethod);

public:
	UPROPERTY(EditAnywhere, Category = "Node State")
	EK2NodePrintState NodePrintState;
	
	UPROPERTY(EditAnywhere, Category = "Node State")
	EK2NodePrintLevel NodePrintLevel;

	UPROPERTY(EditAnywhere, Category = "Node State")
	EK2NodePrintOverrideMethod OverridenMethod;

	UPROPERTY(EditAnywhere, Category = "Node State")
	EK2NodePrintTickMethod TickMethod;

protected:
	UPROPERTY()
	FName TargetFunctionName = "DFLQuickPrintLogMessage";
	
	virtual void UpdateFunctionName();
	
private:
	UPROPERTY()
	TArray<FName> PinNames;

	UPROPERTY()
	int32 MaxGetHeadNodeRecursiveIterations;
	
private:
	UFunction* GetTargetFunction() const;
	UEdGraphNode* GetHeadNode(UEdGraphPin* Pin);
	UEdGraphPin* GetExecPinFromThenPin(const UEdGraphPin* ThenPin);
	FString GetNodeTitle(UEdGraphNode* Node);
	bool IsPinCircularDependent(UEdGraphPin* Pin);

#pragma region Formatting
	FName GetUniquePinName() const;
	void SynchronizeArgumentPinType(UEdGraphPin* Pin) const;
	UEdGraphPin* FindArgumentPin(const FName InPinName) const;
	bool CanEditArguments() const { return GetStringPin()->LinkedTo.Num() > 0; }
	void SetArgumentName(int32 InIndex, FName InName);
	void RemoveArgument(int32 InIndex);
	FText GetArgumentName(int32 InIndex) const;
	int32 GetArgumentCount() const { return PinNames.Num(); }
	TArray<FName> GetArgumentNames();
	void AddArgumentPin();
	int32 GetArgumentIndex(const FName PinName);
#pragma endregion

#pragma region PinGetters
	UEdGraphPin* GetThenPin() const;
	UEdGraphPin* GetStringPin() const;
	UEdGraphPin* GetDurationPin() const;
#pragma endregion

#pragma region UpdateFunctions
	void UpdateNodePrintLevel(EK2NodePrintLevel NewNodePrintLevel);
	void UpdateNodePrintState(EK2NodePrintState NewNodePrintState);
	void RefreshNode();
#pragma endregion

#pragma region StateLevelFunction
	int32 GetNodePrintStateEnumSize() const;
	int32 GetNodePrintLevelEnumSize() const;
	UEnum* GetEnumFromName(FString EnumName) const;
#pragma endregion

#pragma region ContextMenuFunction
	void CreateNodePrintMessageLevel(FToolMenuSection& Section);
	void CreateNodePrintWarningLevel(FToolMenuSection& Section);
	void CreateNodePrintErrorLevel(FToolMenuSection& Section);

	void CreateNodePrintLogState(FToolMenuSection& Section);
	void CreateNodePrintState(FToolMenuSection& Section);
	void CreateNodeLogState(FToolMenuSection& Section);

	void CreateNodeEnableState(FToolMenuSection& Section);
	void CreateNodeDisableState(FToolMenuSection& Section);

	void CreateNodeEnableDuration(FToolMenuSection& Section);
	void CreateNodeDisableDuration(FToolMenuSection& Section);

	void CreateNodeTickEnable(FToolMenuSection& Section);
	void CreateNodeNoTickEnable(FToolMenuSection& Section);
#pragma endregion
};


