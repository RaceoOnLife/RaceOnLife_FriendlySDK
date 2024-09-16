#include "Core/Gamemodes/FootballRework/AI/FootballAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Core/Gamemodes/FootballRework/FootballGate.h"
#include "Core/Gamemodes/FootballRework/FootballBall.h"
#include <Navigation/PathFollowingComponent.h>
#include <AISystem.h>
#include "Core/Gamemodes/FootballRework/AI/AIVehicle.h"

void AFootballAIController::MoveInDirection(FVector Direction)
{
    AAIVehicle* Vehicle = Cast<AAIVehicle>(GetPawn());
    if (!Vehicle) return;

    Direction.Normalize();

    FVector ForwardVector = Vehicle->GetActorForwardVector();
    FVector RightVector = Vehicle->GetActorRightVector();

    float ForwardDotProduct = FVector::DotProduct(Direction, ForwardVector);
    float RightDotProduct = FVector::DotProduct(Direction, RightVector);

    Vehicle->MoveForward(ForwardDotProduct);
    Vehicle->MoveRight(RightDotProduct);
}

void AFootballAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AActor*> FoundBalls;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFootballBall::StaticClass(), FoundBalls);
	if (FoundBalls.Num() == 0) return;
	AFootballBall* Ball = Cast<AFootballBall>(FoundBalls[0]);

    TArray<AActor*> FoundGates;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFootballGate::StaticClass(), FoundGates);
    if (FoundGates.Num() < 2) return;
    AFootballGate* MyGate = nullptr;
    AFootballGate* TheirGate = nullptr;
    for (AActor* Actor : FoundGates)
    {
        AFootballGate* Gate = Cast<AFootballGate>(Actor);
        if (Gate->TeamGate == AITeam)
        {
            MyGate = Gate;
        }
        else
        {
            TheirGate = Gate;
        }
    }
    if (!MyGate || !TheirGate) return;

    MoveToActor(Ball, 10.0f);

    if (FVector::Dist(GetPawn()->GetActorLocation(), Ball->GetActorLocation()) < KickRadius)
    {
        FVector DriveDirection = (TheirGate->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal();
        MoveInDirection(DriveDirection);
    }
}
