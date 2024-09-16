#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "EditorCore/Objects/BaseEditorActor.h"
#include "EditorHUDWidget.generated.h"

UENUM(BlueprintType)
enum class EEditorCategory : uint8
{
    Default UMETA(DisplayName = "Default"),
    Physics UMETA(DisplayName = "Physics"),
    Vehicle UMETA(DisplayName = "Vehicle"),
    Entity UMETA(DisplayName = "Entity"),
    AI UMETA(DisplayName = "AI")
};

USTRUCT(BlueprintType)
struct FEditorObjectInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    FString ObjectName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    UTexture2D* ObjectIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    TSubclassOf<class ABaseEditorActor> ActorReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    EEditorCategory Category;

    FEditorObjectInfo()
        : ObjectName(TEXT("Unnamed"))
        , ObjectIcon(nullptr)
        , ActorReference(nullptr)
        , Category(EEditorCategory::Default)
    {}
};

UCLASS()
class INGAMELEVELEDITOR_API UEditorHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    TArray<FEditorObjectInfo> EditorObjectsInfo;
};
