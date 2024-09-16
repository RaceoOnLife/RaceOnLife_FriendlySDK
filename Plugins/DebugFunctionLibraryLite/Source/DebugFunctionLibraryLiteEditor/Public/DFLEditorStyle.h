// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "Styling/SlateStyle.h"

class FDFLEditorStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static void ReloadTextures();
	static const ISlateStyle& Get();
	static FName GetStyleSetName();

private:
	static TSharedRef<class FSlateStyleSet> Create();
	static TSharedPtr<class FSlateStyleSet> StyleInstance;
};

