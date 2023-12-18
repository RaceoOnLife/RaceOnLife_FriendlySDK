// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tests/Controllers/CitySampleTestControllerAutoTest.h"

#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Logging/LogVerbosity.h"
#include "PlatformFeatures.h"
#include "VideoRecordingSystem.h"

#include "Game/CitySampleGameMode.h"
#include "Game/CitySampleGameState.h"
#include "Game/CitySamplePlayerController.h"
#include "UI/CitySampleUIComponent.h"


namespace CitySampleTest
{
	static TAutoConsoleVariable<float> CVarSoakTime(
		TEXT("CitySampleTest.SoakTime"),
		30.0f,
		TEXT("Duration of the sandbox soak, in seconds, per run."),
		ECVF_Default);

	static TAutoConsoleVariable<bool> CVarSkipTestSequence(
		TEXT("CitySampleTest.SkipTestSequence"),
		false,
		TEXT("Whether to skip the test sequence after the sandbox soak, if applicable."),
		ECVF_Default);

	static TAutoConsoleVariable<bool> CVarRequestShutdown(
		TEXT("CitySampleTest.RequestShutdown"),
		false,
		TEXT("Whether to request shutdown when the current run is finished, regardless of NumRuns."),
		ECVF_Default);
}

//////////////////////////////////////////////////////////////////////
// States
//////////////////////////////////////////////////////////////////////

// Base
void FCitySampleAutoTestState::Start(const ECitySampleAutoTestState PrevState)
{
	check(IsValid(Controller));
	TimeSinceStart = 0.0;
}

void FCitySampleAutoTestState::Tick(const float TimeDelta)
{
	TimeSinceStart += TimeDelta;
}

// InitialLoad
class FCitySampleAutoTestState_InitialLoad : public FCitySampleAutoTestState
{
public:
	FCitySampleAutoTestState_InitialLoad(UCitySampleTestControllerAutoTest* const TestController) : FCitySampleAutoTestState(TestController) {}

	virtual void Tick(float TimeDelta) override
	{
		Super::Tick(TimeDelta);

		if (GetTestStateTime() >= MaxStateChangeWait)
		{
			UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest InitialLoad %s: waited for state change for %f seconds, bindings may be missing, validate current state."), ANSI_TO_TCHAR(__func__), MaxStateChangeWait);
			Controller->SetTestState(ECitySampleAutoTestState::Soak);
		}
	}

private:
	const float MaxStateChangeWait = 30.0f;
};

// Sandbox Intro
class FCitySampleAutoTestState_Intro : public FCitySampleAutoTestState
{
public:
	FCitySampleAutoTestState_Intro(UCitySampleTestControllerAutoTest* const TestController) : FCitySampleAutoTestState(TestController) {}

	virtual void Start(const ECitySampleAutoTestState PrevState) override
	{
		Super::Start(PrevState);

		// If starting a new run, kick off requested captures

		if (Controller->RequestsVideoCapture())
		{
			Controller->TryStartingVideoCapture();
		}

		if (Controller->RequestsFPSChart())
		{
			Controller->ConsoleCommand(TEXT("StartFPSChart"));
		}

		const ACitySampleGameState* const GameState = Controller->GetCitySampleGameState();
		if (!GameState || !GameState->IsSandboxIntroPlaying())
		{
			UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest Intro %s: intro is not playing..."), ANSI_TO_TCHAR(__func__));
			Controller->SetTestState(ECitySampleAutoTestState::Soak);
		}
	}
};

// Sandbox Soak
class FCitySampleAutoTestState_Soak : public FCitySampleAutoTestState
{
public:
	FCitySampleAutoTestState_Soak(UCitySampleTestControllerAutoTest* const TestController) : FCitySampleAutoTestState(TestController) {}

