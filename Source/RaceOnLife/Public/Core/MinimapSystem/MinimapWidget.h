#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

USTRUCT(BlueprintType)
struct FFPawnData
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    APawn* Pawn;

    UPROPERTY(BlueprintReadWrite)
    UTexture2D* Icon;

    FFPawnData()
        : Pawn(nullptr), Icon(nullptr)
    {
    }

    FFPawnData(APawn* InPawn, UTexture2D* InIcon)
        : Pawn(InPawn), Icon(InIcon)
    {
    }
};

UCLASS()
class RACEONLIFE_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Minimap")
    void InitializeMinimap();

    UFUNCTION(BlueprintCallable, Category = "Minimap")
    void UpdateDynamicObjects();

    UFUNCTION(BlueprintCallable, Category = "Minimap")
    void AddDynamicObject(const FFPawnData& PawnData);

protected:
    UPROPERTY(meta = (BindWidget))
    class UImage* MinimapBackground;

    UPROPERTY(meta = (BindWidget))
    class UCanvasPanel* MinimapCanvas;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* MinimapTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float MinimapScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    FVector2D MinimapCenter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    FVector2D MinimapSize;

    UPROPERTY()
    TArray<UImage*> StaticObjectsIcons;

    UPROPERTY()
    TArray<UImage*> DynamicObjectsIcons;

    UPROPERTY()
    TMap<APawn*, UImage*> PawnIconMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* BuildIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    TSubclassOf<class AActor> RoadClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    TSubclassOf<class AActor> BuildClass;

    virtual void NativeConstruct() override;

    FVector2D WorldToMinimapPosition(const FVector& WorldLocation);
};
