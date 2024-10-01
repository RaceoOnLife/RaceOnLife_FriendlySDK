

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Json.h"
#include "GameFramework/Pawn.h"

#include "RaceOnLifeLibrary.generated.h"

UCLASS()
class RACEONLIFE_API URaceOnLifeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|Sound")
    static TArray<FString> GetInputDevices();
    
    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|Sound")
    static TArray<FString> GetOutputDevices();

    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|Sound")
    static bool SetOutputDevice(const FString& DeviceName);

    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|Sound")
    static FString GetCurrentInputDevice();

    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|Sound")
    static FString GetCurrentOutputDevice();

    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|Sound")
    static bool SetInputDevice(const FString& DeviceName);

    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|VehicleSpawn")
    static AActor* GetClosestActorOfClass(TSubclassOf<AActor> ActorClass, APawn* PawnReference);

    /*
     * Calculates an impulse vector based on the camera's forward direction and the vehicle's speed.
     * If the speed is less than or equal to 30 km/h, a weaker lobbed impulse is applied.
     * @param CameraComponent - The camera component to get the direction from.
     * @param VehicleSpeed - The speed of the vehicle from ChaosWheeledVehiclePawn, which influences the strength of the impulse.
     * @return Calculated impulse vector.
     */
    UFUNCTION(BlueprintPure, Category = "RaceOnLife Library|Football")
    static FVector CalculateImpulse(UCameraComponent* CameraComponent, float VehicleSpeed);

    UFUNCTION(BlueprintCallable, Category = "RaceOnLife Library|AntiAliasing")
    static void SetAntiAliasing(int32 Method);

    /* 

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static FString GetUserDataFilePath(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static TArray<FString> GetBuyedItems(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static void SetBuyedItems(APlayerController* PlayerController, const TArray<FString>& Items);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static void AddBuyedItem(APlayerController* PlayerController, const FString& Item);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static float GetBalance(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static void SetBalance(APlayerController* PlayerController, float Balance);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static int32 GetPlayedGames(APlayerController* PlayerController, const FString& GameMode);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static void SetPlayedGames(APlayerController* PlayerController, const FString& GameMode, int32 Count);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static void AddPlayedGame(APlayerController* PlayerController, const FString& GameMode);

    UFUNCTION(BlueprintCallable, Category = "User Data")
    static bool IsItemBuyed(APlayerController* PlayerController, const FString& ItemID);

private:
    static FString EncryptData(const FString& Data, const FString& Key);
    static FString DecryptData(const FString& EncryptedData, const FString& Key);

    */
};
