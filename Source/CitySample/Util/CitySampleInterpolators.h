// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CitySampleInterpolators.generated.h"

namespace CitySampleInterpolatorHelpers
{
	template<class T> T GetZeroForType() {}
	template<> inline float GetZeroForType() { return 0.f; };
	template<> inline FVector GetZeroForType() { return FVector::ZeroVector; };
	template<> inline FRotator GetZeroForType() { return FRotator::ZeroRotator; };
	template<> inline FQuat GetZeroForType() { return FQuat::Identity; };

	template<class T> T NormalizeIfRotator(T Input) { return Input; }
	template<> inline FRotator NormalizeIfRotator(FRotator Input) { return Input.GetNormalized(); };
}

/** 
 * Generic templated version of FMath::InterpTo functions.
 * These are called "infinite impulse response" filters.
 */
template<class T>
struct TGenericIIRInterpolator
{
public:
	TGenericIIRInterpolator() = default;
	
 	TGenericIIRInterpolator(float InInterpSpeed)
	 	: InterpSpeed(InInterpSpeed)
 	{};

	/** 
	 * Updates the interpolator for the given new goal value and time slice. 
	 * Does a full eval in a single timeslice. 
	 */
	T Eval(T NewGoalValue, float DeltaTime)
	{
		if (bPendingReset)
		{
			PerformReset(NewGoalValue);
		}
		else
		{
			CurrentValue = SingleStepEval(NewGoalValue, DeltaTime);
			LastUpdateLeftoverTime = 0.f;
		}

		return CurrentValue;
	}

	/** Does sub-stepping, with partial-interval rewinding */
	T EvalSubstepped(T NewGoalValue, float DeltaTime)
	{
		if (bPendingReset)
		{
			PerformReset(NewGoalValue);
		}
		else
		{
			float RemainingTime = DeltaTime;

			// handle leftover rewind
			if (bDoLeftoverRewind && (LastUpdateLeftoverTime > 0.f))
			{
				// rewind back to state at end of last full MaxSubstepTime update
				RemainingTime += LastUpdateLeftoverTime;
				CurrentValue = ValueAfterLastFullStep;

				//UE_LOG(LogTemp, Log, TEXT("Rewound %f to pos %s"), LastUpdateLeftoverTime, *CurrentPos.ToString());
				LastUpdateLeftoverTime = 0.f;
			}

			// move the goal linearly toward goal while we substep
			const T EquilibriumStepRate = CitySampleInterpolatorHelpers::NormalizeIfRotator<T>(NewGoalValue - LastGoalValue) * (1.f / RemainingTime);
			T LerpedGoalValue = LastGoalValue;

			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float StepTime = FMath::Min(MaxSubstepTime, RemainingTime);

				if (bDoLeftoverRewind && (StepTime < MaxSubstepTime))
				{
					// last partial step, cache where we were after last full step
					// so we can resume from there on the next eval
					LastUpdateLeftoverTime = StepTime;
					ValueAfterLastFullStep = CurrentValue;
				}

				LerpedGoalValue += EquilibriumStepRate * StepTime;
				RemainingTime -= StepTime;

				//UE_LOG(LogTemp, Log, TEXT("...   internal eval starting at %s, towards %s, steptime = %f"), *CurrentPos.ToString(), *LerpedEquilibriumPos.ToString(), StepTime);
				CurrentValue = SingleStepEval(LerpedGoalValue, StepTime);
				//UE_LOG(LogTemp, Log, TEXT("...      resultant pos = %s"), *CurrentPos.ToString());

				LastGoalValue = NewGoalValue;
			}
		}

		return CurrentValue;
	}

	/** Update the interpolation speed */
	void SetInterpSpeed(float NewInterpSpeed) 
	{ 
		InterpSpeed = NewInterpSpeed; 
	};

	/** 
	 * Sets the starting CurrentValue for the interpolation. Note this will cancel any pending resets
	 * since a reset will render this ineffective.
	 */
	void SetInitialValue(T InitialValue)
	{
		CurrentValue = InitialValue;
		bPendingReset = false;
	}

	/** Returns the current value of the interpolator. */
	T GetCurrentValue() const 
	{ 
		return CurrentValue; 
	};

	/** Interpolator value will snap to the goal value on the next Eval() */
	void Reset() 
	{ 
		bPendingReset = true; 
	}

protected:
	/** Maximum timeslice per substep. */
	static constexpr float MaxSubstepTime = 1.f / 120.f;

	/** Ideal/destination value for this interpolator. */
	T GoalValue;

	/** Current value for this interpolator. */
	T CurrentValue;

 	/** Controls how fast to approach the goal value */
 	float InterpSpeed = 6.f;

	/** If true, snap current value to the goal value on the next eval. */
	bool bPendingReset = true;

	bool bDoLeftoverRewind = true;
	float LastUpdateLeftoverTime = 0.f;
	T ValueAfterLastFullStep;
	T LastGoalValue;

