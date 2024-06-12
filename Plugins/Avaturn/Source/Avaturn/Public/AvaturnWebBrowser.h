// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "WebBrowser.h"
#include "WebLink.h"
#include "SWebBrowser.h"
#include "AvaturnWebBrowser.generated.h"
/**
 * 
 */
UCLASS()
class UAvaturnWebBrowser : public UWebBrowser
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Load Avaturn Browser"), Category = "Avaturn|Web Browser")
	void LoadAvaturn();

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", DisplayName = "Setup Browser"), Category = "Avaturn|Web Browser")
	void SetupBrowser(const UObject* WorldContextObject, const FAvatarExport& Response);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Bind Browser To Object"), Category = "Avaturn|Web Browser")
	void BindBrowserToObject();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Toggle Orientation"), Category = "Avaturn|Web Browser")
	void ToggleOrientation(bool bPortrait);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn|Web Browser")
	FString SubDomain = "demo";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avaturn|Web Browser")
	FString LinkFromAPI = "";

	uint8 BrowserFPS = 60;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	UPROPERTY()
	UWebLink* WebLinkObject;

	FString LastUrl = "";
};