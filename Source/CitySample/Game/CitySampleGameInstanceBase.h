// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "CitySampleGameInstanceBase.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCitySampleOnSaveGameLoaded, const UCitySampleSaveGame* const, LoadedSaveGame);

UCLASS(Blueprintable)
class CITYSAMPLE_API UCitySampleGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()

public:

	UCitySampleGameInstanceBase();

	//~ Begin UGameInstance Interface
	virtual void Init() override;
	virtual void Shutdown() override;
protected:
	virtual void OnStart() override;
	//~ End UGameInstance Interface

public:

	/** Returns the save game object, which may have been loaded from disk or freshly created. */
	UFUNCTION(BlueprintPure, Category="SaveGame")
	UCitySampleSaveGame* GetSaveGame() 
	{
		return SaveGame;
	}

	/** Initializes the save game by either loading from disk or creating a new one. Optionally returns whether a save game was found. */
	UFUNCTION(BlueprintPure, Category = "SaveGame", meta=(DisplayName="LoadCitySampleSaveGame", AutoCreateRefTerm = "bOutSaveGameFound"))
	UCitySampleSaveGame* K2Node_LoadCitySampleSaveGame(bool& bOutSaveGameFound)
	{
		return LoadCitySampleSaveGame(&bOutSaveGameFound);
	}

	UCitySampleSaveGame* LoadCitySampleSaveGame(bool* const bOutSaveGameFound=nullptr);

	UPROPERTY(BlueprintAssignable)
	FCitySampleOnSaveGameLoaded OnSaveGameLoaded;

	/** Saves the save game to disk. */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveCitySampleGameData(bool bAsync=true);

	/** Whether saved data was loaded from disk. This returns false when the save game was newly created. */
	UFUNCTION(BlueprintPure, Category="SaveGame")
	bool IsSaveGameLoaded() const
	{
		return bSaveGameLoaded;
	}

	/** Deletes current save game and reset save data to defaults. */
	UFUNCTION(BlueprintCallable, Category="SaveGame")
	void ClearCitySampleSaveData();

	//~ Begin CitySample Input Modifiers
	UFUNCTION(BlueprintPure, Category="Input Modifiers")
	void GetInvertedAxes(bool& bX, bool& bY, bool& bZ) const;
	
	UFUNCTION(BlueprintCallable, Category="Input Modifiers")
	void SetInvertedAxes(const bool bX, const bool bY, const bool bZ);

	UFUNCTION(BlueprintPure, Category = "Input Modifiers")
	FVector GetLookSensitivity() const;

	UFUNCTION(BlueprintCallable, Category="Input Modifiers", meta=(AutoCreateRefTerm="Scalar"))
	void SetLookSensitivity(const FVector Scalar);

	UFUNCTION(BlueprintPure, Category = "Input Modifiers")
	bool GetRumbleEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Input Modifiers")
	void SetRumbleEnabled(const bool bEnabled);
	//~ End CitySample Input Modifiers

	UFUNCTION(BlueprintPure, Category = "Nanite")
	const FName& GetCurrentNaniteVisualization()
	{
		return CurrentNaniteVisualization;
	}

	/** Attempts to set current nanite visualization type to the string parameter that was passed in */
	UFUNCTION(BlueprintCallable, Category = "Nanite", meta=(AutoCreateRefTerm="Visualization"))
	void SetNaniteVisualization(const FString& Visualization=TEXT("none"));

	/** Resets both Nanite Visualization and Temporal AA sample count to the currently set defaults */
	UFUNCTION(BlueprintCallable, Category = "Nanite")
	void ResetNaniteVisualization();

	UTexture* GetBaseGroomTexture() const
	{
		return BaseGroomTexture.LoadSynchronous();
	}

	UTexture* GetBaseFollicleMaskTexture()
	{
		return BaseFollicleMaskTexture.LoadSynchronous();
	}

private:

	/** Delegate providing default functionality for beginning streaming pause. */
	FBeginStreamingPauseDelegate BeginStreamingPauseDelegate;

	/** Delegate providing default functionality for ending streaming pause. */
	FEndStreamingPauseDelegate EndStreamingPauseDelegate;

	UPROPERTY()
	UCitySampleSaveGame* SaveGame = nullptr;

	// Callback for logging whether the game was successfully saved or not
	void OnSaveGameComplete(const FString& SaveFile, const int32 UserIndex, bool bSuccess);

	/** Name of save file we load and save to */
	const static FString SaveFileName;

	/** Whether saved data has been loaded. */
	bool bSaveGameLoaded = false;

	// CVar sink handles and handlers to enable the default CVar values to be reset when set externally
	FConsoleVariableSinkHandle NaniteVisualizationSinkHandle;
	FConsoleVariableSinkHandle TemporalAASamplesSinkHandle;
	void UpdateDefaultNaniteVisualization();
	void UpdateDefaultTemporalAASamples();

	UPROPERTY(Transient, VisibleAnywhere, Category = "Nanite")
	FName DefaultNaniteVisualization;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Nanite")
	int32 DefaultTemporalAASamples;

	UPROPERTY(VisibleAnywhere, Category = "Nanite")
	FName CurrentNaniteVisualization;

	UPROPERTY(EditDefaultsOnly, Config)
	TSoftObjectPtr<class UTexture> BaseGroomTexture;

	UPROPERTY(EditDefaultsOnly, Config)
	TSoftObjectPtr<class UTexture> BaseFollicleMaskTexture;
};

