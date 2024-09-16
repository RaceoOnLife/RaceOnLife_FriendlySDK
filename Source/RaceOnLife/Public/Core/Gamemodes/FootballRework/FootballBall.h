

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballBall.generated.h"

UCLASS()
class RACEONLIFE_API AFootballBall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFootballBall();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
