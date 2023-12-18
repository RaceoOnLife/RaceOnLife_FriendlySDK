// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/GameState.h"
#include "CitySampleGameState.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSandboxIntroStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSandboxIntroFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTestSequenceStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTestSequenceFinished);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTriggerDaytime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTriggerNighttime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterPhotomode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitPhotomode);


UCLASS()
class CITYSAMPLE_API ACitySampleGameState : public AGameState
{
	GENERATED_BODY()

	//////////////////////////////////////////////////////////////////////////
	// Sandbox Intro

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSandboxIntroStarted OnSandboxIntroStarted;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSandboxIntroFinished OnSandboxIntroFinished;

	UFUNCTION(BlueprintCallable)
	bool StartSandboxIntro();

	UFUNCTION(BlueprintCallable)
	void StopSandboxIntro();

	UFUNCTION(BlueprintPure)
	bool IsSandboxIntroPlaying() const
	{
		return bSandboxIntroPlaying;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent)
	bool ReceiveStartSandboxIntro();

	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveStopSandboxIntro();
	
	virtual void HandleMatchHasStarted() override;

private:
	UPROPERTY(VisibleAnywhere, Transient)
	bool bSandboxIntroPlaying = false;

	//////////////////////////////////////////////////////////////////////////
	// Test Sequence

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTestSequenceStarted OnTestSequenceStarted;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTestSequenceFinished OnTestSequenceFinished;

	UFUNCTION(BlueprintCallable)
	bool StartTestSequence();

	UFUNCTION(BlueprintCallable)
	void StopTestSequence();

	UFUNCTION(BlueprintPure)
	bool IsTestSequencePlaying() const
	{
		return bTestSequencePlaying;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent)
	bool ReceiveStartTestSequence();

	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveStopTestSequence();

private:
	UPROPERTY(VisibleAnywhere, Transient)
	bool bTestSequencePlaying = false;

	//////////////////////////////////////////////////////////////////////////
	// GameState Events

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTriggerDaytime OnTriggerDaytime;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTriggerNighttime OnTriggerNighttime;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnEnterPhotomode OnEnterPhotomode;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnExitPhotomode OnExitPhotomode;
};