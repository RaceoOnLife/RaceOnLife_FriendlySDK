// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleCheatManager.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "ChaosVehicleMovementComponent.h"
#include "MassSpawner.h"

#include "Game/CitySampleGameMode.h"
#include "Game/CitySamplePlayerController.h"
#include "Vehicles/CitySampleVehicleBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogCitySampleCheatManager, Log, All);


UCitySampleCheatManager::UCitySampleCheatManager()
{
	SummonOffset = FVector(500.0f, 0.0f, 100.0f);
}

void UCitySampleCheatManager::FlipVehicle()
{	
	if (APlayerController* const MyPlayerController = GetOuterAPlayerController())
	{	
		if (APawn* Pawn = MyPlayerController->GetPawn())
		{			
			if (ACitySampleVehicleBase* Vehicle = Cast<ACitySampleVehicleBase>(Pawn))
			{ 
				Vehicle->FlipVehicle();		
			}
		}		
	}
}

void UCitySampleCheatManager::Summon(const FString& ClassName)
{
	SummonAtOffset(ClassName, SummonOffset.X, SummonOffset.Y, SummonOffset.Z);
}

void UCitySampleCheatManager::SummonBlueprintCallable(const FString& ClassName)
{
	Summon(ClassName);
}

void UCitySampleCheatManager::SummonAtOffset(const FString& ClassName, float xOffset, float yOffset, float zOffset)
{
	UE_LOG(LogCitySampleCheatManager, Log, TEXT("Fabricate %s"), *ClassName);

	bool bIsValidClassName = true;
	FString FailureReason;
	if (ClassName.Contains(TEXT(" ")))
	{
		FailureReason = FString::Printf(TEXT("ClassName contains a space."));
		bIsValidClassName = false;
	}
	else if (!FPackageName::IsShortPackageName(ClassName))
	{
		if (ClassName.Contains(TEXT(".")))
		{
			FString PackageName;
			FString ObjectName;
			ClassName.Split(TEXT("."), &PackageName, &ObjectName);

			const bool bIncludeReadOnlyRoots = true;
			FText Reason;
			if (!FPackageName::IsValidLongPackageName(PackageName, bIncludeReadOnlyRoots, &Reason))
			{
				FailureReason = Reason.ToString();
				bIsValidClassName = false;
			}
		}
		else
		{
			FailureReason = TEXT("Class names with a path must contain a dot. (i.e /Script/Engine.StaticMeshActor)");
			bIsValidClassName = false;
		}
	}

	bool bSpawnedActor = false;
	if (bIsValidClassName)
	{
		const TSoftClassPtr<AActor>* Class = SummonShortNamesMap.Find(ClassName);
		UClass* NewClass = nullptr;

		if (Class)
		{
			NewClass = Class->LoadSynchronous();
		}
		
		if (!NewClass)
		{
			NewClass = UClass::TryFindTypeSlow<UClass>(ClassName);
		}

		if (NewClass)
		{
			if (NewClass->IsChildOf(AActor::StaticClass()))
			{
				APlayerController* const MyPlayerController = GetOuterAPlayerController();
				if (MyPlayerController)
				{
					FRotator const SpawnRot = MyPlayerController->GetControlRotation();
					FVector SpawnLoc = MyPlayerController->GetFocalLocation();
					const FVector Offset = { xOffset, yOffset, zOffset };
					SpawnLoc += Offset.RotateAngleAxis(SpawnRot.Yaw, FVector::UpVector);

					FActorSpawnParameters SpawnInfo;
					SpawnInfo.Instigator = MyPlayerController->GetInstigator();
					AActor* Actor = GetWorld()->SpawnActor(NewClass, &SpawnLoc, 0, SpawnInfo);
					if (Actor)
					{
						bSpawnedActor = true;
					}
					else
					{
						FailureReason = TEXT("SpawnActor failed.");
						bSpawnedActor = false;
					}
				}
			}
			else
			{
				FailureReason = TEXT("Class is not derived from Actor.");
				bSpawnedActor = false;
			}
		}
		else
		{
			FailureReason = TEXT("Failed to find class.");
			bSpawnedActor = false;
		}
	}

	if (!bSpawnedActor)
	{
		UE_LOG(LogCitySampleCheatManager, Warning, TEXT("Failed to summon %s. Reason: %s"), *ClassName, *FailureReason);
	}
}

void UCitySampleCheatManager::SummonAtOffsetBlueprintCallable(const FString& ClassName, FVector Offset)
{
	SummonAtOffset(ClassName, Offset.X, Offset.Y, Offset.Z);
}

void UCitySampleCheatManager::CitySampleHideUI(const bool bShouldBeHidden)
{
	if (ACitySamplePlayerController* const MyPlayerController = Cast<ACitySamplePlayerController>(GetOuterAPlayerController()))
	{
		if (UCitySampleUIComponent* const CitySampleUI = MyPlayerController->GetCitySampleUIComponent())
		{
			CitySampleUI->HideUI(bShouldBeHidden);
		}
	}
}

void UCitySampleCheatManager::WatchRandomVehicle(bool bWatch)
{
	if (ACitySamplePlayerController* const MyPlayerController = Cast<ACitySamplePlayerController>(GetOuterAPlayerController()))
	{
		if (bWatch)
		{
			TArray<AActor*> AllVehicles;
			UGameplayStatics::GetAllActorsOfClass(this, ACitySampleVehicleBase::StaticClass(), AllVehicles);

			if (AllVehicles.Num() > 0)
			{
				const int32 Index = FMath::RandRange(0, AllVehicles.Num() - 1);

				AActor* VehicleToWatch = AllVehicles[Index];

				MyPlayerController->SetViewTarget(VehicleToWatch);
			}
		}
		else
		{
			MyPlayerController->SetViewTarget(MyPlayerController->GetPawn());
		}

	}
}

void UCitySampleCheatManager::TuneCurrentVehicle()
{
	if (ACitySamplePlayerController* const MyPlayerController = Cast<ACitySamplePlayerController>(GetOuterAPlayerController()))
	{
		if (AWheeledVehiclePawn* Vehicle = MyPlayerController->GetPawn<AWheeledVehiclePawn>())
		{
			MyPlayerController->ConsoleCommand(
				*FString::Printf(TEXT("editobject %s"), *(Vehicle->GetVehicleMovementComponent()->GetPathName()))
			);
			MyPlayerController->ConsoleCommand(
				*FString::Printf(TEXT("editobject %s"), *(Vehicle->GetPathName()))
			);
		}
	}
}

void UCitySampleCheatManager::ScaleCrowdCount(const float Scale)
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World, AMassSpawner::StaticClass()); It; ++It)
		{
			if(AMassSpawner* Spawner = Cast<AMassSpawner>(*It))
			{
				Spawner->ScaleSpawningCount(Scale);
				Spawner->DoDespawning();
				Spawner->DoSpawning();
			}
		}
	}
}

