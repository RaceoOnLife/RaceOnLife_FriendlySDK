// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CitySampleAnimNotifyState_PlayMontageOnFace.generated.h"

UCLASS()
class CITYSAMPLE_API UCitySampleAnimNotifyState_PlayMontageOnFace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UCitySampleAnimNotifyState_PlayMontageOnFace(const FObjectInitializer& ObjectInitializer);

	// UAnimNotifyState interface
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif
	// End of UAnimNotifyState interface

	// The animation to play. If not using montage, it will create a dynamic one
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montage)
	class UAnimSequenceBase* AnimToPlay = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montage)
	bool bTryToSync = false;

	// If false, the anim will keep until its duration is up, and potentially past the main montage. This won't work if the anim is played as a dynamic montage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montage)
	bool bStopOnNotifyEnd = true;
};
