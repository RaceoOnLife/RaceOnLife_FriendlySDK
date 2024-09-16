

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RaceOnLifePlayerControllerBase.generated.h"

/**
 * 
 */
UCLASS()
class RACEONLIFE_API ARaceOnLifePlayerControllerBase : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="User Data")
	FString GetUserID() const;
};
