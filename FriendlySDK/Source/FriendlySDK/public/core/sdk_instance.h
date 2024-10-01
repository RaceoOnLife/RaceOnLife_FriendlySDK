#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "sdk_instance.generated.h"

UCLASS()
class FRIENDLYSDK_API Usdk_instance : public UGameInstance
{
    GENERATED_BODY()

public:
    Usdk_instance();

    UPROPERTY(BlueprintReadWrite, Category = "Player")
    int32 PlayerScore;

    UFUNCTION(BlueprintCallable, Category = "Player")
    void SetPlayerScore(int32 NewScore);

    UFUNCTION(BlueprintCallable, Category = "Player")
    int32 GetPlayerScore() const;
};