	virtual void Start(const ECitySampleAutoTestState PrevState) override
	{
		Super::Start(PrevState);

		// If starting a new run, kick off requested captures
		if (PrevState == ECitySampleAutoTestState::InitialLoad || PrevState == ECitySampleAutoTestState::Finished)
		{
			if (Controller->RequestsVideoCapture())
			{
				Controller->TryStartingVideoCapture();
			}

			if (Controller->RequestsFPSChart())
			{
				Controller->ConsoleCommand(TEXT("StartFPSChart"));
			}
		}

		const float SoakTime = CitySampleTest::CVarSoakTime->GetFloat();
		if (SoakTime > 0.0f)
		{
			UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest Soak %s: soak time started for %f seconds..."), ANSI_TO_TCHAR(__func__), SoakTime);
		}
		else
		{
			UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest Soak %s: soaking indefinitely until soak time is set >0..."), ANSI_TO_TCHAR(__func__));
		}
	}

	virtual void Tick(float TimeDelta) override
	{
		Super::Tick(TimeDelta);
		
		const float SoakTime = CitySampleTest::CVarSoakTime->GetFloat();
		if (SoakTime > 0.0f && GetTestStateTime() >= SoakTime)
		{
			if (!CitySampleTest::CVarSkipTestSequence->GetBool())
			{
				Controller->SetTestState(ECitySampleAutoTestState::TestSequence);
			}
			else
			{
				Controller->SetTestState(ECitySampleAutoTestState::Finished);
			}
		}		
	}
};

// Sandbox Test Sequence
class FCitySampleAutoTestState_TestSequence : public FCitySampleAutoTestState
{
public:
	FCitySampleAutoTestState_TestSequence(UCitySampleTestControllerAutoTest* const TestController) : FCitySampleAutoTestState(TestController) {}

	virtual void Start(const ECitySampleAutoTestState PrevState) override
	{
		Super::Start(PrevState);

		if (ACitySampleGameState* const GameState = Controller->GetCitySampleGameState())
		{
			if (GameState && GameState->StartTestSequence())
			{
				UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest Test Sequence %s: starting sandbox test sequence, waiting for finished event..."), ANSI_TO_TCHAR(__func__));
			}
			else 
			{
				UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest Test Sequence %s: unable to start test sequence, may not be implemented..."), ANSI_TO_TCHAR(__func__));
				Controller->SetTestState(ECitySampleAutoTestState::Finished);
			}
		}
	}
};

// Finished
class FCitySampleAutoTestState_Finished : public FCitySampleAutoTestState
{
public:
	FCitySampleAutoTestState_Finished(UCitySampleTestControllerAutoTest* const TestController) : FCitySampleAutoTestState(TestController) {}

	virtual void Start(const ECitySampleAutoTestState PrevState) override
	{
		Super::Start(PrevState);

		if (Controller->RequestsMemReport())
		{
			Controller->ExecuteMemReport();
		}

		Controller->MarkRunComplete();

		// Success validation goes here...

		bWaitingOnFPSChart = false;
		bWaitingOnFinalizingVideo = false;

#if CSV_PROFILER
		if (FCsvProfiler* const CsvProfiler = FCsvProfiler::Get())
		{
			CsvProfilerDelegateHandle = CsvProfiler->OnCSVProfileFinished().AddLambda([this](const FString& Filename)
			{
				UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest OnCSVProfileFinished: CsvProfiler finished..."), ANSI_TO_TCHAR(__func__));
				bWaitingOnFPSChart = false;
			});

			if (CsvProfiler->IsCapturing())
			{
				bWaitingOnFPSChart = true;
				Controller->ConsoleCommand(TEXT("StopFPSChart"));
			}
			else
			{
				bWaitingOnFPSChart = CsvProfiler->IsWritingFile();
			}

		}
#endif // CSV_PROFILER

		if (IVideoRecordingSystem* const VideoRecordingSystem = IPlatformFeaturesModule::Get().GetVideoRecordingSystem())
		{
			VideoRecordingDelegateHandle = VideoRecordingSystem->GetOnVideoRecordingFinalizedDelegate().AddLambda([this](bool bSucceeded, const FString& FilePath)
			{
				if (bSucceeded)
				{
					UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest OnVideoRecordingFinalized: video recording successfully finalized at %s."), *FilePath);
				}
				else
				{
					UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest OnVideoRecordingFinalized: video recording failed to finalize..."));
				}

				bWaitingOnFinalizingVideo = false;
			});
		}

		bWaitingOnFinalizingVideo = Controller->TryFinalizingVideoCapture(true);
	}

