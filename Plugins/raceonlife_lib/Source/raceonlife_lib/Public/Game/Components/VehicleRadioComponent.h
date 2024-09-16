

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleRadioComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RACEONLIFE_LIB_API UVehicleRadioComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVehicleRadioComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
    TArray<FString> DefaultMusicLibrary;

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayTrack(int32 TrackID);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void ToggleRepeat();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayNextTrack(bool IsUserChangedTrack);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayPreviousTrack();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PauseTrack();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void AdjustVolume(bool IsVolumeUp);

    UFUNCTION(BlueprintCallable, Category = "Music")
    FString GetMusicName() const;

    UFUNCTION(BlueprintCallable, Category = "Music")
    float GetMusicTimecode() const;

private:
    int32 CurrentTrackID;
    bool bIsRepeatEnabled;
    bool bIsTrackPaused;
    float CurrentTimecode;
    float Volume;
};
