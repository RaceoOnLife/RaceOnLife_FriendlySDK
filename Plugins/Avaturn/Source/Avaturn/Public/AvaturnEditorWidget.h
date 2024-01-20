// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AvaturnEditorWidget.generated.h"


UCLASS()
class AVATURN_API UAvaturnEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	void OnAvatarAssetsLoaded();

	UFUNCTION(BlueprintImplementableEvent)
	void OnCustomTokenInvalid();

	UFUNCTION(BlueprintCallable)
	void InitAvaturnSDK(const FString& NewAuthString);

	UFUNCTION(BlueprintCallable)
	void SelectAsset(const FString& Id);

	UFUNCTION(BlueprintCallable)
	void SelectBodyType(const int32 BodyType);

	UFUNCTION(BlueprintCallable)
	void SelectHairColor(const FString& HairColor);

	UPROPERTY(BlueprintReadOnly)
	class UAvaturnGameSubsystem* GameSubsystem = nullptr;


	virtual void NativeOnInitialized() override;
};
