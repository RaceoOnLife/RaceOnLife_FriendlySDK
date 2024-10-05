

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"
#include "UObject/NoExportTypes.h"
#include "lib_types.generated.h"

USTRUCT(BlueprintType)
struct FMusicStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	USoundWave* MusicReference;

	UPROPERTY(BlueprintReadWrite)
	FString MusicName;

	UPROPERTY(BlueprintReadWrite)
	float MusicLength;

	UPROPERTY(BlueprintReadWrite)
	UTexture2D* MusicImage;

	FMusicStruct() : MusicReference(nullptr), MusicName(""), MusicLength(0.f), MusicImage(nullptr)
	{
	}

	bool operator==(const FMusicStruct& Other) const
	{
		return MusicName == Other.MusicName;
	}
};

class RACEONLIFE_LIB_API lib_types
{
public:
	lib_types();
	~lib_types();


};
