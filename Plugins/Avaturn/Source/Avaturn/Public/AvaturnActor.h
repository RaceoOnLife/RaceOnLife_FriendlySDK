// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AvaturnActor.generated.h"

/** Intended to be used as an actor for runtime loading and displaying the avatars. */
UCLASS(BlueprintType)
class AVATURN_API AAvaturnActor : public AActor
{
	GENERATED_BODY()

public:
	/** Default constructor. Sets default values for this actor's properties. */
	AAvaturnActor();

	/** The default SkeletalMeshComponent. The skeletal mesh will be set during the avatar loading process. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkeletalMeshComponent;

	/** Handles the loading and setup of the avatar. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class UAvaturnComponent* AvaturnComponent;
};
