// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AvaturnGameSubsystem.generated.h"

UCLASS()
class UAvaturnGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UAvaturnGameSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, Category="Avaturn")
	class UAvaturnMemoryCache* MemoryCache;
};