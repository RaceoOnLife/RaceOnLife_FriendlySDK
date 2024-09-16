#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameCoreInstance.generated.h"

/**
 * 
 */
UCLASS()
class RACEONLIFE_API UGameCoreInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category="Game")
	bool CheckIsStartedFromClient();

};
