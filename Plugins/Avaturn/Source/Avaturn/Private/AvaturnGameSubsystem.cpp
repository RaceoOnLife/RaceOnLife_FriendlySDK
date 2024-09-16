// Copyright © 2023++ Avaturn

#include "AvaturnGameSubsystem.h"
#include "AvaturnMemoryCache.h"

UAvaturnGameSubsystem::UAvaturnGameSubsystem()
	: MemoryCache(nullptr)
{}

void UAvaturnGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	MemoryCache = NewObject<UAvaturnMemoryCache>(this, TEXT("MemoryCache"));
}

void UAvaturnGameSubsystem::Deinitialize()
{
	MemoryCache = nullptr;
}
