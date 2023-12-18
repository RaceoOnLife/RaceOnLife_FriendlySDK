// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CrowdCharacterEnums.h"
#include "CrowdCharacterDefinition.h"

#include "CrowdCharacterLineupActor.generated.h"

class UCrowdCharacterDataAsset;
class UTextRenderComponent;

USTRUCT()
struct FCrowdLineupInstance
{
	GENERATED_BODY()

	FCrowdLineupInstance() = default;
	FCrowdLineupInstance(AActor* InActor, const FIntVector InCoordinates, const FCrowdCharacterOptions InOptions)
		: LineupActor(InActor)
		, LineupCoordinates(InCoordinates)
		, InstanceOptions(InOptions)
	{
	};

	UPROPERTY()
	TObjectPtr<AActor> LineupActor = nullptr;

	UPROPERTY()
	FIntVector LineupCoordinates = FIntVector::ZeroValue;

	UPROPERTY()
	FCrowdCharacterOptions InstanceOptions;
};

UENUM(BlueprintType)
enum class ECrowdLineupType : uint8
{
	Variation,
	Random
};

USTRUCT(BlueprintType)
struct FCrowdLineupVariationOptions
{
	GENERATED_BODY()

	FCrowdLineupVariationOptions()
		: bX(false)
		, X_Variation(ECrowdLineupVariation::Skeleton)
		, bY(true)
		, Y_Variation(ECrowdLineupVariation::Outfit)
		, bZ(true)
		, Z_Variation(ECrowdLineupVariation::OutfitMaterial)
	{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle))
	bool bX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bX"))
	ECrowdLineupVariation X_Variation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle))
	bool bY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bY"))
	ECrowdLineupVariation Y_Variation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle))
	bool bZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bZ"))
	ECrowdLineupVariation Z_Variation;
};

USTRUCT(BlueprintType)
struct FCrowdLineupRandomOptions
{
	GENERATED_BODY()

	FCrowdLineupRandomOptions()
		: LineupSize(1, 1, 1)
	{};

	// Specify the number of characters to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector LineupSize;

	// The set of properties which will remain fixed when randomizing
	// Represented as an Array due to issues with how sets of enums display in editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ECrowdLineupVariation> FixedOptions;
};

UCLASS(autoExpandCategories=(VariationOptions, RandomOptions))
class CITYSAMPLE_API ACrowdCharacterLineup : public AActor
{
	GENERATED_BODY()

private:
	struct FCrowdVariationSpecifier
	{
		bool bEnabled;
		EAxis::Type TargetAxis;
		ECrowdLineupVariation Variation;

		int CurrentValue;

		FCrowdVariationSpecifier() = default;
		FCrowdVariationSpecifier(const bool bInEnabled, const EAxis::Type InTargetAxis, const ECrowdLineupVariation InVariation, const int InValue = 0)
			: bEnabled(bInEnabled)
			, TargetAxis(InTargetAxis)
			, Variation(InVariation)
			, CurrentValue(InValue)
		{

		};

		FIntVector GetOffsetDirection(int32 Scale = 1) const
		{
			switch (TargetAxis)
			{
			case EAxis::X:
				return FIntVector(Scale, 0, 0);
			case EAxis::Y:
				return FIntVector(0, Scale, 0);
			case EAxis::Z:
				return FIntVector(0, 0, Scale);
			default:
			{
				checkNoEntry();
				return FIntVector();
			}
			}
		}
	};
	
public:

	ACrowdCharacterLineup();

	virtual void Destroyed() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostEditMove(bool bFinished) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Lineup", CallInEditor)
	void BuildLineup();

	UFUNCTION(BlueprintCallable, Category = "Lineup", CallInEditor)
	void ClearLineup();

	UFUNCTION(BlueprintCallable, Category = "Lineup", CallInEditor)
	void UpdateLineup();

	UFUNCTION(BlueprintImplementableEvent, Category = "Lineup")
	AActor* SpawnLineupActor(const FIntVector SpawnCoordinates, const FString& Label, FVector SpawnLocation, FRotator SpawnRotation,
		const FCrowdCharacterDefinition CharacterDefinition, const FCrowdCharacterOptions CharacterOptions);

	UFUNCTION(BlueprintImplementableEvent, Category = "Lineup")
	void UpdateLineupActor(AActor* TargetActor, const FIntVector SpawnCoordinates, const FCrowdCharacterDefinition CharacterDefinition);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup")
	UCrowdCharacterDataAsset* CharacterDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category="Lineup")
	FVector Spacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Lineup")
	FVector LocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Lineup")
	FRotator RotationOffset;

	UPROPERTY(EditAnywhere, Category = "Lineup")
	float RowLabelOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup")
	ECrowdLineupType LineupType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta=(EditCondition="LineupType==ECrowdLineupType::Variation", EditConditionHides))
	FCrowdLineupVariationOptions VariationOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta = (EditCondition = "LineupType==ECrowdLineupType::Random", EditConditionHides))
	FCrowdLineupRandomOptions RandomOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta=(ShowOnlyInnerProperties))
	FCrowdCharacterOptions BaseOptions;

private:
	void SpawnCharacterFromOptions(const FString VariantLabel, const FIntVector LineupCoordinates, const FCrowdCharacterOptions& CharacterOptions);

	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> RowLabels;

	void ResolveVariationDependencies(FCrowdVariationSpecifier& FirstVariation, FCrowdVariationSpecifier& SecondVariation, FCrowdVariationSpecifier& ThirdVariation);

	static bool GetParentVariation(ECrowdLineupVariation ChildVariation, ECrowdLineupVariation& ParentVariation);
	static bool IsDependant(ECrowdLineupVariation FirstVariation, ECrowdLineupVariation SecondVariation);

	int GetNumberOfVariants(const FCrowdCharacterOptions& CharacterOptions, ECrowdLineupVariation VariantType) const;

	void PopulateCharacterDefinition(const FCrowdCharacterOptions& CharacterOptions, FCrowdCharacterDefinition& CharacterDefinition);

	int GetCharacterOptionsEntry(const FCrowdCharacterOptions& CharacterOptions, const ECrowdLineupVariation VariationType);
	void SetCharacterOptionsEntry(FCrowdCharacterOptions& CharacterOptions, const ECrowdLineupVariation VariationType, const int VariantValue);

	void UpdateInstanceTransforms();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneComponent;

	UPROPERTY()
	TArray<FCrowdLineupInstance> LineupInstances;

};