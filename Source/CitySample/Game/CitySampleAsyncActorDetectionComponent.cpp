// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleAsyncActorDetectionComponent.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CitySample.h"

namespace AsyncActorDetectionCVars
{
	static int32 bDebugActorDetectionTrace = 0;
	static FAutoConsoleVariableRef CVarDebugActorDetectionTrace(
		TEXT("CitySample.DebugActorDetectionTrace"), bDebugActorDetectionTrace,
		TEXT("Turn on debug drawing for async actor detection."),
		ECVF_Default);
}

UCitySampleAsyncActorDetectionComponent::UCitySampleAsyncActorDetectionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;


	ActorDetectionTraceDelegate.BindUObject(this, &UCitySampleAsyncActorDetectionComponent::HandleAsyncActorDetectionTrace);
}

void UCitySampleAsyncActorDetectionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (Owner->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	for (int i = 0; i < TraceDefinitions.Num(); i++)
	{
		// Already have a trace in flight
		if (TraceDefinitions[i].ActorDetectionTraceHandle.IsValid())
		{
			return;
		}

		FVector Start = Owner->GetActorLocation();
		FRotator TraceRot = Owner->GetActorRotation();

		Start += TraceRot.RotateVector(TraceDefinitions[i].TraceStartOffset);

		bool bMeetsMinimumSpeed = false;
		FVector ParentVelocity = FVector::ZeroVector;
		float Speed;
		TArray<AActor*> IgnoreActors;

		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			ParentVelocity = Pawn->GetVelocity();
			TraceRot = ParentVelocity.Rotation();
			IgnoreActors.Add(Owner);
		}
		else if (AActor* AttachParentActor = Owner->GetAttachParentActor())
		{
			Start = AttachParentActor->GetActorLocation();
			ParentVelocity = AttachParentActor->GetVelocity();
			TraceRot = ParentVelocity.Rotation();
			IgnoreActors.Add(Owner);
			IgnoreActors.Add(AttachParentActor);
		}

		Speed = ParentVelocity.Size();

		float TraceLength = TraceDefinitions[i].TraceRange.X;
		if (Speed >= TraceDefinitions[i].SpeedRange.X)
		{
			bMeetsMinimumSpeed = true;
			TraceLength = FMath::GetMappedRangeValueClamped(TraceDefinitions[i].SpeedRange, TraceDefinitions[i].TraceRange, Speed);
		}

		if (bMeetsMinimumSpeed)
		{
			FVector End = Start + TraceRot.Vector() * TraceLength;

	#if ENABLE_DRAW_DEBUG
			if (GIsEditor && AsyncActorDetectionCVars::bDebugActorDetectionTrace)
			{
				TIndirectArray<FWorldContext> WorldContexts = GEngine->GetWorldContexts();
				for (int32 WorldIdx = 0; WorldIdx < WorldContexts.Num(); WorldIdx++)
				{
					if (WorldContexts[WorldIdx].WorldType == EWorldType::PIE)
					{
						DrawDebugSphere(WorldContexts[WorldIdx].World(), Start, 10, 10, FColor::Green, false, 10.0f);

						if (FMath::IsNearlyZero(TraceDefinitions[i].TraceExtent))
						{
							DrawDebugLine(WorldContexts[WorldIdx].World(), Start, End, FColor::Green, false, 10.0f);
						}
						else
						{
							DrawDebugCylinder(WorldContexts[WorldIdx].World(), Start, End, TraceDefinitions[i].TraceExtent, 10, FColor::Green, false, 10.0f);
						}
					}
				}
			}
	#endif

			static FName NAME_AsyncActorDetection(TEXT("AsyncActorDetection"));
			FCollisionQueryParams QueryParams(NAME_AsyncActorDetection, false);
			QueryParams.AddIgnoredActors(IgnoreActors);

			if (FMath::IsNearlyZero(TraceDefinitions[i].TraceExtent))
			{
				TraceDefinitions[i].ActorDetectionTraceHandle = World->AsyncLineTraceByChannel(EAsyncTraceType::Single, Start, End, TraceDefinitions[i].CollisionChannel, QueryParams, FCollisionResponseParams::DefaultResponseParam, &ActorDetectionTraceDelegate);
			}
			else
			{
				FCollisionShape CollisionShape;
				CollisionShape.SetSphere(TraceDefinitions[i].TraceExtent);

				TraceDefinitions[i].ActorDetectionTraceHandle = World->AsyncSweepByChannel(EAsyncTraceType::Single, Start, End, FQuat::Identity, TraceDefinitions[i].CollisionChannel, CollisionShape, QueryParams, FCollisionResponseParams::DefaultResponseParam,  &ActorDetectionTraceDelegate);
			}
		}
	}
}

bool UCitySampleAsyncActorDetectionComponent::IsValidHitClass(UClass* HitClass)
{
	if (ClassesToConsider.Num() == 0)
	{
		return true;
	}

	for (int i = 0; i < ClassesToConsider.Num(); i++)
	{
		if (HitClass->IsChildOf(ClassesToConsider[i]))
		{
			return true;
		}
	}

	return false;
}

void UCitySampleAsyncActorDetectionComponent::HandleAsyncActorDetectionTrace(const FTraceHandle& InTraceHandle, FTraceDatum& InTraceDatum)
{

	for (int i = 0; i < TraceDefinitions.Num(); i++)
	{
		if (TraceDefinitions[i].ActorDetectionTraceHandle == InTraceHandle)
		{
			TraceDefinitions[i].ActorDetectionTraceHandle = FTraceHandle();

			bool bFoundValidHit = false;
			FHitResult* HitResult = FHitResult::GetFirstBlockingHit(InTraceDatum.OutHits);
			if (HitResult)
			{
				if (HitResult->HasValidHitObjectHandle())
				{
					AActor* HitActor = HitResult->GetActor();
					
					if (AsyncActorDetectionCVars::bDebugActorDetectionTrace)
					{
						UE_LOG(LogCitySample, Log, TEXT("Async actor trace hit %s"), *HitActor->GetName());
					}

					if (IsValidHitClass(HitActor->GetClass()))
					{
						bFoundValidHit = true;

						if (!TraceDefinitions[i].RecentlyDetectedActors.Contains(HitActor))
						{
							TraceDefinitions[i].RecentlyDetectedActors.Add(HitActor);

							// Broadcast the hit
							OnDetectActor.Broadcast(HitActor, *HitResult);
						}
					}
				}
			}		

			if (!bFoundValidHit)
			{
				TraceDefinitions[i].RecentlyDetectedActors.Reset();
			}
			break;
		}
	}
}