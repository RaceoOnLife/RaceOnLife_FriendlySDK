// Copyright © 2023++ Avaturn

#include "AvaturnActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "AvaturnComponent.h"

AAvaturnActor::AAvaturnActor()
{
	// Set this actor not to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = false;
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SetRootComponent(SkeletalMeshComponent);

	AvaturnComponent = CreateDefaultSubobject<UAvaturnComponent>(TEXT("Avaturn"));
	SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
}
