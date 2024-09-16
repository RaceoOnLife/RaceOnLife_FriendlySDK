// Fill out your copyright notice in the Description page of Project Settings.

#include "WebLink.h"
#include "AvaturnWebBrowser.h"
#include "Json.h"

void UWebLink::AvatarGenerated(FString JsonResponse)
{
	FExportAvatarResult ExportResult;
	FString Url = "";
	FString AvatarId = "";
	bool bDataUrl = false;
	bool bSupportFaceAnims = false;

	TSharedPtr<FJsonObject> JsonParsed = ParseJSON(JsonResponse)->AsObject();;

	if (JsonParsed == nullptr)
	{
		return;
	}

	if (!JsonParsed->HasField("url"))
	{
		return;
	}

	ExportResult.bDataUrl = JsonParsed->GetStringField("urlType").Equals(TEXT("dataURL"));
	ExportResult.bAvatarSupportsFaceAnimations = JsonParsed->GetBoolField("avatarSupportsFaceAnimations");

	ExportResult.Url = JsonParsed->GetStringField("url");

	ExportResult.AvatarId = JsonParsed->GetStringField("avatarId");

	ExportResult.BodyId = JsonParsed->GetStringField("bodyId");
	ExportResult.Gender = JsonParsed->GetStringField("gender");
	ExportResult.SessionId = JsonParsed->GetStringField("sessionId");

	if (ExportResult.bDataUrl)
	{
		// remove "data:application/octet-stream;base64,"
		ExportResult.Url.RemoveAt(0, 37);
	}

	if (ExportResult.Url.IsEmpty())
	{
		return;
	}

	AvatarExportResponse.Execute(ExportResult);
	LastAvatarUrl = ExportResult.Url;
}

void UWebLink::SetAvatarExportCallback(const FAvatarExport& AvatarExportCallback)
{
	AvatarExportResponse = AvatarExportCallback;
}

TSharedPtr<FJsonValue> UWebLink::ParseJSON(FString JsonResponse)
{
	TSharedPtr<FJsonValue> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(*JsonResponse);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		return JsonParsed;
	}

	return nullptr;
}