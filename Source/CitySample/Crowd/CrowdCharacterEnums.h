// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CrowdCharacterEnums.generated.h"

UENUM(BlueprintType)
enum class ECitySampleCrowdGender : uint8
{
	A,
	B,
};

UENUM(BlueprintType)
enum class ECitySampleCrowdBodyType : uint8
{
	NormalWeight,
	OverWeight,
	UnderWeight
};

// ----------------------------------------------------

FORCEINLINE
FString GenderToString(const ECitySampleCrowdGender& Skeleton)
{
	switch (Skeleton)
	{
		case ECitySampleCrowdGender::B:
			return "B";
		default:
			return "A";
	}
}

FORCEINLINE
FString BodyTypeToString(const ECitySampleCrowdBodyType& BodyType)
{
	switch (BodyType)
	{
		case ECitySampleCrowdBodyType::OverWeight:
			return "OverWeight";
		case ECitySampleCrowdBodyType::UnderWeight:
			return "UnderWeight";
		default:
			return "NormalWeight";
	}
}

UENUM(BlueprintType)
enum class ECrowdHairSlots : uint8
{
	Hair		UMETA(DisplayName = "Hair"),
	Eyebrows	UMETA(DisplayName = "Eyebrows"),
	Fuzz		UMETA(DisplayName = "Fuzz"),
	Eyelashes	UMETA(DisplayName = "Eyelashes"),
	Mustache	UMETA(DisplayName = "Mustache"),
	Beard		UMETA(DisplayName = "Beard"),
	MAX
};

UENUM(BlueprintType)
enum class ECrowdMeshSlots : uint8
{
	Base		UMETA(DisplayName = "Base"),
	Body		UMETA(DisplayName = "Body"),
	Head		UMETA(DisplayName = "Head"),
	Top			UMETA(DisplayName = "Top"),
	Bottom		UMETA(DisplayName = "Bottom"),
	Shoes		UMETA(DisplayName = "Shoes"),
	MAX
};

FORCEINLINE ECrowdMeshSlots GetParentMeshSlot(ECrowdMeshSlots ChildSlot)
{
	switch (ChildSlot)
	{
	case ECrowdMeshSlots::Head:
		return ECrowdMeshSlots::Body;
	default:
		return ECrowdMeshSlots::Base;
	}
}

UENUM(BlueprintType)
enum class ECrowdLineupVariation : uint8
{
	Skeleton,
    BodyType,
    Head,
    Outfit,
    OutfitMaterial,
    Hair,
    Eyebrows,
    Fuzz,
    Eyelashes,
    Mustache,
    Beard,
    HairColor,
    SkinTexture,
    SkinTextureModifier,
    Accessory,
    ScaleFactor,
    PatternColor,
    PatternOption
};

UENUM(BlueprintType)
enum class ECharacterDefinitionType : uint8
{
	Crowd,
	Hero
};

UENUM(BlueprintType)
enum class ECrowdMaterialParameterType : uint8
{
	Color,
	Vector,
	Float
};

UENUM(BlueprintType)
enum class ECrowdPatternUsage : uint8
{
	None,
	PatternList,
	Driven
};

UENUM()
enum EAnimToTextureDataAssetSlots
{
	ATTDAS_Body,
	ATTDAS_Top,
	ATTDAS_Bottom,
	ATTDAS_Shoes,
	ATTDAS_MAX
};