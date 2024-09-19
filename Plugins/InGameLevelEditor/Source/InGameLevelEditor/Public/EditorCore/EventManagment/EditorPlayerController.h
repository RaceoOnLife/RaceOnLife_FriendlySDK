#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EditorCore/UI/EditorHUDWidget.h"
#include "EditorPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class INGAMELEVELEDITOR_API AEditorPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UEditorHUDWidget> EditorHUDWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "UserInput")
AActor* PerformLineTraceToCursor(FString& ReturnValue2);
};
