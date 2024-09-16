#include "EditorCore/EventManagment/EditorPlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"

void AEditorPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (EditorHUDWidgetClass)
    {
        UEditorHUDWidget* HUDWidget = CreateWidget<UEditorHUDWidget>(this, EditorHUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
            SetInputMode(FInputModeGameAndUI());
        }
    }

    bShowMouseCursor = true;
}

AActor* AEditorPlayerController::PerformLineTraceToCursor()
{
    if (!GetPawn())
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

    if (!World || !PlayerController)
    {
        return nullptr;
    }

    float MouseX, MouseY;
    if (!PlayerController->GetMousePosition(MouseX, MouseY))
    {
        return nullptr;
    }

    FVector2D ScreenPosition(MouseX, MouseY);

    FVector WorldLocation, WorldDirection;
    if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLocation, WorldDirection))
    {
        FVector End = WorldLocation + (WorldDirection * 10000.0f);

        FHitResult HitResult;

        FCollisionQueryParams TraceParams(FName(TEXT("CursorTrace")), true, this);
        TraceParams.bTraceComplex = true;
        TraceParams.bReturnPhysicalMaterial = false;

        if (World->LineTraceSingleByChannel(HitResult, WorldLocation, End, ECC_Visibility, TraceParams))
        {
            DrawDebugLine(World, WorldLocation, HitResult.Location, FColor::Green, false, 1.0f, 0, 1.0f);
            DrawDebugSphere(World, HitResult.Location, 10.0f, 12, FColor::Red, false, 1.0f);

            return HitResult.GetActor();
        }
        else
        {
            return nullptr;
            UE_LOG(LogTemp, Warning, TEXT("No hit"));
        }
    }

    return nullptr;
}