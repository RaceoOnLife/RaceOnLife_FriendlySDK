#include "lib/sdk_subsystem.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "JsonUtilities.h"

const FString usdk_subsystem::sdk_api = TEXT("your-api-key-there");
/*
    The API key can be created on the website https://localhost/sdk/developer/api
*/

void usdk_subsystem::GetWalletData(const FString& WalletAddress, FString& OutBalance, FString& OutAddress)
{
    FString Url = FString::Printf(TEXT("https://localhost/getWalletData?address=%s"), *WalletAddress);
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    SendHttpRequest(Url, TEXT("GET"), TEXT(""), Headers, [this, &OutBalance, &OutAddress](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            FString ResponseContent = Response->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                OutBalance = JsonObject->GetStringField(TEXT("balance"));
                OutAddress = JsonObject->GetStringField(TEXT("address"));
            }
        }
    });
}

void usdk_subsystem::PurchaseOperation(const FString& ItemID, int32 Amount)
{
    FString Url = TEXT("https://localhost/purchase");
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetStringField(TEXT("item_id"), ItemID);
    RequestJson->SetNumberField(TEXT("amount"), Amount);

    FString RequestContent;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestContent);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

    SendHttpRequest(Url, TEXT("POST"), RequestContent, Headers, [](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        // response processing will be added later.
    });
}

void usdk_subsystem::CreditOperation(const FString& UserID, int32 Amount)
{
    FString Url = TEXT("https://localhost/credit");
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetStringField(TEXT("user_id"), UserID);
    RequestJson->SetNumberField(TEXT("amount"), Amount);

    FString RequestContent;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestContent);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

    SendHttpRequest(Url, TEXT("POST"), RequestContent, Headers, [](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        // response processing will be added later.
    });
}

FString usdk_subsystem::GetUserID()
{
    FString Url = TEXT("https://localhost/getuserid");
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    SendHttpRequest(Url, TEXT("GET"), TEXT(""), Headers, [Callback](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        FString UserID = TEXT("");
        if (bWasSuccessful && Response.IsValid())
        {
            FString ResponseContent = Response->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                UserID = JsonObject->GetStringField(TEXT("user_id"));
            }
        }
        Callback.ExecuteIfBound(UserID);
    });
}

FString usdk_subsystem::GetUserNickname()
{
    FString Url = TEXT("https://localhost/getusernickname");
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    SendHttpRequest(Url, TEXT("GET"), TEXT(""), Headers, [Callback](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        FString Nickname = TEXT("");
        if (bWasSuccessful && Response.IsValid())
        {
            FString ResponseContent = Response->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                Nickname = JsonObject->GetStringField(TEXT("nickname"));
            }
        }
        Callback.ExecuteIfBound(Nickname);
    });
}

void usdk_subsystem::GetSave(const FString& SaveName, TArray<uint8>& OutSaveData)
{
    FString Url = FString::Printf(TEXT("http://localhost/getsavedata?save_name=%s"), *SaveName);
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    SendHttpRequest(Url, TEXT("GET"), TEXT(""), Headers, [&OutSaveData](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            OutSaveData = Response->GetContent();
        }
    });
}

void usdk_subsystem::SendAndRemoveSave(const FString& SaveName, const TArray<uint8>& SaveData)
{
    FString Url = TEXT("https://localhost/savesave");
    TMap<FString, FString> Headers;
    Headers.Add(TEXT("Authorization"), sdk_api);

    FString EncodedSaveData = FBase64::Encode(SaveData);

    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetStringField(TEXT("save_name"), SaveName);
    RequestJson->SetStringField(TEXT("save_data"), EncodedSaveData);

    FString RequestContent;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestContent);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

    SendHttpRequest(Url, TEXT("POST"), RequestContent, Headers, [SaveName, this](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            FString SavePath = FPaths::ProjectSavedDir() + SaveName;
            IFileManager::Get().Delete(*SavePath);
        }
    });
}

void usdk_subsystem::SendHttpRequest(const FString& Url, const FString& Verb, const FString& Content, const TMap<FString, FString>& Headers, TFunction<void(FHttpResponsePtr, bool)> Callback)
{
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(Verb);
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    for (const auto& Header : Headers)
    {
        Request->SetHeader(Header.Key, Header.Value);
    }

    if (!Content.IsEmpty())
    {
        Request->SetContentAsString(Content);
    }

    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bWasSuccessful)
    {
        Callback(Resp, bWasSuccessful);
    });

    Request->ProcessRequest();
}
