#include "Core/MinimapSystem/MinimapWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"

void UMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!MinimapTexture)
    {
        UE_LOG(LogTemp, Warning, TEXT("MinimapTexture is not set!"));
        return;
    }

    if (MinimapBackground)
    {
        MinimapBackground->SetBrushFromTexture(MinimapTexture);
    }

    InitializeMinimap();
}

void UMinimapWidget::InitializeMinimap()
{
    UWorld* World = GetWorld();
    if (!World || !MinimapCanvas)
        return;

    TArray<AActor*> Buildings;
    UGameplayStatics::GetAllActorsOfClass(World, BuildClass, Buildings);

    for (AActor* Building : Buildings)
    {
        FVector2D MinimapPosition = WorldToMinimapPosition(Building->GetActorLocation());

        UImage* BuildingIcon = NewObject<UImage>(this);
        BuildingIcon->SetBrushFromTexture(BuildIcon);

        UCanvasPanelSlot* CanvasSlot = MinimapCanvas->AddChildToCanvas(BuildingIcon);
        CanvasSlot->SetPosition(MinimapPosition);
        CanvasSlot->SetSize(FVector2D(16, 16));

        StaticObjectsIcons.Add(BuildingIcon);
    }

    TArray<AActor*> Roads;
    UGameplayStatics::GetAllActorsOfClass(World, RoadClass, Roads);

    for (AActor* Road : Roads)
    {
        USplineComponent* Spline = Road->FindComponentByClass<USplineComponent>();
        if (Spline)
        {
            int32 NumberOfPoints = Spline->GetNumberOfSplinePoints();

            for (int32 i = 0; i < NumberOfPoints - 1; ++i)
            {
                FVector StartPoint = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
                FVector EndPoint = Spline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

                FVector2D StartPos = WorldToMinimapPosition(StartPoint);
                FVector2D EndPos = WorldToMinimapPosition(EndPoint);
            }
        }
    }
}

void UMinimapWidget::AddDynamicObject(const FFPawnData& PawnData)
{
    if (!MinimapCanvas || !PawnData.Pawn || !PawnData.Icon)
        return;

    UImage* PawnIcon = NewObject<UImage>(this);
    PawnIcon->SetBrushFromTexture(PawnData.Icon);

    UCanvasPanelSlot* CanvasSlot = MinimapCanvas->AddChildToCanvas(PawnIcon);
    CanvasSlot->SetSize(FVector2D(16, 16));

    DynamicObjectsIcons.Add(PawnIcon);
    PawnIconMap.Add(PawnData.Pawn, PawnIcon);
}

void UMinimapWidget::UpdateDynamicObjects()
{
    for (auto& Pair : PawnIconMap)
    {
        APawn* Pawn = Pair.Key;
        UImage* Icon = Pair.Value;

        if (Pawn && Icon)
        {
            FVector2D MinimapPosition = WorldToMinimapPosition(Pawn->GetActorLocation());
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
            {
                CanvasSlot->SetPosition(MinimapPosition);
            }
        }
    }
}

FVector2D UMinimapWidget::WorldToMinimapPosition(const FVector& WorldLocation)
{
    FVector2D Offset = FVector2D(WorldLocation.X - MinimapCenter.X, WorldLocation.Y - MinimapCenter.Y);
    FVector2D MinimapPosition = Offset / MinimapScale;
    MinimapPosition += MinimapSize / 2.0f;
    return MinimapPosition;
}