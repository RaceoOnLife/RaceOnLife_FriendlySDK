#include "EditorCore/lib/EditorBPLibrary.h"
#include "EditorCore/Objects/BaseEditorActor.h"
#include "Engine/World.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UEditorBPLibrary::SaveLevel(UObject* WorldContextObject, const FString& FileName)
{
	UWorld* World = WorldContextObject->GetWorld();

	TArray<AActor*> _foundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ABaseEditorActor::StaticClass(), _foundActors);

	TArray<TSharedPtr<FJsonValue>> _jsonActorsArray;

	for (AActor* SpawnedActor : _foundActors)
	{
		FActorSaveData _actorData;
		_actorData.Location = SpawnedActor->GetActorLocation();
		_actorData.Rotation = SpawnedActor->GetActorRotation();
		_actorData.Scale = SpawnedActor->GetActorScale3D();
		_actorData.ActorClassName = SpawnedActor->GetClass()->GetName();

		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		FJsonObjectConverter::UStructToJsonObject(FActorSaveData::StaticStruct(), &_actorData, JsonObject.ToSharedRef(), 0, 0);

		_jsonActorsArray.Add(MakeShareable(new FJsonValueObject(JsonObject)));
	}

	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField(TEXT("Actors"), _jsonActorsArray);

	FString _jsonString;
	TSharedRef<TJsonWriter<>> _writer = TJsonWriterFactory<>::Create(&_jsonString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), _writer);

	FString FilePath = FPaths::ProjectSavedDir() + FileName;
	FFileHelper::SaveStringToFile(_jsonString, *FilePath);
}

FLoadLevelResult  UEditorBPLibrary::LoadLevel(UObject* WorldContextObject, const FString& FileName)
{
    FLoadLevelResult Result;

    UWorld* World = WorldContextObject->GetWorld();

    if (!World)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Invalid WorldContextObject.");
        return Result;
    }

    FString FilePath = FPaths::ProjectSavedDir() + FileName;
    FString JsonString;

    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Failed to load file: %s"), *FilePath);
        return Result;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Failed to parse JSON.");
        return Result;
    }

    const TArray<TSharedPtr<FJsonValue>>* JsonActorsArray;
    if (!RootObject->TryGetArrayField(TEXT("Actors"), JsonActorsArray))
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Failed to find 'Actors' field in JSON.");
        return Result;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ABaseEditorActor::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        Actor->Destroy();
    }

    for (const TSharedPtr<FJsonValue>& JsonActorValue : *JsonActorsArray)
    {
        TSharedPtr<FJsonObject> JsonActorObject = JsonActorValue->AsObject();
        FActorSaveData ActorData;
        FJsonObjectConverter::JsonObjectToUStruct(JsonActorObject.ToSharedRef(), FActorSaveData::StaticStruct(), &ActorData, 0, 0);

        UClass* ActorClass = FindObject<UClass>(ANY_PACKAGE, *ActorData.ActorClassName);

        if (ActorClass)
        {
            FActorSpawnParameters SpawnParams;
            AActor* SpawnedActor = World->SpawnActor<AActor>(ActorClass, ActorData.Location, ActorData.Rotation, SpawnParams);

            if (SpawnedActor)
            {
                SpawnedActor->SetActorScale3D(ActorData.Scale);
            }
        }
        else
        {
            Result.bSuccess = false;
            Result.ErrorMessage = FString::Printf(TEXT("Failed to find class: %s"), *ActorData.ActorClassName);
            return Result;
        }
    }

    Result.bSuccess = true;
    return Result;
}