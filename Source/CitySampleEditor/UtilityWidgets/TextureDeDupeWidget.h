// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"

#include "TextureDeDupeWidget.generated.h"

UCLASS(BlueprintType)
class UTextureDeDupeWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void FindDuplicateNamedTextures(bool bMustBeInCook = true);

	UFUNCTION(BlueprintCallable)
	void FindDuplicateCRCTextures(bool bMustBeInCook = true);
};