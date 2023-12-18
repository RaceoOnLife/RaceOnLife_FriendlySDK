// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"
#include "WorldPartition/DataLayer/ActorDataLayer.h"
#include "CitySamplePlayFromHere.generated.h"

UCLASS(hidecategories=(DataLayers,Cooking,HLOD,Input,WorldPartition,Collision,Replication,Rendering))
class ACitySamplePlayFromHere : public AActor
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	virtual bool CanChangeIsSpatiallyLoadedFlag() const override { return false; }
	virtual void OnPlayFromHere() override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PlayFromHere)
	bool bOverrideActiveDataLayers;
		
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PlayFromHere, meta=(EditCondition="bOverrideActiveDataLayers"))
	TArray<FActorDataLayer> ActiveDataLayers;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PlayFromHere)
	bool bOverrideLoadedDataLayers;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PlayFromHere, meta=(EditCondition="bOverrideLoadedDataLayers"))
	TArray<FActorDataLayer> LoadedDataLayers;
#endif

	/** blueprint event: this will get called before BeginPlay if this actor was choosen to play from here. */
	UFUNCTION(BlueprintImplementableEvent, Category=PlayFromHere)
	void PlayFromHereEvent();
};
