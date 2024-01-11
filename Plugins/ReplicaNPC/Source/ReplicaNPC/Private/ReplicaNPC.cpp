// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReplicaNPC.h"
#include "ReplicaSettings.h"


#define LOCTEXT_NAMESPACE "FReplicaNPCModule"

void FReplicaNPCModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FReplicaNPCModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FReplicaNPCModule, ReplicaNPC)


FString UReplicaNPC::GetEnvironmentVariable(FString key)
{
	FString result;
#if PLATFORM_WINDOWS
	result = FWindowsPlatformMisc::GetEnvironmentVariable(*key);
#endif
#if PLATFORM_MAC
	result = FApplePlatformMisc::GetEnvironmentVariable(*key);
#endif

#if PLATFORM_LINUX
	result = FLinuxPlatformMisc::GetEnvironmentVariable(*key);
#endif
	return result;
}


void UReplicaNPC::Activate()
{
	// Create Payload
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	// Data URI / Base64 Format the Audio Path
	TArray<uint8> WavData;
	FString DataURI = TEXT("data:audio/wav;base64,");

		if (!AudioPath.IsEmpty())
		{
			FFileHelper::LoadFileToArray(WavData, *AudioPath);
			FString Base64Data = FBase64::Encode(WavData);
			DataURI = DataURI.Append(Base64Data);

			
		};

	
	JsonObject->SetStringField(TEXT("voice"), UEnum::GetValueAsString(Voice));
	JsonObject->SetStringField(TEXT("style"), UEnum::GetValueAsString(Style));
	
	
	//Get API KEY from DefaultGame.ini
    FString ApiKey = "";
	GConfig->GetString(TEXT("/Script/ReplicaNPC.ReplicaSettings"),
		TEXT("API_Key"),
		ApiKey,
		GGameIni);

	JsonObject->SetStringField(TEXT("api_key"), ApiKey);


	JsonObject->SetStringField(TEXT("context"), *FString::Printf(TEXT("%s"), *Context));
	JsonObject->SetStringField(TEXT("audio"), *FString::Printf(TEXT("%s"), *DataURI));
	JsonObject->SetStringField(TEXT("character_name"), CharacterName);
	JsonObject->SetStringField(TEXT("text"), SpeechText);
	//If Text is empty, the API will generate text from audio file path (Speech to text).
	
	// Setting this to TRUE uses open ai gpt3 with better responses but the response time can be slower at times.
	JsonObject->SetBoolField(TEXT("use_openai"), OpenAI);
	JsonObject->SetBoolField(TEXT("use_hosted_llm"), HostedLLM);

	FString JSONString;
	TSharedRef<TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<>::Create(&JSONString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

	
	HttpAddress = "https://npc.replicastudios.com/e2e";
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(FString(HttpAddress));
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/json"));
	Request->SetContentAsString(JSONString);

	Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
		{
			FString StreamUrl = TEXT("Request Failed");
			FString NewContext = Context;
			TArray<FString> Phonemes;
			TArray<FString> Timeline;
			FString ResponseText = "";
			FString PlayerText = "";

			
			if (bSuccess)
			{
				TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
				TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());
				FJsonSerializer::Deserialize(JsonReader, JsonObject);

				JsonObject->TryGetStringField("url", StreamUrl);
				JsonObject->TryGetStringField("new_context", NewContext);
				JsonObject->TryGetStringField("response_text", ResponseText);
				JsonObject->TryGetStringField("transcription", PlayerText);
				JsonObject->TryGetStringArrayField("response_phonemes", Phonemes);
				JsonObject->TryGetStringArrayField("response_phonemes_timing", Timeline);
				UE_LOG(LogTemp, Warning, TEXT("Request Success"));
			}

			
			Completed.Broadcast(StreamUrl, NewContext, Timeline, Phonemes, ResponseText, PlayerText, bSuccess);
			UE_LOG(LogTemp, Warning, TEXT("Request Complete"));
		});
	

	Request->ProcessRequest();
	UE_LOG(LogTemp, Warning, TEXT("Processing Request"));
}


UReplicaNPC* UReplicaNPC::Replica(UObject* WorldContextObject, FString AudioPath, FString SpeechText, FString CharacterName, FString Context, TEnumAsByte<Voices> CurrentVoice, TEnumAsByte<CurrentStyles> CurrentStyle, ELanguageModel LanguageModelUsed)
{
	
	
	// Create Action Instance for Blueprint System
	UReplicaNPC* Action = NewObject<UReplicaNPC>();

	switch (LanguageModelUsed)
	{
	case OpenAImodel:
		Action->OpenAI = true;
		Action->HostedLLM = false;
		break;
	case HostedLLMmodel:
		Action->OpenAI = false;
		Action->HostedLLM = true;
		break;
	default:
		Action->OpenAI = true;
		Action->HostedLLM = false;
		break;
	}
	//Action->OpenAI = false;
	//Action->HostedLLM = true;
	Action->AudioPath = AudioPath;
	Action->SpeechText = SpeechText;
	Action->CharacterName = CharacterName;
	Action->Context = Context;
	Action->Voice = CurrentVoice;
	Action->Style = CurrentStyle;
	//Action->SwitchAPI = SwitchAPI;
	
	Action->RegisterWithGameInstance(WorldContextObject);

	return Action;
}
