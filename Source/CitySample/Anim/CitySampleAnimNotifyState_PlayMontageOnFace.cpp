// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAnimNotifyState_PlayMontageOnFace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Crowd/CrowdCharacterActor.h"

UCitySampleAnimNotifyState_PlayMontageOnFace::UCitySampleAnimNotifyState_PlayMontageOnFace(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCitySampleAnimNotifyState_PlayMontageOnFace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp != nullptr)
	{
		if (ACitySampleCrowdCharacter* CrowdCharacter = Cast<ACitySampleCrowdCharacter>(MeshComp->GetOwner()))
		{
			UAnimInstance* LeaderAnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
			const UAnimMontage* LeaderMontage = Cast<UAnimMontage>(Animation);

			USkeletalMeshComponent* FollowerMesh = CrowdCharacter->GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Head);
			if (FollowerMesh && LeaderMontage)
			{
				UAnimInstance* FollowerAnimInstance = FollowerMesh ? FollowerMesh->GetAnimInstance() : nullptr;
				UAnimMontage* FollowerMontage = Cast<UAnimMontage>(AnimToPlay);

				if (FollowerMontage == nullptr)
				{
					FollowerMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(AnimToPlay, FAnimSlotGroup::DefaultSlotName, LeaderMontage->GetDefaultBlendInTime(), LeaderMontage->GetDefaultBlendOutTime(), 1.f);
				}

				if (FollowerAnimInstance && FollowerMontage && LeaderAnimInstance)
				{
					FollowerAnimInstance->Montage_Play(FollowerMontage);

					if (bTryToSync)
					{
						FAnimMontageInstance* LeaderMontageInstance = LeaderAnimInstance->GetActiveInstanceForMontage(LeaderMontage);
						FAnimMontageInstance* FollowerMontageInstance = FollowerAnimInstance->GetActiveInstanceForMontage(FollowerMontage);

						if (LeaderMontageInstance && FollowerMontageInstance)
						{
							FollowerMontageInstance->MontageSync_Follow(LeaderMontageInstance);
						}
					}
				}
			}
		}
	}
}

void UCitySampleAnimNotifyState_PlayMontageOnFace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (bStopOnNotifyEnd && (MeshComp != nullptr))
	{
		if (ACitySampleCrowdCharacter* CrowdCharacter = Cast<ACitySampleCrowdCharacter>(MeshComp->GetOwner()))
		{
			USkeletalMeshComponent* FollowerMesh = CrowdCharacter->GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Head);
			if (FollowerMesh)
			{
				UAnimInstance* FollowerAnimInstance = FollowerMesh ? FollowerMesh->GetAnimInstance() : nullptr;
				const UAnimMontage* FollowerMontage = Cast<UAnimMontage>(AnimToPlay);;

				if (FollowerAnimInstance && FollowerMontage)
				{
					FollowerAnimInstance->Montage_StopWithBlendOut(FollowerMontage->GetBlendOutArgs(), FollowerMontage);
				}
			}
		}
	}
}

#if WITH_EDITOR
bool UCitySampleAnimNotifyState_PlayMontageOnFace::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return Cast<UAnimMontage>(Animation) != nullptr;
}
#endif