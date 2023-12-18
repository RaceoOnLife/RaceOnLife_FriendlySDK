// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "CrowdCharacterDefinition.h"
#include "MassEntityTypes.h"
#include "MassObserverProcessor.h"

#include "CrowdVisualizationFragment.generated.h"

struct FMassEntityQuery;

USTRUCT()
struct CITYSAMPLE_API FCitySampleCrowdVisualizationFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "")
	FCrowdVisualizationID VisualizationID;

	UPROPERTY(EditAnywhere, Category = "")
	uint32 TopColor = 0;

	UPROPERTY(EditAnywhere, Category = "")
	uint32 BottomColor = 0;

	UPROPERTY(EditAnywhere, Category = "")
	uint32 ShoesColor = 0;

	UPROPERTY(EditAnywhere, Category = "")
	uint8 SkinAtlasIndex = 0;
};

UCLASS()
class CITYSAMPLE_API UCitySampleCrowdVisualizationFragmentInitializer : public UMassObserverProcessor
{
	GENERATED_BODY()
	
public:
	UCitySampleCrowdVisualizationFragmentInitializer();	

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

protected:
	FMassEntityQuery EntityQuery;


	uint32 FindColorOverride(FCrowdCharacterDefinition& CharacterDefinition, USkeletalMesh* SkelMesh);
	UAnimToTextureDataAsset* GetAnimToTextureDataAsset(TSoftObjectPtr<UAnimToTextureDataAsset> SoftPtr);
};