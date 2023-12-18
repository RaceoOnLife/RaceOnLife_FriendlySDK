// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CitySampleTypesUI.generated.h"

UENUM(BlueprintType)
enum class ECitySampleControlsFlavor : uint8
{
	Keyboard,
	Gamepad
};

UENUM(BlueprintType)
enum class ECitySamplePromptTextType : uint8
{
	Center,
	Top,
	Left,
	Right,
	Bottom
};

ENUM_RANGE_BY_FIRST_AND_LAST(ECitySamplePromptTextType, ECitySamplePromptTextType::Center, ECitySamplePromptTextType::Bottom)

USTRUCT(BlueprintType)
struct FCitySampleTextConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCollapseWhenEmpty = true;

	// #todo: add BP callable function to BP library
	void Configure(class UTextBlock* TextBlock) const;
};

USTRUCT(BlueprintType)
struct FCitySampleImageConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush Brush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HighlightColor = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCitySampleTextConfig LabelTextConfig;

	// #todo: add BP callable function to BP library
	void Configure(class UImage* Image, class UTextBlock* LabelText=nullptr) const;
};