	virtual void Tick(const float TimeDelta) override
	{
		Super::Tick(TimeDelta);

		// Checks whether the test shutdown is complete and the test is ready to end
		if (!bWaitingOnFPSChart && !bWaitingOnFinalizingVideo)
		{
			if (Controller->GetRunsRemaining() > 0 && !CitySampleTest::CVarRequestShutdown->GetBool())
			{
				const UWorld* const World = Controller->GetWorld();
				if (ACitySampleGameMode* const GameMode = World ? World->GetAuthGameMode<ACitySampleGameMode>() : nullptr)
				{
					GameMode->RestartGame();
				}
			}
			else
			{
				// If all runs are complete, then cleanup and shutdown test
				Controller->SetTestState(ECitySampleAutoTestState::Shutdown);
			}
		}
	}

	virtual void End(const ECitySampleAutoTestState NewState) override
	{
#if CSV_PROFILER
		if (FCsvProfiler* const CsvProfiler = FCsvProfiler::Get())
		{
			CsvProfiler->OnCSVProfileFinished().Remove(CsvProfilerDelegateHandle);
		}
#endif // CSV_PROFILER

		if (IVideoRecordingSystem* const VideoRecordingSystem = IPlatformFeaturesModule::Get().GetVideoRecordingSystem())
		{
			VideoRecordingSystem->GetOnVideoRecordingFinalizedDelegate().Remove(VideoRecordingDelegateHandle);
		}

		Super::End(NewState);
	}

private:
	bool bWaitingOnFPSChart = false;
	bool bWaitingOnFinalizingVideo = false;

	FDelegateHandle CsvProfilerDelegateHandle;
	FDelegateHandle VideoRecordingDelegateHandle;
};

// Finished
class FCitySampleAutoTestState_Shutdown : public FCitySampleAutoTestState
{
public:
	FCitySampleAutoTestState_Shutdown(UCitySampleTestControllerAutoTest* const TestController) : FCitySampleAutoTestState(TestController) {}

	virtual void Start(const ECitySampleAutoTestState PrevState) override
	{
		Super::Start(PrevState);
		Controller->EndCitySampleTest(TestExitCode);
	}

	void SetTestExitCode(const int32 ExitCode)
	{
		TestExitCode = ExitCode;
	}

private:
	int32 TestExitCode = 0;
};


//////////////////////////////////////////////////////////////////////
// Non state-specific logic
//////////////////////////////////////////////////////////////////////


FString UCitySampleTestControllerAutoTest::GetStateName(const ECitySampleAutoTestState State) const
{
	return UEnum::GetValueAsString(TEXT("/Script/CitySample.ECitySampleAutoTestState"), State);
}

FCitySampleAutoTestState& UCitySampleTestControllerAutoTest::GetTestState() const
{
	check(CurrentState != ECitySampleAutoTestState::MAX);
	FCitySampleAutoTestState* State = States[(uint8)CurrentState];
	check(State);
	return *State;
}

FCitySampleAutoTestState& UCitySampleTestControllerAutoTest::SetTestState(const ECitySampleAutoTestState NewState)
{
	const FString& EndingStateStr = GetStateName(CurrentState);
	const FString& StartingStateStr = GetStateName(NewState);

	if (CurrentState == NewState)
	{
		UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: reloading %s"), ANSI_TO_TCHAR(__func__), *EndingStateStr);
	}
	else
	{
		UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: %s -> %s"), ANSI_TO_TCHAR(__func__), *EndingStateStr, *StartingStateStr);
	}

	// End current test state
	GetTestState().End(NewState);

	// Transition current test state from the previous state to the new state
	const ECitySampleAutoTestState PrevState = CurrentState;
	CurrentState = NewState;
	FCitySampleAutoTestState& CurrentTestState = GetTestState();

	// Start new test state
	CurrentTestState.Start(PrevState);
	return CurrentTestState;
}

void UCitySampleTestControllerAutoTest::EndCitySampleTest(const int32 ExitCode /*= 0*/)
{
	if (CurrentState != ECitySampleAutoTestState::Shutdown)
	{
		UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: called outside Shutdown test state, something went wrong."), ANSI_TO_TCHAR(__func__));
		static_cast<FCitySampleAutoTestState_Shutdown&>(SetTestState(ECitySampleAutoTestState::Shutdown)).SetTestExitCode(ExitCode);
		return;
	}

	Super::EndCitySampleTest(ExitCode);
}

