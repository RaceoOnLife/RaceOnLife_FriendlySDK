// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/NoExportTypes.h"
#include "AvaturnSettings.generated.h"

#if WITH_EDITOR
DECLARE_DELEGATE_OneParam(FEditorSettingsChanged, const FName&);
#endif

UCLASS(config=Game, defaultconfig, meta = (DisplayName="Avaturn"))
class AVATURN_API UAvaturnSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAvaturnSettings();

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Avatar Caching", meta = (DisplayName = "Enable Avatar Caching",
		ToolTip = "If checked, the loaded avatars will be saved in the local storage."))
	bool bEnableAvatarCaching;

	static void SetAvatarCaching(bool bEnableCaching);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	FEditorSettingsChanged SettingsChanged;
#endif

};
