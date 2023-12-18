// Copyright Epic Games, Inc. All Rights Reserved.


#include "CitySampleInterpolators.h"

bool FCitySampleInterpolatorTests::RunSubstepTest_CDSpringVector()
{
	static FVector Goal(10.f, 0.f, 0.f);			// initial goal
	static FVector GoalVelocity(1.f, 0.f, 0.f);		// how fast goal is moving
	static const int NumUpdates = 10;				// num frames to simulate
	static int HitchStart = 3, HitchStop = 6;		// hitch start/stop frames
	static float dtarray[NumUpdates] = { 0.1f, 0.04f, 0.08f, 0.02f, 0.05f, 0.1f, 0.3f, 0.4f, 0.33f, 0.12f };

	FCritDampSpringInterpolatorVector Nonhitched(10.f);
	FCritDampSpringInterpolatorVector Hitched(10.f);

	// get them started at 0,0,0
	Nonhitched.Reset();
	Hitched.Reset();
	Nonhitched.EvalSubstepped(FVector::ZeroVector, 0.001f);
	Hitched.EvalSubstepped(FVector::ZeroVector, 0.001f);

	FVector NonHitchedPos = FVector::ZeroVector;
	FVector LastNonhitchedPos = FVector::ZeroVector;
	FVector HitchedPos = FVector::ZeroVector;
	FVector LastHitchedPos = FVector::ZeroVector;

	float hitch_dt_accum = 0.f;
	for (int i = 0; i < NumUpdates; ++i)
	{
		const float dt = dtarray[i];
		Goal += GoalVelocity * dt;

		if (i >= HitchStart && i < HitchStop)
		{
			UE_LOG(LogTemp, Log, TEXT("Update %i, Hitched was skipped"), i);
			hitch_dt_accum += dt;
		}
		else
		{
			float realdt = (i == HitchStop) ? dt + hitch_dt_accum : dt;
			LastHitchedPos = HitchedPos;
			HitchedPos = Hitched.EvalSubstepped(Goal, realdt);
			float HitchedDelta = (LastHitchedPos - HitchedPos).Size();
			UE_LOG(LogTemp, Log, TEXT("Update %i, dt = %f, Hitched delta    = %f, HitchedPos = %s"), i, realdt, HitchedDelta, *HitchedPos.ToString());
		}

		LastNonhitchedPos = NonHitchedPos;
		NonHitchedPos = Nonhitched.EvalSubstepped(Goal, dt);
		float NonHitchedDelta = (LastNonhitchedPos - NonHitchedPos).Size();
		UE_LOG(LogTemp, Log, TEXT("Update %i, dt = %f, Nonhitched delta = %f, NonHitchedPos = %s"), i, dt, NonHitchedDelta, *NonHitchedPos.ToString());
	}

	if ((HitchedPos - NonHitchedPos).IsNearlyZero())
	{
		UE_LOG(LogTemp, Log, TEXT("... TEST PASSED!"));
		return true;
	}

	UE_LOG(LogTemp, Log, TEXT("... TEST FAILED!"));
	return false;
}