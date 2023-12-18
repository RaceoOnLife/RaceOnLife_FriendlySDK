// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleGameInstanceBase.h"

#include "Kismet/GameplayStatics.h"
#include "NaniteVisualizationData.h"
#include "StreamingPauseRendering.h"

#include "CitySample.h"
#include "Game/CitySamplePlayerController.h"
#include "Game/CitySampleSaveGame.h"
#include "Util/CitySampleAssetManager.h"
#include "Util/CitySampleInputModifiers.h"


const FString UCitySampleGameInstanceBase::SaveFileName = TEXT("CitySampleSaveData");

UCitySampleGameInstanceBase::UCitySampleGameInstanceBase()
{
	DefaultNaniteVisualization = NAME_None;
	DefaultTemporalAASamples = false;
	CurrentNaniteVisualization = NAME_None;
}

void UCitySampleGameInstanceBase::OnSaveGameComplete(const FString& SaveFile, const int32 UserIndex, bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogCitySample, Warning, TEXT("Game was not successfully saved!"), *GetName());
	}
}

void UCitySampleGameInstanceBase::Init()
{
	Super::Init();

	// Loads a save game from disk, if it exists, or creates a new save game.
	LoadCitySampleSaveGame(&bSaveGameLoaded);

	UCitySampleAssetManager* CitySampleAssetManager = CastChecked<UCitySampleAssetManager>(&UAssetManager::Get());
	CitySampleAssetManager->PreloadItemDefinitions();
}

void UCitySampleGameInstanceBase::Shutdown()
{
	Super::Shutdown();

	// Reset nanite visualization cvars and unregister cvar sink callbacks
	ResetNaniteVisualization();
	IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(NaniteVisualizationSinkHandle);
	IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(TemporalAASamplesSinkHandle);
}

void UCitySampleGameInstanceBase::OnStart()
{
	Super::OnStart();

	int32 SessionHint = INDEX_NONE;

	// Set nanite visualization cvars and register cvar callbacks
	UpdateDefaultNaniteVisualization();
	UpdateDefaultTemporalAASamples();
	CurrentNaniteVisualization = DefaultNaniteVisualization;

	NaniteVisualizationSinkHandle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(FConsoleCommandDelegate::CreateUObject(this, &UCitySampleGameInstanceBase::UpdateDefaultNaniteVisualization));
	TemporalAASamplesSinkHandle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(FConsoleCommandDelegate::CreateUObject(this, &UCitySampleGameInstanceBase::UpdateDefaultTemporalAASamples));
}

UCitySampleSaveGame* UCitySampleGameInstanceBase::LoadCitySampleSaveGame(bool* const bOutSaveGameFound/*=false*/)
{
	SaveGame = Cast<UCitySampleSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveFileName, 0));

	if (SaveGame)
	{
		if (bOutSaveGameFound != nullptr)
		{
			*bOutSaveGameFound = true;
		}
		
		bool bX, bY, bZ;
		GetInvertedAxes(bX, bY, bZ);
		SetInvertedAxes(bX, SaveGame->bVerticalAxisInverted, bZ);

		SetLookSensitivity(SaveGame->LookSensivity);
		SetRumbleEnabled(SaveGame->bForceFeedbackEnabled);

		// Broadcast the loaded data so others can do the same
		OnSaveGameLoaded.Broadcast(SaveGame);
	}
	else
	{
		if (bOutSaveGameFound != nullptr)
		{
			*bOutSaveGameFound = false;
		}

		UE_LOG(LogCitySample, Display, TEXT("%s: failed to load save data. Most likely no save data exists yet."), ANSI_TO_TCHAR(__func__));
		SaveGame = Cast<UCitySampleSaveGame>(UGameplayStatics::CreateSaveGameObject(UCitySampleSaveGame::StaticClass()));

		if (SaveGame == nullptr)
		{
			UE_LOG(LogCitySample, Warning, TEXT("%s: failed to create a new save game."), ANSI_TO_TCHAR(__func__));
		}
	}

	return SaveGame;
}

void UCitySampleGameInstanceBase::SaveCitySampleGameData(bool bAsync/*=true*/)
{
	if (SaveGame) 
	{
		bool bX, bZ;
		GetInvertedAxes(bX, SaveGame->bVerticalAxisInverted, bZ);
		SaveGame->LookSensivity = GetLookSensitivity();

		// PlayerController may be null, so we can't always update rumble
		if (const APlayerController* const PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			SaveGame->bForceFeedbackEnabled = PC->bForceFeedbackEnabled;
		}

		if (bAsync)
		{ 
			// Start saving game data asynchronously...
			FAsyncSaveGameToSlotDelegate AsyncSaveGameDelegate;
			AsyncSaveGameDelegate.BindUObject(this, &UCitySampleGameInstanceBase::OnSaveGameComplete);
			UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SaveFileName, 0, AsyncSaveGameDelegate);
		}
		else
		{
			// Immediately save game data
			OnSaveGameComplete(SaveFileName, 0, UGameplayStatics::SaveGameToSlot(SaveGame, SaveFileName, 0));
		}
	}
	else
	{
		UE_LOG(LogCitySample, Warning, TEXT("No save data exists. Save failed..."));
		OnSaveGameComplete(SaveFileName, 0, false);
	}
}