void UCitySampleTestControllerAutoTest::UnbindAllDelegates()
{
	if (UWorld* const World = GetWorld())
	{
		World->OnWorldBeginPlay.RemoveAll(this);
		World->GameStateSetEvent.RemoveAll(this);
	}

	if (ACitySampleGameState* const CitySampleGameState = GetCitySampleGameState())
	{
		CitySampleGameState->OnSandboxIntroStarted.RemoveAll(this);
		CitySampleGameState->OnSandboxIntroFinished.RemoveAll(this);
		CitySampleGameState->OnTestSequenceFinished.RemoveAll(this);
	}

	IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(SoakTimeSink);

	Super::UnbindAllDelegates();
}

void UCitySampleTestControllerAutoTest::OnInit()
{
	Super::OnInit();

	// Initialize State Data
	States[(uint8)ECitySampleAutoTestState::InitialLoad] = new FCitySampleAutoTestState_InitialLoad(this);
	States[(uint8)ECitySampleAutoTestState::Intro] = new FCitySampleAutoTestState_Intro(this);
	States[(uint8)ECitySampleAutoTestState::Soak] = new FCitySampleAutoTestState_Soak(this);
	States[(uint8)ECitySampleAutoTestState::TestSequence] = new FCitySampleAutoTestState_TestSequence(this);
	States[(uint8)ECitySampleAutoTestState::Finished] = new FCitySampleAutoTestState_Finished(this);
	States[(uint8)ECitySampleAutoTestState::Shutdown] = new FCitySampleAutoTestState_Shutdown(this);

	StateTimeouts[(uint8)ECitySampleAutoTestState::InitialLoad] = 60.f;
	StateTimeouts[(uint8)ECitySampleAutoTestState::Intro] = 45.f;
	StateTimeouts[(uint8)ECitySampleAutoTestState::Soak] =  CitySampleTest::CVarSoakTime->GetFloat();
	StateTimeouts[(uint8)ECitySampleAutoTestState::TestSequence] = 600.0f;
	StateTimeouts[(uint8)ECitySampleAutoTestState::Finished] = 300.f;
	StateTimeouts[(uint8)ECitySampleAutoTestState::Shutdown] = 30.f;

	for (int32 StateIdx = 0; StateIdx < (uint8)ECitySampleAutoTestState::MAX; ++StateIdx)
	{
		checkf(States[StateIdx] != nullptr, TEXT("Missing state object for state %s"), *GetStateName((ECitySampleAutoTestState)StateIdx));
	}

	const FConsoleCommandDelegate SoakTimeDelegate = FConsoleCommandDelegate::CreateUObject(this, &ThisClass::OnSoakTimeChanged);
	SoakTimeSink = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(SoakTimeDelegate);

	// Set and start the InitialLoad test state
	UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: setting test state to InitialLoad"), ANSI_TO_TCHAR(__func__));
	CurrentState = ECitySampleAutoTestState::InitialLoad;
	GetTestState().Start(ECitySampleAutoTestState::MAX);
}

void UCitySampleTestControllerAutoTest::OnPreMapChange()
{
	Super::OnPreMapChange();

	if (CurrentState != ECitySampleAutoTestState::InitialLoad)
	{
		UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: setting test state to InitialLoad"), ANSI_TO_TCHAR(__func__));
		SetTestState(ECitySampleAutoTestState::InitialLoad);
	}
}

void UCitySampleTestControllerAutoTest::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);

	GetTestState().Tick(TimeDelta);
	MarkHeartbeatActive();

	if (GetTestState().GetTestStateTime() > StateTimeouts[(uint8)CurrentState])
	{
		if (CurrentState == ECitySampleAutoTestState::Finished || CurrentState == ECitySampleAutoTestState::Shutdown)
		{
			// Treat timeouts during finalization/shutdown as warnings
			UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: '%s' test state timed out after %f seconds"), ANSI_TO_TCHAR(__func__), *GetStateName(CurrentState), StateTimeouts[(uint8)CurrentState]);
			EndCitySampleTest();
		}
		else
		{
			// Treat all other timeouts as errors
			UE_LOG(LogCitySampleTest, Error, TEXT("AutoTest %s: '%s' test state timed out after %f seconds"), ANSI_TO_TCHAR(__func__), *GetStateName(CurrentState), StateTimeouts[(uint8)CurrentState]);
			EndCitySampleTest(1);
		}
	}
}

