// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleEditor.h"
#include "UnrealEd.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"
#include "Validation/CitySampleEditorValidator.h"
#include "LevelInstance/CitySampleLevelInstanceUtils.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "CitySampleEditor"

IMPLEMENT_GAME_MODULE(FCitySample_EditorModule, CitySampleEditor);

DEFINE_LOG_CATEGORY(LogCitySampleEditor);



static bool HasValidPlayWorld()
{
	return GEditor->PlayWorld != nullptr;
}

static bool HasNoPlayWorld()
{
	return !HasValidPlayWorld();
}

static void CitySampleCheckContent_Clicked()
{
	UCitySampleEditorValidator::ValidateCheckedOutContent(/*bInteractive=*/true);
}

static void CitySamplePlayToolbarCreation()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.SettingsToolBar");
	FToolMenuSection& Section = Menu->FindOrAddSection("Game");

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"CheckContent",
		FUIAction(
			FExecuteAction::CreateStatic(&CitySampleCheckContent_Clicked),
			FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)),
		LOCTEXT("CheckContentButton", "Check Content"),
		LOCTEXT("CheckContentDescription", "Runs the Content Validation job on all checked out assets to look for warnings and errors"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.MapCheck")
	));
}


void FCitySample_EditorModule::StartupModule()
{
	if (!IsRunningGame())
	{
		if (FSlateApplication::IsInitialized())
		{
			UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&CitySamplePlayToolbarCreation));

			FCitySampleLevelInstanceUtils::ExtendContextMenu();
		}
	}
}

#undef LOCTEXT_NAMESPACE
