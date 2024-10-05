#include "Game/Components/VehicleRadioComponent.h"
#include "Misc/FileHelper.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Sound/AudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "RuntimeAudioImporterLibrary.h"
#include "UObject/ConstructorHelpers.h"

UVehicleRadioComponent::UVehicleRadioComponent()
{
    // maybe there will be something here, but it's unlikely. kek
}

void UVehicleRadioComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UVehicleRadioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVehicleRadioComponent::PlayTrack(FMusicStruct MusicData, USoundAttenuation* AttenuationSettings)
{
    if (!MusicData.MusicReference)
    {
        UE_LOG(LogTemp, Warning, TEXT("No music reference provided!"));
        return;
    }

    APlayerController* Owner = Cast<APlayerController>(GetOwner());
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("Owner not found!"));
        return;
    }

    APawn* ControlledPawn = Cast<APawn>(Owner->GetPawn());
    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Owner is not a controlled pawn!"));
        return;
    }

    UMeshComponent* Mesh = ControlledPawn->FindComponentByClass<UMeshComponent>();
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("No mesh component found!"));
        return;
    }

    UAudioComponent* AudioComponent = ControlledPawn->FindComponentByClass<UAudioComponent>();

    if (!AudioComponent)
    {
        AudioComponent = NewObject<UAudioComponent>(ControlledPawn);
        if (AudioComponent)
        {
            AudioComponent->bAutoDestroy = false;
            AudioComponent->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform);
            AudioComponent->RegisterComponent();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create audio component!"));
            return;
        }
    }

    if (AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }

    AudioComponent->SetSound(MusicData.MusicReference);
    if (AttenuationSettings)
    {
        AudioComponent->AttenuationSettings = AttenuationSettings;
    }

    AudioComponent->Play();
    CurrentPlayingMusic = MusicData;
}

void UVehicleRadioComponent::ToggleRepeat()
{
    bIsRepeatEnabled = !bIsRepeatEnabled;
}

FMusicStruct UVehicleRadioComponent::PlayNextTrack(TArray<FMusicStruct> MusicLibrary, bool IsUserChangedTrack)
{
    int32 CurrentIndex = MusicLibrary.Find(CurrentPlayingMusic);

    if (CurrentIndex == -1)
    {
        return MusicLibrary[0];
    }

    int32 NextIndex = (CurrentIndex + 1) % MusicLibrary.Num();

    if (CurrentIndex == MusicLibrary.Num() - 1)
    {
        NextIndex = 0;
    }

    CurrentPlayingMusic = MusicLibrary[NextIndex];

    return MusicLibrary[NextIndex];
}

FMusicStruct UVehicleRadioComponent::PlayPreviousTrack(TArray<FMusicStruct> MusicLibrary)
{
    int32 CurrentIndex = MusicLibrary.Find(CurrentPlayingMusic);

    if (CurrentIndex == -1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Current track is not found in the music library!"));
        return FMusicStruct();
    }

    int32 PreviousIndex = (CurrentIndex - 1 + MusicLibrary.Num()) % MusicLibrary.Num();

    if (CurrentIndex == 0)
    {
        PreviousIndex = MusicLibrary.Num() - 1;
    }

    CurrentPlayingMusic = MusicLibrary[PreviousIndex];

    return MusicLibrary[PreviousIndex];
}


void UVehicleRadioComponent::PauseTrack(FMusicStruct MusicData)
{
    return;
}

void UVehicleRadioComponent::AdjustVolume(bool IsVolumeUp)
{
    const float VolumeStep = 0.05f;
    if (IsVolumeUp)
    {
        MusicVolume = FMath::Clamp(MusicVolume + VolumeStep, 0.0f, 1.0f);
    }
    else
    {
        MusicVolume = FMath::Clamp(MusicVolume - VolumeStep, 0.0f, 1.0f);
    }
    UE_LOG(LogTemp, Log, TEXT("Current volume: %d%%"), static_cast<int32>(MusicVolume * 100));
}

FString UVehicleRadioComponent::GetMusicName() const
{
    return CurrentPlayingMusic.MusicName.IsEmpty() ? CurrentPlayingMusic.MusicName : FString("No Track");
}

float UVehicleRadioComponent::GetMusicTimecode() const
{
    APlayerController* Owner = Cast<APlayerController>(GetOwner());
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("Owner not found!"));
        return 0.f;
    }

    APawn* ControlledPawn = Cast<APawn>(Owner->GetPawn());
    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Owner is not a controlled pawn!"));
        return 0.f;
    }

    UAudioComponent* AudioComponent = ControlledPawn->FindComponentByClass<UAudioComponent>();
    if (AudioComponent && AudioComponent->IsPlaying())
    {
        return 0.f;
    }
    return 0.f;
}

void UVehicleRadioComponent::ImportMusicFromDisk(FString Path)
{
    TArray<FString> MusicFiles;
    IFileManager& FileManager = IFileManager::Get();
    FileManager.FindFilesRecursive(MusicFiles, *Path, TEXT("*.mp3"), true, false, false);

    for (const FString& File : MusicFiles)
    {
        URuntimeAudioImporterLibrary* AudioImporter = URuntimeAudioImporterLibrary::CreateRuntimeAudioImporter();

        FilePath = File;

        AudioImporter->OnResult.AddDynamic(this, &UVehicleRadioComponent::OnAudioImportCompleted);

        AudioImporter->ImportAudioFromFile(File, ERuntimeAudioFormat::Mp3);
    }
}

void UVehicleRadioComponent::OnAudioImportCompleted(URuntimeAudioImporterLibrary* Importer, UImportedSoundWave* ImportedSoundWave, ERuntimeImportStatus Status)
{
    if (Status == ERuntimeImportStatus::SuccessfulImport)
    {
        FMusicStruct NewMusic;
        NewMusic.MusicReference = ImportedSoundWave;

        NewMusic.MusicName = ImportedSoundWave->GetPathName();
        NewMusic.MusicLength = ImportedSoundWave->Duration;

        ImportedMusic.Add(NewMusic);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to import audio."));
    }
}

UTexture2D* UVehicleRadioComponent::ImportImageAsTexture2D(FString ImagePath)
{
    return nullptr;
}
