// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleLevelInstanceUtils.h"
#include "ToolMenus.h"
#include "ToolMenuDelegates.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/Engine.h"
#include "ScopedTransaction.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "CitySample/LevelInstance/CitySampleConvertedISMActor.h"

#define LOCTEXT_NAMESPACE "CitySampleLevelInstanceUtils"

void ConvertPackedToActor(APackedLevelActor* PackedLevelActor)
{
	const FScopedTransaction Transaction(LOCTEXT("ConvertPLIToActor", "Convert Packed Level Instance to ISM Actor"));
	UWorld* World = PackedLevelActor->GetWorld();
	ULevel* Level = PackedLevelActor->GetLevel();
	
	const FName OldActorName = PackedLevelActor->GetFName();
	const FName OldActorReplacedNamed = MakeUniqueObjectName(PackedLevelActor->GetOuter(), PackedLevelActor->GetClass(), *FString::Printf(TEXT("%s_CONVERTED"), *OldActorName.ToString()));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = OldActorName;
	SpawnParams.bCreateActorPackage = false;
	SpawnParams.OverridePackage = PackedLevelActor->GetExternalPackage();
	SpawnParams.OverrideActorGuid = PackedLevelActor->GetActorGuid();
	
	// Don't go through AActor::Rename here because we aren't changing outers (the actor's level) and we also don't want to reset loaders
	// if the actor is using an external package. We really just want to rename that actor out of the way so we can spawn the new one in
	// the exact same package, keeping the package name intact.
	PackedLevelActor->UObject::Rename(*OldActorReplacedNamed.ToString(), PackedLevelActor->GetOuter(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);

	ACitySampleConvertedISMActor* NewActor = World->SpawnActor<ACitySampleConvertedISMActor>(SpawnParams);
	
	// Attach conversion information
	NewActor->WorldAsset = PackedLevelActor->GetWorldAsset();
	
	PackedLevelActor->UnregisterAllComponents();
	NewActor->UnregisterAllComponents();

	// BP Generated Components do not get copied so we need to change them to Instanced components first
	if (PackedLevelActor->GetClass()->ClassGeneratedBy != nullptr)
	{
		NewActor->BlueprintAsset = Cast<UBlueprint>(PackedLevelActor->GetClass()->ClassGeneratedBy);
		PackedLevelActor->Modify();
		TArray<UActorComponent*> BlueprintComponents(PackedLevelActor->BlueprintCreatedComponents);
		for (UActorComponent* Component : BlueprintComponents)
		{
			Component->SetFlags(RF_Transactional);
			Component->Modify();
			PackedLevelActor->AddInstanceComponent(Component);
		}
		PackedLevelActor->BlueprintCreatedComponents.Empty();
	}

	UEngine::FCopyPropertiesForUnrelatedObjectsParams CopyParams;
	CopyParams.bDoDelta = false;
	CopyParams.bNotifyObjectReplacement = true;
	UEditorEngine::CopyPropertiesForUnrelatedObjects(PackedLevelActor, NewActor, CopyParams);

	NewActor->RegisterAllComponents();

	World->EditorDestroyActor(PackedLevelActor, false);
}

void FCitySampleLevelInstanceUtils::ExtendContextMenu()
{
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu"))
	{
		FToolMenuSection& Section = Menu->AddDynamicSection("CitySamplePackedLevelActor", 
			FNewToolMenuDelegate::CreateLambda([](UToolMenu* ToolMenu)
			{
				APackedLevelActor* SelectedActor = GEditor->GetSelectedActorCount() == 1 ? GEditor->GetSelectedActors()->GetTop<APackedLevelActor>() : nullptr;
				if (SelectedActor)
				{
					const FName SectionName = TEXT("CitySamplePackedLevelActor");
					FToolMenuInsert InsertPosition("ActorControl", EToolMenuInsertType::After);
					FToolMenuSection& Section = ToolMenu->AddSection(SectionName, LOCTEXT("CitySamplePackedLevelActor", "CitySample Packed Level Actor"), InsertPosition);
					
					FUIAction Action;
					Action.ExecuteAction = FExecuteAction::CreateStatic(&ConvertPackedToActor, SelectedActor);

					const FText Label = LOCTEXT("ConvertPLAToActorLabel", "Convert to ISM Actor");
					const FText ToolTip = LOCTEXT("ConvertPLAToActorToolTip", "Replaces Packed Level Actor with base Actor retaining packed components");

					Section.AddMenuEntry("ConvertPLAToActor", Label, ToolTip, FSlateIcon(), Action);
				}
			}), 
			FToolMenuInsert(NAME_None, EToolMenuInsertType::First));
	}
}

#undef LOCTEXT_NAMESPACE

