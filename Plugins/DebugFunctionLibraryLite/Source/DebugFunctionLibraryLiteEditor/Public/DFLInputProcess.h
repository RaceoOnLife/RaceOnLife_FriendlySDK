// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

class SDockTab;
class SGraphPanel;
class FBAGraphHandler;
class FBATabHandler;
class FUICommandList;

class DEBUGFUNCTIONLIBRARYLITEEDITOR_API FDFLInputProcessor : public TSharedFromThis<FDFLInputProcessor> , public IInputProcessor
{
public:
	FDFLInputProcessor();
	virtual ~FDFLInputProcessor() override;
	static void Init();
	static TSharedPtr<FDFLInputProcessor> Get();
	static void Cleanup();

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

	TSharedPtr<FUICommandList> Commands;
	
	TSharedPtr<SGraphPanel> GetGraph() const;
	TArray<UEdGraphNode*> GetSelectedNodes() const;
	UBlueprint* GetBlueprint() const;
	bool HasNodeSelected() const;
	TArray<UEdGraphNode*> GetSelectedPrintNodes() const;
	bool HasPrintNodeSelected() const;
	
	void UpdateNodePrintLevel() const;
	void UpdateNodePrintState() const;
	void UpdateNodeEnableState() const;
	void ToggleNodeDurationState() const;
	void ToggleNodeOverrideState() const;
	void ToggleNodeTickMethod() const;
	
};
