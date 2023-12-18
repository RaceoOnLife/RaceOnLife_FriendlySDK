// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UObject/Interface.h"
#include "EnhancedInputSubsystemInterface.h"
#include "ICitySampleInputInterface.generated.h"

template<class UserClass, typename FuncType>
static bool BindInputAction(
	UEnhancedInputComponent* PlayerInputComponent,
	const UInputAction* Action,
	const ETriggerEvent EventType,
	UserClass* Object, FuncType Func)
{
	if (ensure(Action != nullptr))
	{
		PlayerInputComponent->BindAction(Action, EventType, Object, Func);
		return true;
	}

	return false;
}

UINTERFACE(BlueprintType, Blueprintable)
class CITYSAMPLE_API UCitySampleInputInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
public:

};

class CITYSAMPLE_API ICitySampleInputInterface : public IInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	/** Called to add any necessary inputs. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void AddInputContext(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface);

	/** Called to remove any necessary inputs. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveInputContext(const TScriptInterface<IEnhancedInputSubsystemInterface>& SubsystemInterface);
};