#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EditorBPLibrary.generated.h"

USTRUCT(BlueprintType)
struct FLoadLevelResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;

    FLoadLevelResult()
        : bSuccess(false), ErrorMessage(TEXT(""))
    {
    }
};

USTRUCT(BlueprintType)
struct FActorSaveData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FVector Location;

    UPROPERTY(BlueprintReadWrite)
    FRotator Rotation;

    UPROPERTY(BlueprintReadWrite)
    FVector Scale;

    UPROPERTY(BlueprintReadWrite)
    FString ActorClassName;

    FActorSaveData()
        : Location(FVector::ZeroVector)
        , Rotation(FRotator::ZeroRotator)
        , Scale(FVector(1.0f, 1.0f, 1.0f))
        , ActorClassName(TEXT(""))
    {
    }
};

UCLASS()
class INGAMELEVELEDITOR_API UEditorBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "LevelSave")
    static void SaveLevel(UObject* WorldContextObject, const FString& FileName);

    UFUNCTION(BlueprintCallable, Category = "LevelLoad")
    FLoadLevelResult LoadLevel(UObject* WorldContextObject, const FString& FileName);
};
