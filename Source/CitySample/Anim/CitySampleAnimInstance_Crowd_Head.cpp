// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimInstance_Crowd_Head.h"
#include "Components/SkeletalMeshComponent.h"
#include "Crowd/CrowdCharacterActor.h"

UCitySampleAnimInstance_Crowd_Head::UCitySampleAnimInstance_Crowd_Head(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCitySampleAnimInstance_Crowd_Head::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Head_NativeInitializeAnimation);

	CrowdCharacter = Cast<ACitySampleCrowdCharacter>(GetOwningActor());
	SourceMeshComponent = CrowdCharacter ? CrowdCharacter->GetMesh() : nullptr;
}

void UCitySampleAnimInstance_Crowd_Head::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	QUICK_SCOPE_CYCLE_COUNTER(STAT_UCitySampleAnimInstance_Crowd_Head_NativeUpdateAnimation);

	//Workaround for bug when copying from a leader pose that has a leader pose with a different bone hierarchy
	if(SourceMeshComponent && SourceMeshComponent->LeaderPoseComponent.IsValid())
	{
		USkeletalMeshComponent* LeaderPoseComp = Cast<USkeletalMeshComponent>(SourceMeshComponent->LeaderPoseComponent.Get());
		if(LeaderPoseComp)
		{
			SourceMeshComponent = LeaderPoseComp;
		}
	}

	if(CrowdCharacter)
	{
		//This needs to be in Update. Because in InitializeAnimation might have the wrong value.
		const ECitySampleCrowdBodyType BodyType = CrowdCharacter->CharacterOptions.BodyType;

		if (BodyType == ECitySampleCrowdBodyType::OverWeight)
		{
			SetMorphTarget(CurveName_Overweight, 1.f);
			SetMorphTarget(CurveName_Underweight, 0.f);
		}
		else if (BodyType == ECitySampleCrowdBodyType::UnderWeight)
		{
			SetMorphTarget(CurveName_Overweight, 0.f);
			SetMorphTarget(CurveName_Underweight, 1.f);
		}
	}
}