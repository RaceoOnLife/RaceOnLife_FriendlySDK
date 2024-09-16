


#include "Game/Components/VehicleRadioComponent.h"

// Sets default values for this component's properties
UVehicleRadioComponent::UVehicleRadioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentTrackID = 0;
	bIsRepeatEnabled = false;
	bIsTrackPaused = false;
	CurrentTimecode = 0.0f;
	Volume = 1.0f; // 100%
}

void UVehicleRadioComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UVehicleRadioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UVehicleRadioComponent::PlayTrack(int32 TrackID)
{
    if (DefaultMusicLibrary.IsValidIndex(TrackID))
    {
        CurrentTrackID = TrackID;
        FString TrackName = DefaultMusicLibrary[TrackID];
        bIsTrackPaused = false;
        CurrentTimecode = 0.0f;
        UE_LOG(LogTemp, Log, TEXT("Playing track: %s"), *TrackName);
    }
}

void UVehicleRadioComponent::ToggleRepeat()
{
    bIsRepeatEnabled = !bIsRepeatEnabled;
    UE_LOG(LogTemp, Log, TEXT("Repeat is %s"), bIsRepeatEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

void UVehicleRadioComponent::PlayNextTrack(bool IsUserChangedTrack)
{
    if (bIsRepeatEnabled && !IsUserChangedTrack)
    {
        PlayTrack(CurrentTrackID);
    }
    else
    {
        int32 NextTrackID = (CurrentTrackID + 1) % DefaultMusicLibrary.Num();
        PlayTrack(NextTrackID);
    }
}

void UVehicleRadioComponent::PlayPreviousTrack()
{
    if (CurrentTrackID == 0)
    {
        PlayTrack(DefaultMusicLibrary.Num() - 1);
    }
    else
    {
        PlayTrack(CurrentTrackID - 1);
    }
}

void UVehicleRadioComponent::PauseTrack()
{
    if (bIsTrackPaused)
    {
        // ѕродолжить воспроизведение с того же момента
        bIsTrackPaused = false;
        UE_LOG(LogTemp, Log, TEXT("Resuming track: %s"), *DefaultMusicLibrary[CurrentTrackID]);
    }
    else
    {
        bIsTrackPaused = true;
        CurrentTimecode = 120.0f;
        UE_LOG(LogTemp, Log, TEXT("Pausing track: %s"), *DefaultMusicLibrary[CurrentTrackID]);
    }
}

void UVehicleRadioComponent::AdjustVolume(bool IsVolumeUp)
{
    const float VolumeStep = 0.05f;
    if (IsVolumeUp)
    {
        Volume = FMath::Clamp(Volume + VolumeStep, 0.0f, 1.0f);
    }
    else
    {
        Volume = FMath::Clamp(Volume - VolumeStep, 0.0f, 1.0f);
    }
    UE_LOG(LogTemp, Log, TEXT("Current volume: %d%%"), static_cast<int32>(Volume * 100));
}

FString UVehicleRadioComponent::GetMusicName() const
{
    return DefaultMusicLibrary.IsValidIndex(CurrentTrackID) ? DefaultMusicLibrary[CurrentTrackID] : FString("No Track");
}

float UVehicleRadioComponent::GetMusicTimecode() const
{
    return CurrentTimecode;
}
