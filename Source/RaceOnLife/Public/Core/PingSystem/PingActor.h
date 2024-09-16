

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PingActor.generated.h"

UCLASS()
class RACEONLIFE_API APingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APingActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
