// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "AvaturnTypes.h"

class FAvaturnUrlConvertor
{
public:
	static FAvatarUri CreateAvatarUri(const FString& Url, const FString AvatarId);
};
