// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ReplicaSettings.generated.h"

/**
 * 
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Replica Settings"))
class REPLICANPC_API UReplicaSettings : public UDeveloperSettings
{
	 GENERATED_BODY()

		UPROPERTY(config, EditAnywhere, Category = "Replica API key")
	    FString API_Key = "None";

	
	    
};
