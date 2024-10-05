#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/lib_types.h"
#include "RuntimeAudioImporterLibrary.h"
#include "VehicleRadioComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RACEONLIFE_LIB_API UVehicleRadioComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVehicleRadioComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
    TArray<FMusicStruct> DefaultMusicLibrary;

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayTrack(FMusicStruct MusicData, USoundAttenuation* AttenuationSettings);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void ToggleRepeat();

    UFUNCTION(BlueprintCallable, Category = "Music")
    FMusicStruct PlayNextTrack(TArray<FMusicStruct> MusicLibrary, bool IsUserChangedTrack);

    UFUNCTION(BlueprintCallable, Category = "Music")
    FMusicStruct PlayPreviousTrack(TArray<FMusicStruct> MusicLibrary);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PauseTrack(FMusicStruct MusicData);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void AdjustVolume(bool IsVolumeUp);

    UFUNCTION(BlueprintCallable, Category = "Music")
    FString GetMusicName() const;

    UFUNCTION(BlueprintCallable, Category = "Music")
    float GetMusicTimecode() const;

    UFUNCTION(BlueprintCallable, Category = "Music")
    void ImportMusicFromDisk(FString Path);

    UFUNCTION(BlueprintCallable, Category = "Music")
    UTexture2D* ImportImageAsTexture2D(FString ImagePath);

    UPROPERTY(BlueprintReadWrite, Category = "Music")
    FMusicStruct CurrentPlayingMusic;

    UPROPERTY(BlueprintReadWrite, Category = "Music")
    TArray<FMusicStruct> MusicPlaylist;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Music")
    bool GetIsRepeatEnabled() { return bIsRepeatEnabled; }

    UFUNCTION(BlueprintCallable, Category = "Music")
    TArray<FMusicStruct> GetImportedMusic() { return ImportedMusic; }

    UFUNCTION(BlueprintCallable, Category = "Music")
    float GetCurrentMusicVolume() { return MusicVolume; }

private:
    UFUNCTION()
    void OnAudioImportCompleted(URuntimeAudioImporterLibrary* Importer, UImportedSoundWave* ImportedSoundWave, ERuntimeImportStatus Status);

    bool bIsRepeatEnabled;
    float MusicVolume;

    FString FilePath;

    TArray<FMusicStruct> ImportedMusic;
};
