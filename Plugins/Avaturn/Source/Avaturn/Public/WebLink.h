// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AvaturnTypes.h"
#include "WebLink.generated.h"

class UAvaturnWebBrowser;

DECLARE_DYNAMIC_DELEGATE_OneParam(FAvatarExport, const FExportAvatarResult&, ExportResult);

UCLASS()
class UWebLink : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Avaturn")
	void AvatarGenerated(FString JsonResponse);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Callback"), Category = "Avaturn")
	void SetAvatarExportCallback(const FAvatarExport& AvatarExportCallback);

	TSharedPtr<FJsonValue> ParseJSON(FString JsonResponse);

private:

	FAvatarExport AvatarExportResponse;
	FString LastAvatarUrl;
};