void UCitySampleTestControllerAutoTest::BeginDestroy()
{
	UnbindAllDelegates();

	for (int32 StateIdx = 0; StateIdx < (uint8)ECitySampleAutoTestState::MAX; ++StateIdx)
	{
		delete States[StateIdx];
		States[StateIdx] = nullptr;
	}

	Super::BeginDestroy();
}

void UCitySampleTestControllerAutoTest::OnPreWorldInitialize(UWorld* const World)
{
	check(World);
	World->GameStateSetEvent.AddUObject(this, &ThisClass::OnGameStateSet);
	World->OnWorldBeginPlay.AddUObject(this, &ThisClass::OnWorldBeginPlay);
}

void UCitySampleTestControllerAutoTest::OnGameStateSet(AGameStateBase* const GameStateBase)
{
	if (UWorld* const World = GetWorld())
	{
		World->GameStateSetEvent.RemoveAll(this);
	}

	if (ACitySampleGameState* const CitySampleGameState = Cast<ACitySampleGameState>(GameStateBase))
	{
		CitySampleGameState->OnSandboxIntroStarted.AddDynamic(this, &ThisClass::OnSandboxIntroStarted);
		CitySampleGameState->OnSandboxIntroFinished.AddDynamic(this, &ThisClass::OnSandboxIntroFinished);
		CitySampleGameState->OnTestSequenceFinished.AddDynamic(this, &ThisClass::OnSandboxTestSequenceFinished);
	}
}

void UCitySampleTestControllerAutoTest::OnWorldBeginPlay()
{
	const UWorld* const World = GetWorld();
	const ACitySampleGameMode* const GameMode = World ? World->GetAuthGameMode<ACitySampleGameMode>() : nullptr;
	if (!GameMode || !GameMode->UseSandboxIntro())
	{
		SetTestState(ECitySampleAutoTestState::Soak);
	}

	if (RequestsMemReport())
	{
		ExecuteMemReport();
		SetMemReportTimer();
	}
}

void UCitySampleTestControllerAutoTest::OnSandboxIntroStarted()
{
	UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: sandbox intro started..."), ANSI_TO_TCHAR(__func__));
	SetTestState(ECitySampleAutoTestState::Intro);
}

void UCitySampleTestControllerAutoTest::OnSandboxIntroFinished()
{
	if (CurrentState == ECitySampleAutoTestState::Intro)
	{
		UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: sandbox intro finished..."), ANSI_TO_TCHAR(__func__));
	}
	else 
	{
		UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: sandbox intro finished outside the Intro test state..."), ANSI_TO_TCHAR(__func__));
	}

	SetTestState(ECitySampleAutoTestState::Soak);
}

void UCitySampleTestControllerAutoTest::OnSandboxTestSequenceFinished()
{
	if (CurrentState == ECitySampleAutoTestState::TestSequence)
	{
		UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest %s: sandbox test sequence finished..."), ANSI_TO_TCHAR(__func__));
	}
	else
	{
		UE_LOG(LogCitySampleTest, Warning, TEXT("AutoTest %s: sandbox test sequence finished during a non-Sandbox test state..."), ANSI_TO_TCHAR(__func__));
	}

	SetTestState(ECitySampleAutoTestState::Finished);
}

void UCitySampleTestControllerAutoTest::OnSoakTimeChanged()
{
	const float SoakTime = CitySampleTest::CVarSoakTime->GetFloat();
	StateTimeouts[(uint8)ECitySampleAutoTestState::Soak] = SoakTime > 0.0f ? SoakTime + 15.0f : TNumericLimits<float>::Max();

	if (CurrentState == ECitySampleAutoTestState::Soak)
	{
		UE_LOG(LogCitySampleTest, Display, TEXT("AutoTest Soak %s: soak time changed to %f seconds..."), ANSI_TO_TCHAR(__func__), CitySampleTest::CVarSoakTime->GetFloat());
	}
}
