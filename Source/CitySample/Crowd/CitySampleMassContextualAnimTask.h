// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTypes.h"
#include "MassStateTreeTypes.h"
#include "CitySampleMassContextualAnimTask.generated.h"

class UMassSignalSubsystem;
struct FMassMontageFragment;
struct FTransformFragment;
struct FMassMoveTargetFragment;
struct FMassActorFragment;

USTRUCT()
struct FCitySampleMassContextualAnimTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = Input, meta = (Optional))
	FMassEntityHandle TargetEntity;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float Duration = 0.0f;

	UPROPERTY()
	float ComputedDuration = 0.0f;

	/** Accumulated time used to stop task if a montage is set */
	UPROPERTY()
	float Time = 0.f;
};

/** This task is the same as MassContextualAnimTask but it picks the animation to play from 
	the character definition's ContextualAnimDataAsset.
	To add an animation, add a name tag on CommonCrowdContextualAnimNames in project settings,
	and add an entry to that tag in your contextual anim data asset. */
USTRUCT(meta = (DisplayName = "CitySample Mass Contextual Anim Task"))
struct FCitySampleMassContextualAnimTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCitySampleMassContextualAnimTaskInstanceData; 

	FCitySampleMassContextualAnimTask();

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
	TStateTreeExternalDataHandle<FMassMontageFragment, EStateTreeExternalDataRequirement::Optional> MontageRequestHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FMassActorFragment> ActorHandle;

	UPROPERTY(EditAnywhere, Category = Anim,  meta = (GetOptions = "CitySampleMassCrowd.MassCrowdAnimationSettings.GetContextualAnimOptions"))
	FName ContextualAnimName = NAME_None;
};