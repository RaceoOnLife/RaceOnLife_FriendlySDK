#include "lib/sdk_lib.h"
#include "HttpModule.h"
#include "Http.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "core/sdk_instance.h"

bool Usdk_lib::IsPlayerHaveNetwork()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://www.google.com"));
    Request->SetVerb(TEXT("GET"));

    bool bHasNetwork = false;

    Request->OnProcessRequestComplete().BindLambda([&bHasNetwork](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
        {
            bHasNetwork = true;
        }
    });

    Request->ProcessRequest();
    FPlatformProcess::Sleep(0.5f);

    return bHasNetwork;
}

bool Usdk_lib::IsPlayerStartedAtClient()
{
    if (GEngine)
    {
        FString IsGameStartedAtClientStr;
        if (GEngine->GetWorld()->GetGameInstance()->GetStringOption(TEXT("IsGameStartedAtClient"), IsGameStartedAtClientStr) && IsGameStartedAtClientStr.Equals(TEXT("true"), ESearchCase::IgnoreCase))
        {
            return true;
        }
    }
    return false;
}

TArray<FString> Usdk_lib::GetPlayerData(int32 DataID)
{
    TArray<FString> PlayerData;
    FString FilePath = FPaths::Combine(FPaths::AppDataDir(), TEXT("EternityLife/appid/Data/userdata.json"));
    
    FString JsonContent;
    if (FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);

        if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
        {
            switch (DataID)
            {
                case 0: PlayerData.Add(JsonObject->GetStringField(TEXT("ID"))); break;
                case 1: PlayerData.Add(JsonObject->GetStringField(TEXT("EMail"))); break;
                case 2: PlayerData.Add(JsonObject->GetStringField(TEXT("Username"))); break;
                case 3: PlayerData.Add(JsonObject->GetStringField(TEXT("WalletAddress"))); break;
                case 4: PlayerData.Add(JsonObject->GetStringField(TEXT("Balance"))); break;
                case 5:
                {
                    const TArray<TSharedPtr<FJsonValue>>* InventoryArray;
                    if (JsonObject->TryGetArrayField(TEXT("Inventory"), InventoryArray))
                    {
                        for (const auto& Item : *InventoryArray)
                        {
                            PlayerData.Add(Item->AsString());
                        }
                    }
                    break;
                }
                default: break;
            }
        }
    }
    
    return PlayerData;
}

bool Usdk_lib::PingServer(const FString& ServerAddress)
{
    // Создание HTTP-запроса для проверки доступности сервера
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ServerAddress);
    Request->SetVerb(TEXT("HEAD"));

    bool bIsServerAvailable = false;

    Request->OnProcessRequestComplete().BindLambda([&bIsServerAvailable](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
        {
            bIsServerAvailable = true;
        }
    });

    Request->ProcessRequest();
    FPlatformProcess::Sleep(0.5f);

    return bIsServerAvailable;
}

int32 Usdk_lib::GetPlayerScore()
{
    Usdk_instance* sdk_instance = Cast<Usdk_instance>(UGameplayStatics::GetGameInstance(GWorld));
    if (sdk_instance)
    {
        return sdk_instance->GetPlayerScore();
    }
    
    return 0;
}

void Usdk_lib::SetPlayerScore(int32 NewScore)
{
    Usdk_instance* sdk_instance = Cast<Usdk_instance>(UGameplayStatics::GetGameInstance(GWorld));
    if (sdk_instance)
    {
        sdk_instance->SetPlayerScore(NewScore);
    }
}

FDateTime Usdk_lib::GetCurrentGameTime()
{
    return FDateTime::Now();
}

TMap<FString, FString> Usdk_lib::GetGameSettings()
{
    return "";
    // in the future
}

void Usdk_lib::SetGameSetting(const FString& SettingName, const FString& SettingValue)
{
    return;
    // in the future
}

FString Usdk_lib::GetSDKVersion()
{
    return FString("0.0.1-prt");
}

FString Usdk_lib::GetPlatformName()
{
#if PLATFORM_WIN64
    return FString("Windows");
#elif PLATFORM_MAC
    return FString("Mac");
#elif PLATFORM_LINUX
    return FString("Linux");
#elif PLATFORM_ANDROID
    return FString("Android");
#elif PLATFORM_IOS
    return FString("iOS");
#else
    return FString("Other");
#endif
}