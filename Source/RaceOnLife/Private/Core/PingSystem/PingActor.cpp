


#include "Core/PingSystem/PingActor.h"

// Sets default values
APingActor::APingActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APingActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