void UCitySampleGameInstanceBase::ClearCitySampleSaveData()
{
	// If a save game currently exists
	if (UGameplayStatics::DoesSaveGameExist(SaveFileName, 0))
	{
		// Delete the save data from disk
		UGameplayStatics::DeleteGameInSlot(SaveFileName, 0);
		// Clear the current save game so it is not just rewritten on shutdown
		SaveGame = nullptr;
		UE_LOG(LogCitySample, Display, TEXT("CitySample Save Data has been cleared upon request."));
	}
	else
	{
		UE_LOG(LogCitySample, Display, TEXT("Failed to clear save data. Most likely no save data exists yet."));
	}
}

void UCitySampleGameInstanceBase::GetInvertedAxes(bool& bX, bool& bY, bool& bZ) const
{
	bX = UCitySampleInputModifierInvertAxis::bX;
	bY = UCitySampleInputModifierInvertAxis::bY;
	bZ = UCitySampleInputModifierInvertAxis::bZ;
}

void UCitySampleGameInstanceBase::SetInvertedAxes(const bool bX, const bool bY, const bool bZ)
{
	UCitySampleInputModifierInvertAxis::bX = bX;
	UCitySampleInputModifierInvertAxis::bY = bY;
	UCitySampleInputModifierInvertAxis::bZ = bZ;

	if (SaveGame)
	{
		SaveGame->bVerticalAxisInverted = bY;
	}
}

FVector UCitySampleGameInstanceBase::GetLookSensitivity() const
{
	return UCitySampleInputModifierLookSensitivity::Scalar;
}

void UCitySampleGameInstanceBase::SetLookSensitivity(const FVector Scalar)
{
	UCitySampleInputModifierLookSensitivity::Scalar = Scalar;

	if (SaveGame)
	{
		SaveGame->LookSensivity = Scalar;
	}
}

bool UCitySampleGameInstanceBase::GetRumbleEnabled() const
{
	if (const APlayerController* const PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		return PC->bForceFeedbackEnabled;
	}

	return false;
}

void UCitySampleGameInstanceBase::SetRumbleEnabled(const bool bEnabled)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->bForceFeedbackEnabled = bEnabled;

		if (SaveGame)
		{
			SaveGame->bForceFeedbackEnabled = bEnabled;
		}
	}
}

void UCitySampleGameInstanceBase::SetNaniteVisualization(const FString& Visualization/*=TEXT("none")*/)
{
	const FNaniteVisualizationData& VisualizationData = GetNaniteVisualizationData();
	if (VisualizationData.GetModeID(CurrentNaniteVisualization) != VisualizationData.GetModeID(FName(*Visualization)))
	{
		if (IConsoleVariable* const ICVarVisualize = IConsoleManager::Get().FindConsoleVariable(VisualizationData.GetVisualizeConsoleCommandName()))
		{
			// Set the visualization mode and track the current visualization
			ICVarVisualize->Set(*Visualization);
			CurrentNaniteVisualization = FName(*Visualization);
		}

		if (IConsoleVariable* const TemporalAASamples = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAASamples")))
		{
			// If a visualization is active
			if (VisualizationData.GetModeID(CurrentNaniteVisualization) != INDEX_NONE)
			{
				// Disable temporal AA samples to remove jitter
				TemporalAASamples->Set(0);
			}
			else
			{
				// Otherwise reset to the last value set that was not set by this function
				TemporalAASamples->Set(DefaultTemporalAASamples);
			}
		}
	}
}

void UCitySampleGameInstanceBase::ResetNaniteVisualization()
{
	if (IConsoleVariable* const ICVarVisualize = IConsoleManager::Get().FindConsoleVariable(GetNaniteVisualizationData().GetVisualizeConsoleCommandName()))
	{
		// Reset to the last value set that was not set with UCitySampleGameInstanceBase::SetNaniteVisualization
		ICVarVisualize->Set(*DefaultNaniteVisualization.ToString());
		CurrentNaniteVisualization = DefaultNaniteVisualization;
	}

	if (IConsoleVariable* const TemporalAASamples = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAASamples")))
	{
		// Reset to the last value set that was not set with UCitySampleGameInstanceBase::SetNaniteVisualization
		TemporalAASamples->Set(DefaultTemporalAASamples);
	}
}

void UCitySampleGameInstanceBase::UpdateDefaultNaniteVisualization()
{
	if (IConsoleVariable* const ICVarVisualize = IConsoleManager::Get().FindConsoleVariable(GetNaniteVisualizationData().GetVisualizeConsoleCommandName()))
	{
		FName Visualization = FName(*ICVarVisualize->GetString());

		// If the CVar was overridden by something (does not allow overrides of the same visualization)
		if (CurrentNaniteVisualization != Visualization)
		{
			// Set as the current default and set the current visualization as the new value
			DefaultNaniteVisualization = MoveTemp(Visualization);
			CurrentNaniteVisualization = DefaultNaniteVisualization;

			// Reset the temporal AA samples CVar
			if (IConsoleVariable* const TemporalAASamples = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAASamples")))
			{
				TemporalAASamples->Set(DefaultTemporalAASamples);
			}
		}
	}
}

void UCitySampleGameInstanceBase::UpdateDefaultTemporalAASamples()
{
	if (const IConsoleVariable* const CVarTemporalAASamples = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAASamples")))
	{
		const int32 TemporalAASamples = CVarTemporalAASamples->GetInt();

		// If the current visualization is active and temporal AA samples is overridden
		if (GetNaniteVisualizationData().GetModeID(CurrentNaniteVisualization) != INDEX_NONE && TemporalAASamples != 0)
		{
			// Set the new value as the current default value
			DefaultTemporalAASamples = TemporalAASamples;
		}
	}
}

