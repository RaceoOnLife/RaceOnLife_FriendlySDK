// Copyright © 2023++ Avaturn

#include "AvaturnWebBrowser.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/IPluginManager.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"  
#include "Android/AndroidApplication.h"
#endif

#define LOCTEXT_NAMESPACE "WebBrowser"

static const FString LinkObjectName = TEXT("avaturnlinkobject");

static const FString JSAddFirebaseUseSignInWithRedirect = TEXT("window.avaturnFirebaseUseSignInWithRedirect = true;");
static const FString JSAddForceExportHttpUrl = TEXT("window.avaturnForceExportHttpUrl = true;");

static FString JSAvaturnInit = TEXT("   function loadAvaturn() { \n"
	"\twindow.avaturnSDKEnvironment = JSON.stringify({ engine: 'UE', version: '_VERSION_UE_', platform: '_PLATFORM_UE_', plugin_version: '_PLUGIN_VERSION_' }); \n"
	"\twindow.avaturnSDK.init(null, {}).then(() => window.avaturnSDK.on('export', (data) => {window.ue.avaturnlinkobject.avatargenerated(JSON.stringify(data)); }) ); \n"
	"} \n"
	"if (document.readyState === 'loading') { \n"
	"\tdocument.addEventListener('DOMContentLoaded', loadAvaturn); \n"
	"} \n"
	"else { \n"
	"\tloadAvaturn(); \n"
	"}  ");

void UAvaturnWebBrowser::SetupBrowser(const UObject* WorldContextObject, const FAvatarExport& Response)
{
	if (WebBrowserWidget->GetUrl() != LastUrl)
	{
		LastUrl = WebBrowserWidget->GetUrl();
		BindBrowserToObject();
		WebLinkObject->SetAvatarExportCallback(Response);
		ExecuteJavascript(JSAvaturnInit);
		
		// Use force http url only in multiplayer
		if (!UKismetSystemLibrary::IsStandalone(WorldContextObject))
		{
			ExecuteJavascript(JSAddForceExportHttpUrl);
		}

		ExecuteJavascript(JSAddFirebaseUseSignInWithRedirect);
	}
}

void UAvaturnWebBrowser::BindBrowserToObject()
{
	if (!WebLinkObject)
	{
		WebLinkObject = NewObject<UWebLink>(this, *LinkObjectName);
	}
	WebBrowserWidget->BindUObject(LinkObjectName, WebLinkObject, true);
}

void UAvaturnWebBrowser::LoadAvaturn()
{
	if (WebBrowserWidget->GetUrl().IsEmpty())
	{
		WebBrowserWidget->LoadURL(LinkFromAPI.IsEmpty() ? FString::Printf(TEXT("https://%s.avaturn.dev?sdk=true"), *SubDomain) : LinkFromAPI);
	}
}

TSharedRef<SWidget> UAvaturnWebBrowser::RebuildWidget()
{	
	if (IsDesignTime())
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Web Browser", "Web Browser"))
			];
	}
	else
	{
		WebBrowserWidget = SNew(SWebBrowser)
			.InitialURL("")
			.ShowControls(false)
			.SupportsTransparency(bSupportsTransparency)
			.OnUrlChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnUrlChanged))
			.OnBeforePopup(BIND_UOBJECT_DELEGATE(FOnBeforePopupDelegate, HandleOnBeforePopup))
			.BrowserFrameRate(BrowserFPS);

		JSAvaturnInit = JSAvaturnInit.Replace(TEXT("_VERSION_UE_"), *UKismetSystemLibrary::GetEngineVersion());

#if WITH_EDITOR
		JSAvaturnInit = JSAvaturnInit.Replace(TEXT("_PLATFORM_UE_"), TEXT("editor"));
#else
		JSAvaturnInit = JSAvaturnInit.Replace(TEXT("_PLATFORM_UE_"), *UGameplayStatics::GetPlatformName());
#endif
		
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Avaturn"));

		if (Plugin.IsValid())
		{
			JSAvaturnInit = JSAvaturnInit.Replace(TEXT("_PLUGIN_VERSION_"), *Plugin->GetDescriptor().VersionName);
			
		}

		
		return WebBrowserWidget.ToSharedRef();
	}
}

void UAvaturnWebBrowser::ToggleOrientation(bool bPortrait)
{
#if PLATFORM_ANDROID
#if ENGINE_MAJOR_VERSION > 4 && ENGINE_MINOR_VERSION > 1
	FAndroidMisc::SetAllowedDeviceOrientation(bPortrait ? EDeviceScreenOrientation::Portrait : EDeviceScreenOrientation::LandscapeLeft);
#else
	FAndroidMisc::SetDeviceOrientation(bPortrait ? EDeviceScreenOrientation::Portrait : EDeviceScreenOrientation::LandscapeLeft);
#endif
#endif
}