protected:
	T SingleStepEval(T NewGoalValue, float StepTime)
	{
		// need to specialize all types we use!
		check(false);
		return T();
	}

	void PerformReset(T NewGoalValue)
	{
		CurrentValue = NewGoalValue;
		LastGoalValue = NewGoalValue;
		LastUpdateLeftoverTime = 0.f;		// clear out any leftovers for rewind
		bPendingReset = false;
	}
};

template<>
inline float TGenericIIRInterpolator<float>::SingleStepEval(float StepGoalValue, float StepTime)
{
	return FMath::FInterpTo(CurrentValue, StepGoalValue, StepTime, InterpSpeed);
}

template<> 
inline FVector TGenericIIRInterpolator<FVector>::SingleStepEval(FVector StepGoalValue, float StepTime)
{
	return FMath::VInterpTo(CurrentValue, StepGoalValue, StepTime, InterpSpeed);
}

template<>
inline FRotator TGenericIIRInterpolator<FRotator>::SingleStepEval(FRotator StepGoalValue, float StepTime)
{
	return FMath::RInterpTo(CurrentValue, StepGoalValue, StepTime, InterpSpeed);
}

template<>
inline FLinearColor TGenericIIRInterpolator<FLinearColor>::SingleStepEval(FLinearColor StepGoalValue, float StepTime)
{
	return FMath::CInterpTo(CurrentValue, StepGoalValue, StepTime, InterpSpeed);
}

template<>
inline FQuat TGenericIIRInterpolator<FQuat>::SingleStepEval(FQuat StepGoalValue, float StepTime)
{
	return FMath::QInterpTo(CurrentValue, StepGoalValue, StepTime, InterpSpeed);
}



/**
 * This is a double version of the IIR filters above.
 * It's basically two interpolators. One intermediate value to interpolate towards the goal, 
 * and the final value interpolating towars the intermediate value.
 * The end result is a softer departure, while keeping a smooth arrival.
 */
template<class T>
struct TGenericDoubleIIRInterpolator
{
public:
	TGenericDoubleIIRInterpolator()
	{
		SetInterpSpeeds(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	TGenericDoubleIIRInterpolator(float InPrimaryInterpSpeed, float InIntermediateInterpSpeed)
	{
		SetInterpSpeeds(InPrimaryInterpSpeed, InIntermediateInterpSpeed);
	}

	void SetInterpSpeeds(float NewPrimaryInterpSpeed, float NewIntermediateInterpSpeed)
	{
		PrimaryInterpSpeed = NewPrimaryInterpSpeed;
		PrimaryInterpolator.SetInterpSpeed(PrimaryInterpSpeed);
		IntermediateInterpSpeed = NewIntermediateInterpSpeed;
		IntermediateInterpolator.SetInterpSpeed(IntermediateInterpSpeed);
	}

	void SetInitialValue(T InitialValue)
	{
		IntermediateInterpolator.SetInitialValue(InitialValue);
		PrimaryInterpolator.SetInitialValue(InitialValue);
	}

	/**
	 * Updates the interpolator for the given new goal value and time slice.
	 * Does a full eval in a single timeslice.
	 */
	T Eval(T NewGoalValue, float DeltaTime)
	{
		// underlying interpolators will handle resets
		return SingleStepEval(NewGoalValue, DeltaTime);
	}

	/** Does sub-stepping, with partial-interval rewinding */
	T EvalSubstepped(T NewGoalValue, float DeltaTime)
	{
		// underlying interpolators will handle resets
		{
			float RemainingTime = DeltaTime;

			// move the goal linearly toward goal while we substep
			const T EquilibriumStepRate = CitySampleInterpolatorHelpers::NormalizeIfRotator<T>(NewGoalValue - LastGoalValue) * (1.f / RemainingTime);
			T LerpedGoalValue = LastGoalValue;

			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float StepTime = FMath::Min(MaxSubstepTime, RemainingTime);

				LerpedGoalValue += EquilibriumStepRate * StepTime;
				RemainingTime -= StepTime;

				//UE_LOG(LogTemp, Log, TEXT("...   internal eval starting at %s, towards %s, steptime = %f"), *CurrentPos.ToString(), *LerpedEquilibriumPos.ToString(), StepTime);
				SingleStepEval(LerpedGoalValue, StepTime);
				//UE_LOG(LogTemp, Log, TEXT("...      resultant pos = %s"), *CurrentPos.ToString());

				LastGoalValue = NewGoalValue;
			}
		}

		return PrimaryInterpolator.GetCurrentValue();
	}

	void Reset()
	{
		IntermediateInterpolator.Reset();
		PrimaryInterpolator.Reset();
	}

	T GetCurrentValue() const
	{
		return PrimaryInterpolator.GetCurrentValue();
	};

protected:

	T SingleStepEval(T StepGoalValue, float StepTime)
	{
		// make sure step time of the double is same as step time of the underlying singles.
		// that ensures the partial step rewind works and isn't running too often
		T IntermedValue = IntermediateInterpolator.EvalSubstepped(StepGoalValue, StepTime);
		return PrimaryInterpolator.EvalSubstepped(IntermedValue, StepTime);
	}

protected:
	// do we need to store this? it's also in the interpolators
	float PrimaryInterpSpeed = 4.f;
	float IntermediateInterpSpeed = 12.f;

	T LastGoalValue;

	/** Maximum timeslice per substep. */
	static constexpr float MaxSubstepTime = 1.f / 120.f;

	TGenericIIRInterpolator<T> IntermediateInterpolator;
	TGenericIIRInterpolator<T> PrimaryInterpolator;
};

/** 
 * Blueprint-accessible wrappers for the templated interpolators, for use as FProperties
 */
USTRUCT(BlueprintType)
struct FIIRInterpolatorVector
{
	GENERATED_BODY();

public:
	FIIRInterpolatorVector()
	{
		Interpolator = TGenericIIRInterpolator<FVector>(InterpSpeed);
	}

