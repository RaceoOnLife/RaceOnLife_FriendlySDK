

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FootballAIController.generated.h"

/**
 * 
 */
UCLASS()
class RACEONLIFE_API AFootballAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category="Team")
	int AITeam;

private:

	void MoveInDirection(FVector Direction);

	float KickRadius = 0.0f;
	virtual void Tick(float DeltaTime) override;
};
