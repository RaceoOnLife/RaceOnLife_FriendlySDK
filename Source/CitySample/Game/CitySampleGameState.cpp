// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleGameState.h"

#include "CitySampleGameMode.h"

bool ACitySampleGameState::StartSandboxIntro()
{
	bSandboxIntroPlaying = ReceiveStartSandboxIntro();

	if (bSandboxIntroPlaying)
	{
		OnSandboxIntroStarted.Broadcast();
	}

	return bSandboxIntroPlaying;
}

void ACitySampleGameState::StopSandboxIntro()
{
	if (bSandboxIntroPlaying)
	{
		ReceiveStopSandboxIntro();
		bSandboxIntroPlaying = false;
		OnSandboxIntroFinished.Broadcast();
	}
}

void ACitySampleGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	const UWorld* const World = GetWorld();	
	if (const ACitySampleGameMode* const GameMode = World ? World->GetAuthGameMode<ACitySampleGameMode>() : nullptr)
	{
		if (GameMode->UseSandboxIntro())
		{
			StartSandboxIntro();
		}
	}
}

bool ACitySampleGameState::StartTestSequence()
{
	bTestSequencePlaying = ReceiveStartTestSequence();

	if (bTestSequencePlaying)
	{
		OnTestSequenceStarted.Broadcast();
	}

	return bTestSequencePlaying;
}

void ACitySampleGameState::StopTestSequence()
{
	if (bTestSequencePlaying)
	{
		ReceiveStopTestSequence();
		bTestSequencePlaying = false;
		OnTestSequenceFinished.Broadcast();
	}
}