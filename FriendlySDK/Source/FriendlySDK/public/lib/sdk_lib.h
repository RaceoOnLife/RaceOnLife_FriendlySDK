#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "sdk_lib.generated.h"

UCLASS()
class FRIENDLYSDK_API Usdk_lib : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Network")
    static bool IsPlayerHaveNetwork();

    UFUNCTION(BlueprintCallable, Category = "Game")
    static bool IsPlayerStartedAtClient();

    UFUNCTION(BlueprintCallable, Category = "Data")
    static TArray<FString> GetPlayerData(int32 DataID);

    UFUNCTION(BlueprintCallable, Category = "Network")
    static bool PingServer(const FString& ServerAddress);

    UFUNCTION(BlueprintCallable, Category = "Player")
    static int32 GetPlayerScore();

    UFUNCTION(BlueprintCallable, Category = "Player")
    static void SetPlayerScore(int32 NewScore);

    UFUNCTION(BlueprintCallable, Category = "Time")
    static FDateTime GetCurrentGameTime();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static TMap<FString, FString> GetGameSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SetGameSetting(const FString& SettingName, const FString& SettingValue);

    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static FString GetSDKVersion();

    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static FString GetPlatformName();
};
