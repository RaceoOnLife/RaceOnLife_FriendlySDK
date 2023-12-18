// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "CollisionShape.h"
#include "ScalableFloat.h"
#include "WorldCollision.h"

#include "CitySampleAsyncActorDetectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDetectActor, AActor*, DetectedActor, const FHitResult&, HitResult);

USTRUCT(BlueprintType)
struct FCitySampleAsyncTraceDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere)
	FVector TraceStartOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere)
	FVector2D SpeedRange = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere)
	FVector2D TraceRange = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere)
	float TraceExtent = 0.0f;

	UPROPERTY(Transient)
	TArray<AActor*> RecentlyDetectedActors;

	FTraceHandle ActorDetectionTraceHandle;
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UCitySampleAsyncActorDetectionComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()
	
public:

	UCitySampleAsyncActorDetectionComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FOnDetectActor OnDetectActor;

	UPROPERTY(EditAnywhere)
	TArray<UClass*> ClassesToConsider;

	UPROPERTY(EditAnywhere)
	TArray<FCitySampleAsyncTraceDef> TraceDefinitions;

	FTraceDelegate ActorDetectionTraceDelegate;

	bool IsValidHitClass(UClass* HitClass);

	void HandleAsyncActorDetectionTrace(const FTraceHandle& InTraceHandle, FTraceDatum& InTraceDatum);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};