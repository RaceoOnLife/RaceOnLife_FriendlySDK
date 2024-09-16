// Copyright © 2023++ Avaturn

#include "AvaturnSettings.h"

UAvaturnSettings::UAvaturnSettings()
	: bEnableAvatarCaching(false)
{
}

void UAvaturnSettings::SetAvatarCaching(bool bEnableCaching)
{
	UAvaturnSettings* Settings = GetMutableDefault<UAvaturnSettings>();
	if (Settings)
	{
		Settings->bEnableAvatarCaching = bEnableCaching;
		Settings->SaveConfig();
	}
}

#if WITH_EDITOR
void UAvaturnSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	(void)SettingsChanged.ExecuteIfBound(PropertyChangedEvent.GetPropertyName());
}
#endif
