// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleTestControllerBase.h"

#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "PlatformFeatures.h"
#include "VideoRecordingSystem.h"

#include "GauntletModule.h"

#include "Game/CitySamplePlayerController.h"


DEFINE_LOG_CATEGORY(LogCitySampleTest);

namespace CitySampleTest
{
	static TAutoConsoleVariable<int32> CVarMaxRunCount(
		TEXT("CitySampleTest.MaxRunCount"),
		1,
		TEXT("Max number of runs desired for this session."),
		ECVF_Default);

	static TAutoConsoleVariable<FString> CVarMemReportArgs(
		TEXT("CitySampleTest.MemReportArgs"),
		TEXT("-full -csv"),
		TEXT("Duration in seconds between MemReport calls during automated testing."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarMemReportInterval(
		TEXT("CitySampleTest.MemReportInterval"),
		60.0f * 30.0f,
		TEXT("Duration in seconds between MemReport calls during automated testing."),
		ECVF_Default);
}


UCitySampleTestControllerBase::UCitySampleTestControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentGameState(nullptr)
	, WarnStuckTime(10 * 60)
	, NextWarnStuckTime(0)
	, RunCount(0)
	, bRequestsFPSChart(false)
	, bRequestsMemReport(false)
	, bRequestsVideoCapture(false)
{
}


void UCitySampleTestControllerBase::OnInit()
{
	Super::OnInit();

	int32 MaxRunCount;
	if (FParse::Value(FCommandLine::Get(), TEXT("CitySampleTest.MaxRunCount"), MaxRunCount))
	{
		CitySampleTest::CVarMaxRunCount->Set(MaxRunCount);
	}

	bRequestsFPSChart = FParse::Param(FCommandLine::Get(), TEXT("CitySampleTest.FPSChart"));
	bRequestsMemReport = FParse::Param(FCommandLine::Get(), TEXT("CitySampleTest.MemReport"));
	bRequestsVideoCapture = FParse::Param(FCommandLine::Get(), TEXT("CitySampleTest.VideoCapture"));

	MemReportTimerDelegate.BindUObject(this, &ThisClass::OnMemReportTimerExpired);

	const FConsoleCommandDelegate MemReportIntervalDelegate = FConsoleCommandDelegate::CreateUObject(this, &ThisClass::OnMemReportIntervalChanged);
	MemReportIntervalSink = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(MemReportIntervalDelegate);

	FWorldDelegates::OnPreWorldInitialization.AddUObject(this, &ThisClass::OnPreWorldInitializeInternal);
}

void UCitySampleTestControllerBase::OnTick(float TimeDelta)
{
	if ((NextWarnStuckTime > 0) && (GetTimeInCurrentState() >= NextWarnStuckTime))
	{
		FString CurrentStateName = CurrentGameState.IsValid() ? CurrentGameState->GetName() : FString(TEXT("NoGameState"));
		UE_LOG(LogCitySampleTest, Display, TEXT("Have been in state %d for %.02f mins"), *CurrentStateName, GetTimeInCurrentState() / 60.0f);
		NextWarnStuckTime += WarnStuckTime;
	}
}

void UCitySampleTestControllerBase::OnStateChange(FName OldState, FName NewState)
{
	if ((NewState != GetCurrentState()) && (NewState != FGauntletStates::Initialized))
	{
		NextWarnStuckTime = WarnStuckTime;
	}

	check(GetWorld());
	CurrentGameState = GetWorld()->GetGameState<ACitySampleGameState>();
}

void UCitySampleTestControllerBase::BeginDestroy()
{
	UnbindAllDelegates();
	Super::BeginDestroy();
}

void UCitySampleTestControllerBase::EndCitySampleTest(const int32 ExitCode/*=0*/)
{
	UnbindAllDelegates();

	UE_LOG(LogCitySampleTest, Display, TEXT("%s: test completed, requesting exit..."), ANSI_TO_TCHAR(__func__));
	EndTest(ExitCode);
}

void UCitySampleTestControllerBase::UnbindAllDelegates()
{
	ClearMemReportTimer();
	MemReportTimerDelegate.Unbind();
	IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(MemReportIntervalSink);
	FWorldDelegates::OnPreWorldInitialization.RemoveAll(this);
}

ACitySamplePlayerController* UCitySampleTestControllerBase::GetCitySamplePlayerController()
{
	return Cast<ACitySamplePlayerController>(GetFirstPlayerController());
}

ACitySampleGameState* UCitySampleTestControllerBase::GetCitySampleGameState()
{
	if (UWorld* const World = GetWorld())
	{
		return World->GetGameState<ACitySampleGameState>();
	}

	return nullptr;
}

uint32 UCitySampleTestControllerBase::GetRunCount() const
{
	return RunCount;
}

uint32 UCitySampleTestControllerBase::GetMaxRunCount() const
{
	const int32 MaxRunCount = CitySampleTest::CVarMaxRunCount->GetInt();
	return MaxRunCount > 0 ? MaxRunCount : TNumericLimits<uint32>::Max();
}

uint32 UCitySampleTestControllerBase::GetRunsRemaining() const
{
	return GetMaxRunCount() - GetRunCount();
}

uint32 UCitySampleTestControllerBase::MarkRunComplete()
{
	++RunCount;
	UE_LOG(LogCitySampleTest, Display, TEXT("%s: run complete (%u/%u)..."), ANSI_TO_TCHAR(__func__), GetRunCount(), GetMaxRunCount());
	return RunCount;
}

bool UCitySampleTestControllerBase::RequestsFPSChart()
{
	return bRequestsFPSChart;
}

bool UCitySampleTestControllerBase::RequestsMemReport()
{
	return bRequestsMemReport;
}

bool UCitySampleTestControllerBase::RequestsVideoCapture()
{
	return bRequestsVideoCapture;
}

void UCitySampleTestControllerBase::ConsoleCommand(const TCHAR* Cmd)
{
	if (APlayerController* Controller = GetFirstPlayerController())
	{
		Controller->ConsoleCommand(Cmd);
		UE_LOG(LogCitySampleTest, Display, TEXT("Issued console command: '%s'"), Cmd);
	}
}

void UCitySampleTestControllerBase::ExecuteMemReport(const TOptional<FString> Args/*=TOptional<FString>()*/)
{
	ConsoleCommand(*FString(TEXT("MemReport ") + (Args.IsSet() ? Args.GetValue() : CitySampleTest::CVarMemReportArgs->GetString())));
}

void UCitySampleTestControllerBase::SetMemReportTimer(const TOptional<float> Interval/*=TOptional<float>()*/)
{
	if (UWorld* const World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();

		const float AdjustedInterval = Interval.IsSet() ? Interval.GetValue() : CitySampleTest::CVarMemReportInterval->GetFloat();
		if (AdjustedInterval > 0.0f)
		{
			UE_LOG(LogCitySampleTest, Display, TEXT("%s: interval is set to %f seconds..."), ANSI_TO_TCHAR(__func__), AdjustedInterval);
			TimerManager.SetTimer(MemReportTimerHandle, MemReportTimerDelegate, AdjustedInterval, true);
		}
		else if (MemReportTimerHandle.IsValid())
		{
			UE_LOG(LogCitySampleTest, Display, TEXT("%s: interval is set to 0, pausing timer..."), ANSI_TO_TCHAR(__func__));
			TimerManager.PauseTimer(MemReportTimerHandle);
		}
		else
		{
			UE_LOG(LogCitySampleTest, Warning, TEXT("%s: interval is set to 0 and no timer is set, skipping..."), ANSI_TO_TCHAR(__func__));
		}
	}
}

void UCitySampleTestControllerBase::ClearMemReportTimer()
{
	if (UWorld* const World = GetWorld())
	{
		if (MemReportTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(MemReportTimerHandle);
			UE_LOG(LogCitySampleTest, Display, TEXT("%s: cleared..."), ANSI_TO_TCHAR(__func__));
		}
	}
}

bool UCitySampleTestControllerBase::TryStartingVideoCapture()
{
	if (IVideoRecordingSystem* const VideoRecordingSystem = IPlatformFeaturesModule::Get().GetVideoRecordingSystem())
	{
		const EVideoRecordingState RecordingState = VideoRecordingSystem->GetRecordingState();

		if (RecordingState == EVideoRecordingState::None)
		{
			VideoRecordingSystem->EnableRecording(true);

			const FString Suffix = FString::Printf(TEXT("_%s"), *FDateTime::Now().ToString(TEXT("%H-%M-%S")));
			const FString VideoFilename = FString::Printf(TEXT("AutoTest_CL-%d%s"), FEngineVersion::Current().GetChangelist(), *Suffix);
			VideoRecordingTitle = FText::FromString(VideoFilename);
			FVideoRecordingParameters VideoRecordingParameters(VideoRecordingSystem->GetMaximumRecordingSeconds(), true, false, false, FPlatformMisc::GetPlatformUserForUserIndex(0));
			VideoRecordingSystem->NewRecording(*VideoFilename, VideoRecordingParameters);

			if (VideoRecordingSystem->IsEnabled())
			{
				if (RecordingState == EVideoRecordingState::Starting || RecordingState == EVideoRecordingState::Recording)
				{
					UE_LOG(LogCitySampleTest, Log, TEXT("AutoTest %s: starting video recording %s..."), ANSI_TO_TCHAR(__func__), *VideoFilename);
					return true;
				}
				else
				{
					UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: failed to start video recording %s."), ANSI_TO_TCHAR(__func__), *VideoFilename);
				}
			}
			else
			{
				UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: video recording could not be enabled."), ANSI_TO_TCHAR(__func__));
			}
		}
		else
		{
			UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: could not start a new recording, may be already recording."), ANSI_TO_TCHAR(__func__));
		}
	}
	else
	{
		UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: video recording system is null."), ANSI_TO_TCHAR(__func__));
	}

	return false;
}

bool UCitySampleTestControllerBase::TryFinalizingVideoCapture(const bool bStopAutoContinue/*=false*/)
{
	if (IVideoRecordingSystem* const VideoRecordingSystem = IPlatformFeaturesModule::Get().GetVideoRecordingSystem())
	{
		if (VideoRecordingSystem->GetRecordingState() != EVideoRecordingState::None)
		{
			VideoRecordingSystem->FinalizeRecording(true, VideoRecordingTitle, FText::GetEmpty(), bStopAutoContinue);

			if (VideoRecordingSystem->GetRecordingState() == EVideoRecordingState::Finalizing)
			{
				UE_LOG(LogCitySampleTest, Log, TEXT("AutoTest %s: finalizing recording..."), ANSI_TO_TCHAR(__func__));
				return true;
			}
		}
	}
	else
	{
		UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: video recording system is null."), ANSI_TO_TCHAR(__func__));
	}

	return false;
}

void UCitySampleTestControllerBase::OnPreWorldInitializeInternal(UWorld* World, const UWorld::InitializationValues IVS)
{
	TryEarlyExec(World);
	OnPreWorldInitialize(World);
}

void UCitySampleTestControllerBase::TryEarlyExec(UWorld* const World)
{
	check(World);

	if (GEngine)
	{
		// Search the list of deferred commands
		const TArray<FString>& DeferredCmds = GEngine->DeferredCommands;
		TArray<int32> ExecutedIndices;
		for (int32 DeferredCmdIndex = 0; DeferredCmdIndex < DeferredCmds.Num(); ++DeferredCmdIndex)
		{
			// If the deferred command is one that should be executed early
			const FString& DeferredCmd = DeferredCmds[DeferredCmdIndex];
			if (CmdsToExecEarly.ContainsByPredicate([&DeferredCmd](const FString& CmdToFind) { return DeferredCmd.StartsWith(CmdToFind); }))
			{
				UE_LOG(LogCitySampleTest, Display, TEXT("%s: executing '%s' early."), ANSI_TO_TCHAR(__func__), *DeferredCmd);
				GEngine->Exec(World, *DeferredCmd);
				ExecutedIndices.Push(DeferredCmdIndex);
			}
		}

		// Remove the executed commands from the list of deferred commands
		// Note: This is done in reverse order to ensure the cached indices remain valid
		while (!ExecutedIndices.IsEmpty())
		{
			GEngine->DeferredCommands.RemoveAt(ExecutedIndices.Pop());
		}
	}
}

void UCitySampleTestControllerBase::OnMemReportTimerExpired()
{
	ExecuteMemReport();
}

void UCitySampleTestControllerBase::OnMemReportIntervalChanged()
{
	if (MemReportTimerHandle.IsValid())
	{
		if (UWorld* const World = GetWorld())
		{
			if (World->GetTimerManager().GetTimerRate(MemReportTimerHandle) != CitySampleTest::CVarMemReportInterval->GetFloat())
			{
				SetMemReportTimer();
			}
		}
	}
}
