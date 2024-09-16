

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballGate.generated.h"

UCLASS()
class RACEONLIFE_API AFootballGate : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFootballGate();

	UPROPERTY(BlueprintReadWrite, Category="Team")
	int TeamGate;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
