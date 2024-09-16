// Copyright 2022 Just2Devs. All Rights Reserved.

#include "DFLDebugFL.h"
#include "DebugFunctionLibrarySettings.h"
#include "DFLDebugLog.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#pragma region Print
void UDFLDebugFL::DFLQuickPrintLogMessage(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	UKismetSystemLibrary::PrintString(WorldContext, GetQuickPrintString(WorldContext, String, CallingFunction), true, false, Settings->PrintMessageColour, Duration != -1 ? Duration : Settings->PrintDuration);
	UE_LOG(DFLLog, Log, TEXT("%s"), *GetQuickLogString(WorldContext, String, CallingFunction));
}

void UDFLDebugFL::DFLQuickPrintLogWarning(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	UKismetSystemLibrary::PrintString(WorldContext, GetQuickPrintString(WorldContext, String, CallingFunction), true, false, Settings->PrintWarningColour, Duration != -1 ? Duration : Settings->PrintDuration);
	UE_LOG(DFLLog, Warning, TEXT("%s"), *GetQuickLogString(WorldContext, String, CallingFunction));
}

void UDFLDebugFL::DFLQuickPrintLogError(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	UKismetSystemLibrary::PrintString(WorldContext, GetQuickPrintString(WorldContext, String, CallingFunction), true, false, Settings->PrintErrorColour, Duration != -1 ? Duration : Settings->PrintDuration);
	UE_LOG(DFLLog, Error, TEXT("%s"), *GetQuickLogString(WorldContext, String, CallingFunction));
}

void UDFLDebugFL::DFLQuickPrintMessage(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	UKismetSystemLibrary::PrintString(WorldContext, GetQuickPrintString(WorldContext, String, CallingFunction), true, false, Settings->PrintMessageColour, Duration != -1 ? Duration : Settings->PrintDuration);
}

void UDFLDebugFL::DFLPQuickPrintWarning(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	UKismetSystemLibrary::PrintString(WorldContext, GetQuickPrintString(WorldContext, String, CallingFunction), true, false, Settings->PrintWarningColour, Duration != -1 ? Duration : Settings->PrintDuration);
}

void UDFLDebugFL::DFLQuickPrintError(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	UKismetSystemLibrary::PrintString(WorldContext, GetQuickPrintString(WorldContext, String, CallingFunction), true, false, Settings->PrintErrorColour, Duration != -1 ? Duration : Settings->PrintDuration);
}

void UDFLDebugFL::DFLQuickLogMessage(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	UE_LOG(DFLLog, Log, TEXT("%s"), *GetQuickLogString(WorldContext, String, CallingFunction));
}

void UDFLDebugFL::DFLQuickLogWarning(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	UE_LOG(DFLLog, Warning, TEXT("%s"), *GetQuickLogString(WorldContext, String, CallingFunction));
}

void UDFLDebugFL::DFLQuickLogError(UObject* WorldContext, FString String, float Duration, FString CallingFunction)
{
	UE_LOG(DFLLog, Error, TEXT("%s"), *GetQuickLogString(WorldContext, String, CallingFunction));
}

FString UDFLDebugFL::GetQuickPrintString(UObject* WorldContext, FString String, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	if(!WorldContext && Settings->bPrintFunction) return "(" + CallingFunction + ") " + String;
	
	FString ClassName = WorldContext->GetClass()->GetName();
	ClassName.RemoveFromEnd("_C");
	
	if(Settings->bPrintClass && Settings->bPrintFunction)
	{
		return "(" + ClassName + ") (" + CallingFunction + ") " + String;
	}
	
	if(Settings->bPrintClass)
	{
		return "(" + ClassName + ") " + String;
	}

	if(Settings->bPrintFunction)
	{
		return "(" + CallingFunction + ") " + String;
	}
	
	return String;
}

FString UDFLDebugFL::GetQuickLogString(UObject* WorldContext, FString String, FString CallingFunction)
{
	const UDebugFunctionLibrarySettings* Settings = GetDefault<UDebugFunctionLibrarySettings>();
	if(!WorldContext && Settings->bPrintFunction) return "(" + CallingFunction + ") " + String;
	
	FString ClassName = WorldContext->GetClass()->GetName();
	ClassName.RemoveFromEnd("_C");
	
	if(Settings->bPrintClass && Settings->bPrintFunction)
	{
		return "(" + ClassName + ") (" + CallingFunction + ") " + String;
	}
	
	if(Settings->bPrintClass)
	{
		return "(" + ClassName + ") " + String;
	}

	if(Settings->bPrintFunction)
	{
		return "(" + CallingFunction + ") " + String;
	}
	
	return String;
}
#pragma endregion 
