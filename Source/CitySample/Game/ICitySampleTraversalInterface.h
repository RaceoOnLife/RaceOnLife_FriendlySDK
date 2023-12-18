// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "Util/CitySampleTypes.h"
#include "ICitySampleTraversalInterface.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class CITYSAMPLE_API UCitySampleTraversalInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
public:

};

class CITYSAMPLE_API ICitySampleTraversalInterface : public IInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	/** Called to determine the type of traversal mode this instance provides. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	EPlayerTraversalState GetTraversalState() const;
};