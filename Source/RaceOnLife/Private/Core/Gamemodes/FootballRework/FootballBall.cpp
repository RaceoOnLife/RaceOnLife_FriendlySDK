


#include "Core/Gamemodes/FootballRework/FootballBall.h"

// Sets default values
AFootballBall::AFootballBall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFootballBall::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFootballBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

