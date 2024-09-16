// Copyright 2022 Just2Devs. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DFLDebugFL.generated.h"

UCLASS()
class DEBUGFUNCTIONLIBRARYLITE_API UDFLDebugFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#pragma region Print
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickPrintLogMessage(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickPrintLogWarning(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickPrintLogError(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickPrintMessage(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLPQuickPrintWarning(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickPrintError(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickLogMessage(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickLogWarning(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "Debug Function Libraray | Print Debug", meta = (DefaultToSelf = "WorldContext", WorldContext = "WorldContext", CallableWithoutWorldContext))
	static void DFLQuickLogError(UObject* WorldContext, FString String, float Duration = 2, FString CallingFunction = "AMBIGUOUS");
	
	static FString GetQuickPrintString(UObject* WorldContext, FString String, FString CallingFunction);
	static FString GetQuickLogString(UObject* WorldContext, FString String, FString CallingFunction);
#pragma endregion
};

