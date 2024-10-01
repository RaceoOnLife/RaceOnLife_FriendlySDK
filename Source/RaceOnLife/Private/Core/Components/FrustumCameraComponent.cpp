#include "Core/Components/FrustumCameraComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraActor.h"

UFrustumCameraComponent::UFrustumCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFrustumCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CameraActor = Cast<ACameraActor>(UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetViewTarget());
}

void UFrustumCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CameraActor) return;

    FMinimalViewInfo ViewInfo;
    CameraActor->GetCameraComponent()->GetCameraView(DeltaTime, ViewInfo);

    FMatrix ViewProjectionMatrix = ViewInfo.CalculateProjectionMatrix();
    FConvexVolume FrustumVolume;
    GetViewFrustumBounds(FrustumVolume, ViewProjectionMatrix, true);

    FVector CameraLocation = CameraActor->GetActorLocation();

    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;

        TArray<UStaticMeshComponent*> StaticMeshComponents;
        Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

        bool bIsVisible = false;

        for (UStaticMeshComponent* MeshComp : StaticMeshComponents)
        {
            if (IsActorInView(MeshComp, FrustumVolume))
            {
                bIsVisible = true;
                break;
            }
        }

        Actor->SetActorHiddenInGame(!bIsVisible);
    }
}

bool UFrustumCameraComponent::IsActorInView(class UStaticMeshComponent* MeshComp, const FConvexVolume& FrustumVolume) const
{
    if (MeshComp)
    {
        FBoxSphereBounds MeshBounds = MeshComp->Bounds;

        return FrustumVolume.IntersectBox(MeshBounds.Origin, MeshBounds.BoxExtent);
    }

    return false;
}

