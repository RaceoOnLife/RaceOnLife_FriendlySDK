// Copyright © 2023++ Avaturn

#include "AvaturnEditorWidget.h"
#include "Kismet/GameplayStatics.h"
#include "AvaturnGameSubsystem.h"

void UAvaturnEditorWidget::NativeOnInitialized()
{
	GameSubsystem = UGameInstance::GetSubsystem<UAvaturnGameSubsystem>(GetWorld()->GetGameInstance());
	if (IsValid(GameSubsystem))
	{
		GameSubsystem->CustomTokenInvalidDelegate.BindUFunction(this, "OnCustomTokenInvalid");
		GameSubsystem->AssetsLoadedDelegate.BindUFunction(this, "OnAvatarAssetsLoaded");

		Super::NativeOnInitialized();
	}
}

void UAvaturnEditorWidget::SelectAsset(const FString& Id)
{
	GameSubsystem->EquipAvatarAsset(Id);
}

void UAvaturnEditorWidget::SelectBodyType(const int32 BodyType)
{
	GameSubsystem->EquipAvatarBody({ -1, -1, BodyType});
}

void UAvaturnEditorWidget::SelectHairColor(const FString& HairColor)
{
	GameSubsystem->SelectHairColor(HairColor);
}

void UAvaturnEditorWidget::InitAvaturnSDK(const FString& NewAuthString)
{
	GameSubsystem->InitAvaturnSDK(NewAuthString);
}