	FIIRInterpolatorVector(float InInterpSpeed)
		: InterpSpeed(InInterpSpeed)
		, Interpolator(InInterpSpeed)
	{}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float InterpSpeed = 6.f;

	FVector Eval(FVector GoalLocation, float DeltaTime)
	{
		// set every time so we can tweak values while game is live
		Interpolator.SetInterpSpeed(InterpSpeed);
		return Interpolator.Eval(GoalLocation, DeltaTime);
	}

	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(FVector InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	FVector GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TGenericIIRInterpolator<FVector> Interpolator;
};


USTRUCT(BlueprintType)
struct FDoubleIIRInterpolatorVector
{
	GENERATED_BODY();

public:
	FDoubleIIRInterpolatorVector()
	{
		Interpolator = TGenericDoubleIIRInterpolator<FVector>(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	FDoubleIIRInterpolatorVector(float InPrimaryInterpSpeed, float InIntermediateInterpSpeed)
		: PrimaryInterpSpeed(InPrimaryInterpSpeed)
		, IntermediateInterpSpeed(InIntermediateInterpSpeed)
	{
		Interpolator = TGenericDoubleIIRInterpolator<FVector>(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PrimaryInterpSpeed = 4.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float IntermediateInterpSpeed = 12.f;

	FVector Eval(FVector GoalLocation, float DeltaTime)
	{
		// set every time so we can tweak values while game is live
		Interpolator.SetInterpSpeeds(PrimaryInterpSpeed, IntermediateInterpSpeed);
		return Interpolator.Eval(GoalLocation, DeltaTime);
	}

	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(FVector InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	FVector GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TGenericDoubleIIRInterpolator<FVector> Interpolator;
};


USTRUCT(BlueprintType)
struct FIIRInterpolatorRotator
{
	GENERATED_BODY();

public:
	FIIRInterpolatorRotator()
	{
		Interpolator = TGenericIIRInterpolator<FRotator>(InterpSpeed);
	}

	FIIRInterpolatorRotator(float InInterpSpeed)
		: InterpSpeed(InInterpSpeed)
	{
		Interpolator = TGenericIIRInterpolator<FRotator>(InterpSpeed);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float InterpSpeed = 6.f;

	FRotator Eval(FRotator GoalRotation, float DeltaTime)
	{
		// set every time so we can tweak values while game is live
		Interpolator.SetInterpSpeed(InterpSpeed);
		return Interpolator.Eval(GoalRotation, DeltaTime);
	}

	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(FRotator InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	FRotator GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TGenericIIRInterpolator<FRotator> Interpolator;
};


USTRUCT(BlueprintType)
struct FDoubleIIRInterpolatorRotator
{
	GENERATED_BODY();

public:
	FDoubleIIRInterpolatorRotator()
	{
		Interpolator = TGenericDoubleIIRInterpolator<FRotator>(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	FDoubleIIRInterpolatorRotator(float InPrimaryInterpSpeed, float InIntermediateInterpSpeed)
		: PrimaryInterpSpeed(InPrimaryInterpSpeed)
		, IntermediateInterpSpeed(InIntermediateInterpSpeed)
	{
		Interpolator = TGenericDoubleIIRInterpolator<FRotator>(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PrimaryInterpSpeed = 4.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float IntermediateInterpSpeed = 12.f;

	FRotator Eval(FRotator GoalRotation, float DeltaTime)
	{
		// set every time so we can tweak values while game is live
		Interpolator.SetInterpSpeeds(PrimaryInterpSpeed, IntermediateInterpSpeed);
		return Interpolator.Eval(GoalRotation, DeltaTime);
	}

	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(FRotator InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	FRotator GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TGenericDoubleIIRInterpolator<FRotator> Interpolator;
};


USTRUCT(BlueprintType)
struct FIIRInterpolatorFloat
{
	GENERATED_BODY();

public:
	FIIRInterpolatorFloat()
	{
		Interpolator = TGenericIIRInterpolator<float>(InterpSpeed);
	}

	FIIRInterpolatorFloat(float InInterpSpeed)
		: InterpSpeed(InInterpSpeed)
	{
		Interpolator = TGenericIIRInterpolator<float>(InterpSpeed);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float InterpSpeed = 6.f;

	float Eval(float Goal, float DeltaTime)
	{
		// set every time so we can tweak values while game is live
		Interpolator.SetInterpSpeed(InterpSpeed);
		return Interpolator.Eval(Goal, DeltaTime);
	}

	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(float InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	float GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TGenericIIRInterpolator<float> Interpolator;
};


USTRUCT(BlueprintType)
struct FDoubleIIRInterpolatorFloat
{
	GENERATED_BODY();

public:
	FDoubleIIRInterpolatorFloat()
	{
		Interpolator = TGenericDoubleIIRInterpolator<float>(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	FDoubleIIRInterpolatorFloat(float InPrimaryInterpSpeed, float InIntermediateInterpSpeed)
		: PrimaryInterpSpeed(InPrimaryInterpSpeed)
		, IntermediateInterpSpeed(InIntermediateInterpSpeed)
	{
		Interpolator = TGenericDoubleIIRInterpolator<float>(PrimaryInterpSpeed, IntermediateInterpSpeed);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PrimaryInterpSpeed = 4.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float IntermediateInterpSpeed = 12.f;

	float Eval(float Goal, float DeltaTime)
	{
		// set every time so we can tweak values while game is live
		Interpolator.SetInterpSpeeds(PrimaryInterpSpeed, IntermediateInterpSpeed);
		return Interpolator.Eval(Goal, DeltaTime);
	}

	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(float InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	float GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TGenericDoubleIIRInterpolator<float> Interpolator;
};

// #todo
// for completeness:
// double versions of vector and float
// both versions of color and quat
// apply imrproved substepping algo and leftover rewind to accel interpolators

template<class T>
struct TAccelerationInterpolator
{
	TAccelerationInterpolator() = default;

	TAccelerationInterpolator(float InMaxAcceleration, float InMinDeceleration, float InMaxSpeed)
		: MaxAcceleration(InMinDeceleration)
		, MinDeceleration(InMinDeceleration)
		, MaxSpeed(InMaxSpeed)
	{};


	/** Updates the interpolator for the given new goal value and time slice. */
	T Eval(T NewGoalValue, float DeltaTime)
	{
		GoalValue = NewGoalValue;

		if (bPendingReset)
		{
			CurrentValue = NewGoalValue;
			CurrentSpeed = 0.f;
			bPendingReset = false;
		}
		else
		{
			SubstepTick(DeltaTime);
		}

		return CurrentValue;
	}

	/** 
	 * Sets the starting CurrentValue for the interpolation. Note this will cancel any pending resets
	 * since a reset will render this ineffective.
	 */
	void SetInitialValue(T InitialValue)
	{
		CurrentValue = InitialValue;
		CurrentSpeed = 0.f;
		bPendingReset = false;
	}

	/** Returns the current value of the interpolator. */
	T GetCurrentValue() const 
	{ 
		return CurrentValue; 
	};

	/** Interpolator value will snap to the goal value on the next Eval() */
	void Reset() 
	{ 
		bPendingReset = true; 
	}

	bool IsWithinHoldTolerance() const
	{
		// need to specialize all types we use!
		check(false);
		return false;
	}

	// configuration data
	float MaxAcceleration = 0.f;
	float MinDeceleration = 0.f;
	float MaxSpeed = 0.f;
	/** Don't accelerate if within this distance of the goal. */
	float HoldTolerance = 1.f;

private:

	void SubstepTick(float DeltaTime)
	{
		float SimTimeRemaining = DeltaTime;
		while (SimTimeRemaining > 0.f)
		{
			const float StepTime = FMath::Min(SimTimeRemaining, MaxSubstepTime);
			T NewVal = SingleStepEval(StepTime);
			CurrentValue = NewVal;

			SimTimeRemaining -= StepTime;
		}
	}

	T SingleStepEval(float StepTime)
	{
		// need to specialize all types we use!
		check(false);
		return T();
	}

	void UpdateSpeed(float DistanceToGoal, float StepTime)
	{
		// are we close enough to decel?
		// v^2 = v0^2 + 2a * dx

		const float CurSpeedSq = FMath::Square(CurrentSpeed);

		const float IdealStoppingDist = CurSpeedSq / (2.f * MinDeceleration);

		float NewSpeed = 0.f;
		if ((DistanceToGoal < IdealStoppingDist) || (DistanceToGoal < HoldTolerance))
		{
			// we should be decelerating
			// compute real deceleration needed to hit our mark
			float AccelMag = CurSpeedSq / (2.f * FMath::Abs(DistanceToGoal));
			float DeltaSpeed = AccelMag * StepTime;

			// clamp to make sure don't let us go backwards
			NewSpeed = CurrentSpeed - DeltaSpeed;
		}
		else if (DistanceToGoal > HoldTolerance)
		{
			// we should be accelerating toward the goal
			float AccelMag = MaxAcceleration;
			float DeltaSpeed = AccelMag * StepTime;

			NewSpeed = CurrentSpeed + DeltaSpeed;
		}

		// clamp to enforce max speed
		const float MaxSpeedToHitGoal = DistanceToGoal / StepTime;
		CurrentSpeed = FMath::Clamp(NewSpeed, 0.f, FMath::Min(MaxSpeed, MaxSpeedToHitGoal));
	}

	static constexpr float MaxSubstepTime = 1.f / 120.f;

	// state data

	/** a magnitude, always positive */
	float CurrentSpeed = 0.f;

	/** If true, snap current value to the goal value on the next eval. */
	bool bPendingReset = true;

	/** Ideal/destination value for this interpolator. */
	T GoalValue;

	/** Current value for this interpolator. */
	T CurrentValue;

};

template<>
inline float TAccelerationInterpolator<float>::SingleStepEval(float StepTime)
{
	const float CurDistToGoal = FMath::Abs(GoalValue - CurrentValue);;

	UpdateSpeed(CurDistToGoal, StepTime);

	// integrate
	const float DirToGoal = CurrentValue > GoalValue ? -1.f : 1.f;
	const float CurrentVelocity = CurrentSpeed * DirToGoal;
	CurrentValue += CurrentVelocity * StepTime;

	return CurrentValue;
}

template<>
inline FVector TAccelerationInterpolator<FVector>::SingleStepEval(float StepTime)
{
	const float CurDistToGoal = (GoalValue - CurrentValue).Size();

	UpdateSpeed(CurDistToGoal, StepTime);

	// integrate
	const FVector DirToGoal = (GoalValue - CurrentValue).GetSafeNormal();
	const FVector CurrentVelocity = CurrentSpeed * DirToGoal;
	CurrentValue += CurrentVelocity * StepTime;

	return CurrentValue;
}

template<>
inline FRotator TAccelerationInterpolator<FRotator>::SingleStepEval(float StepTime)
{
	const FRotator DeltaToGoal = (GoalValue - CurrentValue).GetNormalized();

	FVector DeltaToGoalInVectorForm(DeltaToGoal.Roll, DeltaToGoal.Pitch, DeltaToGoal.Yaw);
	const float CurDistToGoal = DeltaToGoalInVectorForm.Size();
	UpdateSpeed(CurDistToGoal, StepTime);

	// integrate
	const FVector DirToGoal = DeltaToGoalInVectorForm.GetSafeNormal();
 	const FVector CurrentVelocity = CurrentSpeed * DirToGoal;

	FVector DeltaInVectorForm = CurrentVelocity * StepTime;

	CurrentValue.Roll += DeltaInVectorForm.X;
	CurrentValue.Pitch += DeltaInVectorForm.Y;
	CurrentValue.Yaw += DeltaInVectorForm.Z;

	CurrentValue = CurrentValue.GetNormalized();

	return CurrentValue;
}

template<>
inline bool TAccelerationInterpolator<float>::IsWithinHoldTolerance() const
{
	const float CurDistToGoal = FMath::Abs(GoalValue - CurrentValue);;
	return CurDistToGoal < HoldTolerance;
}

template<>
inline bool TAccelerationInterpolator<FVector>::IsWithinHoldTolerance() const
{
	const float CurDistToGoal = (GoalValue - CurrentValue).Size();
	return CurDistToGoal < HoldTolerance;
}

template<>
inline bool TAccelerationInterpolator<FRotator>::IsWithinHoldTolerance() const
{
	const FRotator DeltaToGoal = (GoalValue - CurrentValue).GetNormalized();
	FVector DeltaToGoalInVectorForm(DeltaToGoal.Roll, DeltaToGoal.Pitch, DeltaToGoal.Yaw);
	const float CurDistToGoal = DeltaToGoalInVectorForm.Size();
	return CurDistToGoal < HoldTolerance;
}

USTRUCT(BlueprintType)
struct FAccelerationInterpolatorParams
{
	GENERATED_BODY();

public:
	FAccelerationInterpolatorParams() = default;

	FAccelerationInterpolatorParams(float InAcceleration, float InDeceleration, float InMaxSpeed)
		: Acceleration(InAcceleration)
		, MinDeceleration(InDeceleration)
		, MaxSpeed(InMaxSpeed)
	{}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Acceleration = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinDeceleration = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSpeed = 500.f;
};

USTRUCT(BlueprintType)
struct FAccelerationInterpolatorFloat
{
	GENERATED_BODY();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AccelerationInterpolator")
	FAccelerationInterpolatorParams AccelerationParams = FAccelerationInterpolatorParams(500.f, 500.f, 2000.f);

	float Eval(float NewGoalValue, float DeltaTime)
	{
		Interpolator.MaxAcceleration = AccelerationParams.Acceleration;
		Interpolator.MinDeceleration = AccelerationParams.MinDeceleration;
		Interpolator.MaxSpeed = AccelerationParams.MaxSpeed;
		return Interpolator.Eval(NewGoalValue, DeltaTime);
	}

	void SetAccelerationParams(const FAccelerationInterpolatorParams& NewParams) { AccelerationParams = NewParams; };
	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(float InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	float GetCurrentValue() const { return Interpolator.GetCurrentValue(); }
	void SetTolerance(float NewTolerance) { Interpolator.HoldTolerance = NewTolerance; }

private:
	TAccelerationInterpolator<float> Interpolator;
};


USTRUCT(BlueprintType)
struct FAccelerationInterpolatorVector
{
	GENERATED_BODY();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AccelerationInterpolator")
	FAccelerationInterpolatorParams AccelerationParams = FAccelerationInterpolatorParams(500.f, 500.f, 2000.f);
	
	FVector Eval(FVector NewGoalValue, float DeltaTime)
	{
		Interpolator.MaxAcceleration = AccelerationParams.Acceleration;
		Interpolator.MinDeceleration = AccelerationParams.MinDeceleration;
		Interpolator.MaxSpeed = AccelerationParams.MaxSpeed;
		return Interpolator.Eval(NewGoalValue, DeltaTime);
	}

	void SetAccelerationParams(const FAccelerationInterpolatorParams& NewParams) { AccelerationParams = NewParams; };
	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(FVector InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	FVector GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

private:
	TAccelerationInterpolator<FVector> Interpolator;
};


USTRUCT(BlueprintType)
struct FAccelerationInterpolatorRotator
{
	GENERATED_BODY();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AccelerationInterpolator")
	FAccelerationInterpolatorParams AccelerationParams = FAccelerationInterpolatorParams(60.f, 60.f, 360.f);
	
	FRotator Eval(FRotator NewGoalValue, float DeltaTime)
	{
		Interpolator.MaxAcceleration = AccelerationParams.Acceleration;
		Interpolator.MinDeceleration = AccelerationParams.MinDeceleration;
		Interpolator.MaxSpeed = AccelerationParams.MaxSpeed;
		return Interpolator.Eval(NewGoalValue, DeltaTime);
	}

	void SetAccelerationParams(const FAccelerationInterpolatorParams& NewParams) { AccelerationParams = NewParams; };
	void Reset() { Interpolator.Reset(); }
	void SetInitialValue(FRotator InitialValue) { Interpolator.SetInitialValue(InitialValue); }
	FRotator GetCurrentValue() const { return Interpolator.GetCurrentValue(); }

	bool IsWithinHoldTolerance() const { return Interpolator.IsWithinHoldTolerance(); }

private:
	TAccelerationInterpolator<FRotator> Interpolator;
};



// equations to compute new pos and velocity after elapsed time dt, given initial conditions
// x(t) = ( (v0 + x0*w) * dt + x0) * e^(-w*dt)
// v(t) = ( v0 - (v0 + x0*w) * w * dt) * e^(-w*dt)
// where...
// v0 is vel at start of frame
// x0 is displacement from rest position at start of frame
// w is the "angular frequency", which is sqrt(k/m), where k is spring constant and m is the mass at the end of the spring
// dt is elapsed time

/**
 * An interpolator using a critically dampened mass-spring system.
 */
template<class T>
struct TCritDampSpringInterpolator
{
public:
	TCritDampSpringInterpolator() = default;

	TCritDampSpringInterpolator(float InNaturalFrequency)
		: NaturalFrequency(InNaturalFrequency)
	{};

	TCritDampSpringInterpolator(float SpringConstant, float Mass)
		: NaturalFrequency(FMath::Sqrt(SpringConstant / Mass))
	{};

	/** Bundle of the key coefficients for updating the spring. */
	struct FCDSpringScalars
	{
		float E = 0.f;
		float ExDT = 0.f;
		float ExDTxW = 0.f;
	};

	/** For pre-computing and caching this spring's scalars, as most updates will happen with the same timestep */
	struct FCDSpringCachedScalars
	{
		float CachedNatFreq = 0.f;
		float CachedTimeStep = 0.f;
		FCDSpringScalars Scalars;

		bool AreCached(float NatFreq, float TimeStep) const
		{
			return ((CachedNatFreq == NatFreq) && (CachedTimeStep == TimeStep));
		}

		bool GetCachedScalars(float NatFreq, float TimeStep, FCDSpringScalars& OutScalars) const
		{
			if (AreCached(NatFreq, TimeStep))
			{
				OutScalars = Scalars;
				return true;
			}

			return false;
		}

		void Set(float NatFreq, float TimeStep, FCDSpringScalars NewScalars)
		{
			CachedNatFreq = NatFreq;
			CachedTimeStep = TimeStep;
			Scalars = NewScalars;
		}
	};

	void CacheScalars(float InNatFreq, float InTimeStep)
	{
		if (CachedScalars.AreCached(InNatFreq, InTimeStep) == false)
		{
			const FCDSpringScalars Scalars = ComputeScalars(InNatFreq, InTimeStep);
			CachedScalars.Set(InNatFreq, InTimeStep, Scalars);
		}
	}

	static FCDSpringScalars ComputeScalars(float InAngFreq, float InTimeStep)
	{
		FCDSpringScalars Out;
		Out.E = FMath::Pow(EULERS_NUMBER, (-InAngFreq * InTimeStep));
		Out.ExDT = Out.E * InTimeStep;
		Out.ExDTxW = Out.ExDT * InAngFreq;
		return Out;
	}

	FCDSpringScalars GetScalars(float InAngFreq, float InTimeStep) const
	{
		FCDSpringScalars Out;
		if (CachedScalars.GetCachedScalars(InAngFreq, InTimeStep, Out))
		{
			return Out;
		}

		return ComputeScalars(InAngFreq, InTimeStep);
	}

// 	T ZeroForType() const
// 	{
// 		check(false);
// 	}

	/** Does substepping, with partial-interval rewinding */
	T EvalSubstepped(T NewEquilibriumPos, float DeltaTime)
	{
		if (bPendingReset)
		{
			CurrentPos = NewEquilibriumPos;
			CurrentVelocity = CitySampleInterpolatorHelpers::GetZeroForType<T>();
			LastEquilibrium = NewEquilibriumPos;

			LastUpdateLeftoverTime = 0.f;		// clear out any leftovers for rewind
			bPendingReset = false;
		}
		else
		{
			float RemainingTime = DeltaTime;

			// handle leftover rewind
			if (bDoLeftoverRewind && (LastUpdateLeftoverTime > 0.f))
			{
				// rewind back to state at end of last full MaxSubstepTime update
				RemainingTime += LastUpdateLeftoverTime;
				CurrentPos = PosAfterLastFullStep;
				CurrentVelocity = VelAfterLastFullStep;

				//UE_LOG(LogTemp, Log, TEXT("Rewound %f to pos %s"), LastUpdateLeftoverTime, *CurrentPos.ToString());
				LastUpdateLeftoverTime = 0.f;
			}

			// move the goal linearly toward goal while we substep
			const T EquilibriumStepRate = (NewEquilibriumPos - LastEquilibrium) * (1.f / RemainingTime);
			T LerpedEquilibriumPos = LastEquilibrium;

			// #todo: it would be best to do this at init time instead of on every eval, but
			// NaturalFrequency could theoretically be changed at runtime
			CacheScalars(NaturalFrequency, MaxSubstepTime);

			while (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float StepTime = FMath::Min(MaxSubstepTime, RemainingTime);

				if (bDoLeftoverRewind && (StepTime < MaxSubstepTime))
				{
					// last partial step, cache where we were after last full step
					// so we can resume from there on the next eval
					LastUpdateLeftoverTime = StepTime;
					PosAfterLastFullStep = CurrentPos;
					VelAfterLastFullStep = CurrentVelocity;
				}

				LerpedEquilibriumPos += EquilibriumStepRate * StepTime;
				RemainingTime -= StepTime;

				//UE_LOG(LogTemp, Log, TEXT("...   internal eval starting at %s, towards %s, steptime = %f"), *CurrentPos.ToString(), *LerpedEquilibriumPos.ToString(), StepTime);
				CurrentPos = SingleStepEval(LerpedEquilibriumPos, StepTime);
				//UE_LOG(LogTemp, Log, TEXT("...      resultant pos = %s"), *CurrentPos.ToString());

				LastEquilibrium = NewEquilibriumPos;
			}
		}

		return CurrentPos;
	}

	/** Does a full non-substepped eval */
	T Eval(T NewEquilibriumPos, float DeltaTime)
	{
		if (bPendingReset)
		{
			PerformReset(NewEquilibriumPos);
		}
		else
		{
			SingleStepEval(NewEquilibriumPos, DeltaTime);
			LastUpdateLeftoverTime = 0.f;
		}

		return CurrentPos;
	}

	T GetCurrentValue() const
	{
		return CurrentPos;
	}

	void Init(T NewEquilibriumValue)
	{
		CurrentPos = NewEquilibriumValue;
		CurrentVelocity = CitySampleInterpolatorHelpers::GetZeroForType<T>();
		bPendingReset = false;
	}

	/** Will snap directly to the equilibrium value on the next Eval() call. */
	void Reset()
	{
		bPendingReset = true;
	}

protected:
	/** Single-step eval. */
	T SingleStepEval(T NewEquilibriumPos, float DeltaTime)
	{
		// rearranged from above...
		// x(t) = v0 * (E * dt) + x0 * ((w * dt * E) + E)
		// v(t) = v0 (E - w*dt*E) + x0 * (-w^2 * dt * E)
		// where E is the e^(-w*dt) term

		const FCDSpringScalars Scalars = GetScalars(NaturalFrequency, DeltaTime);
		const T CurrentDisplacement = CitySampleInterpolatorHelpers::NormalizeIfRotator<T>(CurrentPos - NewEquilibriumPos);
		const T NewDisplacement = CurrentDisplacement * (Scalars.ExDTxW + Scalars.E) + CurrentVelocity * Scalars.ExDT;
		const T NewVel = NewDisplacement * (-Scalars.ExDTxW * NaturalFrequency) + CurrentVelocity * (Scalars.E - Scalars.ExDTxW);
		const T NewPos = CitySampleInterpolatorHelpers::NormalizeIfRotator<T>(NewDisplacement + NewEquilibriumPos);

		CurrentPos = NewPos;
		CurrentVelocity = NewVel;

		return CurrentPos;
	}

	void PerformReset(T NewEquilibriumPos)
	{
		CurrentPos = NewEquilibriumPos;
		CurrentVelocity = CitySampleInterpolatorHelpers::GetZeroForType<T>();
		LastEquilibrium = NewEquilibriumPos;

		LastUpdateLeftoverTime = 0.f;		// clear out any leftovers for rewind
		bPendingReset = false;
	}

public:
	T CurrentPos;
	T CurrentVelocity;

	float NaturalFrequency = 20.f;
	static constexpr float MaxSubstepTime = 1.f / 120.f;

	bool bPendingReset = false;
	bool bDoLeftoverRewind = true;
	float LastUpdateLeftoverTime = 0.f;
	T PosAfterLastFullStep;
	T VelAfterLastFullStep;
	T LastEquilibrium;

	FCDSpringCachedScalars CachedScalars;
};

// template<> inline float TCritDampSpringInterpolator<float>::ZeroForType() const { return 0.f; }
// template<> inline FVector TCritDampSpringInterpolator<FVector>::ZeroForType() const { return FVector::ZeroVector; }
// template<> inline FRotator TCritDampSpringInterpolator<FRotator>::ZeroForType() const { return FRotator::ZeroRotator; }
// template<> inline FQuat TCritDampSpringInterpolator<FQuat>::ZeroForType() const { return FQuat::Identity; }

/** UStruct wrapper for critically damped spring vector interpolator */
USTRUCT(BlueprintType)
struct FCritDampSpringInterpolatorVector
{
	GENERATED_BODY();

public:
	FCritDampSpringInterpolatorVector()
		: Interpolator(NaturalFrequency)
	{}

	FCritDampSpringInterpolatorVector(float InNatFreq)
		: NaturalFrequency(InNatFreq)
		, Interpolator(InNatFreq)
	{}

	FVector Eval(FVector NewEquilibrium, float DeltaTime)
	{
		Interpolator.NaturalFrequency = NaturalFrequency;
		return Interpolator.Eval(NewEquilibrium, DeltaTime);
	}

	FVector EvalSubstepped(FVector NewEquilibrium, float DeltaTime)
	{
		Interpolator.NaturalFrequency = NaturalFrequency;
		return Interpolator.EvalSubstepped(NewEquilibrium, DeltaTime);
	}

	void SetNaturalFrequency(float NewNaturalFrequency) 
	{ 
		NaturalFrequency = NewNaturalFrequency; 
		Interpolator.NaturalFrequency = NaturalFrequency;
	};
	
	void Reset() { Interpolator.Reset(); }
	FVector GetCurrentValue() const { return Interpolator.GetCurrentValue(); }
	void SetInitialValue(FVector InitialValue) { Interpolator.Init(InitialValue); }

public:
	/** Higher = a stiffer spring */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spring")
	float NaturalFrequency = 20.f;

private:
	TCritDampSpringInterpolator<FVector> Interpolator;
};


/** UStruct wrapper for critically damped spring rotator interpolator */
USTRUCT(BlueprintType)
struct FCritDampSpringInterpolatorRotator
{
	GENERATED_BODY();

public:
	FCritDampSpringInterpolatorRotator()
		: Interpolator(NaturalFrequency)
	{}

	FCritDampSpringInterpolatorRotator(float InNatFreq)
		: NaturalFrequency(InNatFreq)
		, Interpolator(InNatFreq)
	{}

	FRotator Eval(FRotator NewEquilibrium, float DeltaTime)
	{
		Interpolator.NaturalFrequency = NaturalFrequency;
		return Interpolator.Eval(NewEquilibrium, DeltaTime);
	}

	FRotator EvalSubstepped(FRotator NewEquilibrium, float DeltaTime)
	{
		Interpolator.NaturalFrequency = NaturalFrequency;
		return Interpolator.EvalSubstepped(NewEquilibrium, DeltaTime);
	}

	void SetNaturalFrequency(float NewNaturalFrequency)
	{
		NaturalFrequency = NewNaturalFrequency;
		Interpolator.NaturalFrequency = NaturalFrequency;
	};

	void Reset() { Interpolator.Reset(); }
	FRotator GetCurrentValue() const { return Interpolator.GetCurrentValue(); }
	void SetInitialValue(FRotator InitialValue) { Interpolator.Init(InitialValue); }

public:
	/** Higher = a stiffer spring */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spring")
	float NaturalFrequency = 20.f;

private:
	TCritDampSpringInterpolator<FRotator> Interpolator;
};


struct FCitySampleInterpolatorTests
{
public:
	static bool RunSubstepTest_CDSpringVector();
};
