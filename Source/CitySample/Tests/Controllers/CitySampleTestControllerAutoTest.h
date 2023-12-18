// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Tests/Controllers/CitySampleTestControllerBase.h"
#include "CitySampleTestControllerAutoTest.generated.h"

class ULocalPlayer;

UENUM()
enum class ECitySampleAutoTestState : uint8
{
	InitialLoad,
	Intro,
	Soak,
	TestSequence,
	Finished,
	Shutdown,

	MAX
};

class FCitySampleAutoTestState
{
public:
	using Super = FCitySampleAutoTestState;

	FCitySampleAutoTestState() = delete;
	FCitySampleAutoTestState(class UCitySampleTestControllerAutoTest* const TestController) : Controller(TestController) {}
	virtual ~FCitySampleAutoTestState() {}

	virtual void Start(const ECitySampleAutoTestState PrevState);
	virtual void End(const ECitySampleAutoTestState NewState) {}
	virtual void Tick(const float TimeDelta);

	double GetTestStateTime() const 
	{
		return TimeSinceStart;
	}

protected:
	class UCitySampleTestControllerAutoTest* Controller;

private:
	double TimeSinceStart = 0.0;
};

UCLASS()
class UCitySampleTestControllerAutoTest : public UCitySampleTestControllerBase
{
	GENERATED_BODY()

public:
	FString GetStateName(const ECitySampleAutoTestState State) const;
	FCitySampleAutoTestState& GetTestState() const;
	FCitySampleAutoTestState& SetTestState(const ECitySampleAutoTestState NewState);

	virtual void EndCitySampleTest(const int32 ExitCode=0) override;

protected:
	virtual void OnInit() override;
	virtual void OnPreMapChange() override;
	virtual void OnTick(float TimeDelta) override;
	virtual void BeginDestroy() override;

private:
	virtual void UnbindAllDelegates() override;

	FCitySampleAutoTestState* States[(uint8)ECitySampleAutoTestState::MAX];
	float StateTimeouts[(uint8)ECitySampleAutoTestState::MAX];
	ECitySampleAutoTestState CurrentState;

	virtual void OnPreWorldInitialize(UWorld* World) override;

	UFUNCTION()
	void OnWorldBeginPlay();

	UFUNCTION()
	void OnGameStateSet(AGameStateBase* const GameStateBase);

	UFUNCTION()
	void OnSandboxIntroStarted();

	UFUNCTION()
	void OnSandboxIntroFinished();

	UFUNCTION()
	void OnSandboxTestSequenceFinished();
	
	FConsoleVariableSinkHandle SoakTimeSink;

	UFUNCTION()
	void OnSoakTimeChanged();
};