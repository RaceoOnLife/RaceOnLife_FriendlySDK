// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "Runtime/Online/HTTP/Public/Http.h"

#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/WindowsPlatformMisc.h"
#endif

#if PLATFORM_MAC
#include "Runtime/Core/Public/Apple/ApplePlatformMisc.h"
#endif

#if PLATFORM_LINUX
#include "Runtime/Core/Public/Linux/LinuxPlatformMisc.h"
#endif

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Modules/ModuleManager.h"
#include "ReplicaNPC.generated.h"

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FOnNPCRequestCompleted, const FString&, Url, const FString&, NewContext, const TArray<FString>&, Timeline, const TArray<FString>&, Phonemes, const FString&, SubTitle, const FString&, PlayerText, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FOnNPCRequestCompleted, const FString&, Url, const FString&, NewContext, const TArray<FString>&, Timeline, const TArray<FString>&, Phonemes, const FString&, SubTitle, const FString&, PlayerText, bool, bSuccess);

UENUM(BlueprintType)
enum Voices
{
	
	Vinnie	UMETA(DisplayName = "Vinnie"),
	Kaylee	UMETA(DisplayName = "Kaylee"),
	Nathan	UMETA(DisplayName = "Nathan"),
	Olivia	UMETA(DisplayName = "Olivia"),

};

UENUM(BlueprintType)
enum CurrentStyles
{
	Serious			UMETA(DisplayName = "Serious"),
	Sassy			UMETA(DisplayName = "Sassy"),
	Defeated		UMETA(DisplayName = "Defeated"),
	Warm            UMETA(DisplayName = "Warm"),
	
	

};

UENUM(BlueprintType)
enum ELanguageModel
{
	OpenAImodel			UMETA(DisplayName = "Open AI"),
	HostedLLMmodel		UMETA(DisplayName = "Hosted LLM"),
};

class FReplicaNPCModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

UCLASS()
class REPLICANPC_API UReplicaNPC : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ReplicaNPC", WorldContext = "WorldContextObject"))
		static UReplicaNPC* Replica(UObject* WorldContextObject, FString AudioPath, FString SpeechText, FString CharacterName, FString Context, TEnumAsByte<Voices> CurrentVoice, TEnumAsByte<CurrentStyles> CurrentStyle, ELanguageModel LanguageModelUsed);

	UPROPERTY(BlueprintAssignable)
		FOnNPCRequestCompleted Completed;
	
	//UPROPERTY(BlueprintAssignable)
		//FNewTestDelegate Completed;

	UPROPERTY()
		FString AudioPath;
	UPROPERTY()
		FString Context;
	UPROPERTY()
		FString SpeechText;

	UPROPERTY()
		FString CharacterName;

	UPROPERTY()
		FString HttpAddress;

	//UPROPERTY()
		//bool SwitchAPI;

	UPROPERTY()
		bool OpenAI;

	UPROPERTY()
		bool HostedLLM;

	

	UPROPERTY()
		TEnumAsByte<Voices> Voice;
	UPROPERTY()
		TEnumAsByte<CurrentStyles> Style;



protected:
	static FString GetEnvironmentVariable(FString key);

};
