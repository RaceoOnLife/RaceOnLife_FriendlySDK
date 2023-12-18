// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleUnrealEdEngine.h"

#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"

UCitySampleUnrealEdEngine::UCitySampleUnrealEdEngine(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCitySampleUnrealEdEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);

	// Update source control settings and register in case the provider changes
	UpdateSourceControlSettings();
	ISourceControlModule::Get().RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateLambda([this](ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider) {
		if (NewProvider.GetName() == FName("Perforce"))
		{
			this->UpdateSourceControlSettings();
		}
	}));
}

void UCitySampleUnrealEdEngine::UpdateSourceControlSettings()
{
	ISourceControlModule& SourceControlModule = ISourceControlModule::Get();

	if (!SourceControlModule.IsEnabled() || !SourceControlStatusBranches.Num())
	{
		UE_LOG(LogCitySampleEditor, Warning, TEXT("Unable to add content browser status branches, none specified"));
		return;
	}

	ISourceControlProvider& SourceControlProvider = SourceControlModule.GetProvider();

	if (SourceControlProvider.GetName() != FName("Perforce"))
	{
		UE_LOG(LogCitySampleEditor, Warning, TEXT("Unable to add content browser status branches, source control provider is not Perforce"));
		return;
	}

	UE_LOG(LogCitySampleEditor, Log, TEXT("Adding content browser status branches from ini"));

	for (const FString& Branch : SourceControlStatusBranches)
	{
		UE_LOG(LogCitySampleEditor, Log, TEXT("%s"), *Branch);
	}

	SourceControlProvider.RegisterStateBranches(SourceControlStatusBranches, TEXT("Collaboration/CitySample/CitySample"));
}
