// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleConvertedISMActor.h"
#include "Components/SceneComponent.h"

ACitySampleConvertedISMActor::ACitySampleConvertedISMActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->Mobility = EComponentMobility::Static;
}