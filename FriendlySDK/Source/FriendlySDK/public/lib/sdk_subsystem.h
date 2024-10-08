#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "sdk_subsystem.generated.h"

UCLASS()
class FRIENDLYSDK_API USDKSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static const FString sdk_api;

    UFUNCTION(BlueprintCallable, Category = "Web3")
    void GetWalletData(const FString& WalletAddress, FString& OutBalance, FString& OutAddress);

    UFUNCTION(BlueprintCallable, Category = "Web3")
    void PurchaseOperation(const FString& ItemID, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Web3")
    void CreditOperation(const FString& UserID, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "User")
    void GetUserID(const FOnGetUserData& Callback);

    UFUNCTION(BlueprintCallable, Category = "User")
    void GetUserNickname(const FOnGetUserData& Callback);

    UFUNCTION(BlueprintCallable, Category = "SaveData")
    void GetSave(const FString& SaveName, TArray<uint8>& OutSaveData);

    UFUNCTION(BlueprintCallable, Category = "SaveData")
    void SendAndRemoveSave(const FString& SaveName, const TArray<uint8>& SaveData);

private:
    void SendHttpRequest(const FString& Url, const FString& Verb, const FString& Content, const TMap<FString, FString>& Headers, TFunction<void(FHttpResponsePtr, bool)> Callback);
};
