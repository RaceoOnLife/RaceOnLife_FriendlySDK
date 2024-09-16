#include "Core/GameCoreInstance.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

void UGameCoreInstance::Init()
{
	Super::Init();
}

bool UGameCoreInstance::CheckIsStartedFromClient()
{
    const TCHAR* CommandLine = FCommandLine::Get();

    TArray<FString> Args;
    FString(CommandLine).ParseIntoArrayWS(Args);

    bool bIsGameStartedAtLauncher = false;

    for (const FString& Arg : Args)
    {
        if (Arg.StartsWith(TEXT("IsGameStartedAtLauncher=")))
        {
            FString Value = Arg.RightChop(24);
            if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
            {
                bIsGameStartedAtLauncher = true;
                break;
            }
        }
    }

    if (bIsGameStartedAtLauncher)
    {
        UE_LOG(LogTemp, Log, TEXT("Game started from client"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Game doesn't started from client"));
    }

    return bIsGameStartedAtLauncher;
}
