// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"

#include "MassActorPoolableInterface.h"

#include "Animation/MassCrowdAnimInstance.h"
#include "IMassCrowdActor.h"

#include "Character/CitySampleCharacter.h"
#include "CrowdCharacterDefinition.h"

#include "CrowdCharacterActor.generated.h"

class UCrowdCharacterDataAsset;
class UGroomComponent;
class ULODSyncComponent;
class UStaticMeshComponent;
class UMassAgentComponent;
enum class ECrowdMeshSlots : uint8;
enum class ECrowdHairSlots : uint8;
enum class ESyncOption : uint8;

struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCrowdCharacterEvent, FCrowdOutfitDefinition, CurrentOutfit);

UCLASS(DefaultToInstanced)
class CITYSAMPLE_API ACitySampleCrowdCharacter : public ACitySampleCharacter, public IMassCrowdActorInterface, public IMassActorPoolableInterface
{
	GENERATED_BODY()

public:

	ACitySampleCrowdCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void PostInitProperties() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;

public:
	// Uses the currently set options for the actor to build the corresponding character
	UFUNCTION(BlueprintCallable, Category = "Crowd")
	void BuildCharacter();

	UFUNCTION(BlueprintCallable, Category = "Crowd")
	void BuildCharacterFromDefinition(const FCrowdCharacterDefinition& InCharacterDefinition);

	UFUNCTION(BlueprintCallable, Category = "Crowd")
	void BuildCharacterFromID(const FCrowdVisualizationID& InVisualizationID);

	UFUNCTION(BlueprintCallable, Category="Character")
	USkeletalMeshComponent* GetSkeletalMeshComponentForSlot(ECrowdMeshSlots BodySlot);

	UFUNCTION(BlueprintCallable, Category = "Character")
	UGroomComponent* GetGroomComponentForSlot(ECrowdHairSlots HairSlot);

	UFUNCTION(BlueprintCallable, Category = "Character")
	UStaticMeshComponent* GetAccessoryMeshComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character", Interp)
	TObjectPtr<UCrowdCharacterDataAsset> CrowdCharacterData;

	UPROPERTY(transient, BlueprintReadOnly, Category = "Character")
	TObjectPtr<class UMassCrowdContextualAnimationDataAsset> CurrentContextualAnimDataAsset;

	// Persistent anim data we can pull from during anim's init/spawn
	UPROPERTY(transient)
	FMassCrowdAnimInstanceData SpawnAnimData;

	// Whether to asynchronously load any assets which have not previously been loaded
	UPROPERTY(EditAnywhere, Category="Character", Interp)
	bool bShouldAsyncLoad = true;

	UPROPERTY(EditAnywhere, Category="Character", meta=(DisplayName="Build Character on Contruction"))
	bool bShouldBuildOnConstruct = true;

	UPROPERTY(EditAnywhere, Category = "Character", Interp)
	bool bEnableHair = false;

	// Sets whether hair is enabled for the character.
	// Optionally forces the character to be updated to match the new hair state
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetEnableHair(const bool bNewHairEnabled, const bool bForceUpdateCharacter = false);

	// Determines if cards should be used instead of strands when rendering hair
	UPROPERTY(EditAnywhere, Category = "Character", Interp)
	bool bUseCards = true;

	// The set of properties which will remain fixed when randomizing
	// Represented as an Array due to issues with how sets of enums display in editor
	UPROPERTY(EditAnywhere, Category = "Character", Interp, meta = (EditCondition="CharacterDataType==ECharacterDataType::Crowd", EditConditionHides))
	TArray<ECrowdLineupVariation> RandomFixedOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character", Interp, meta = (EditCondition="CharacterDataType==ECharacterDataType::Crowd", EditConditionHides))
	FCrowdCharacterOptions CharacterOptions;

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Character")
	void Randomize();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Character")
	void RandomizeFromStream(const FRandomStream& RandomStream);

	UFUNCTION(BlueprintPure, Category="Character")
	UDataAsset* GetCurrentLocomotionAnimSet() const;

	UFUNCTION(BlueprintPure, Category="Character")
    UDataAsset* GetCurrentAccessoryAnimSet() const;
	
	virtual void OnGetOrSpawn(FMassEntityManager* EntitySubsystem, const FMassEntityHandle MassAgent) override;

	UFUNCTION()
	void UpdateOffset()
	{
		if (AdditionalIKXOffset != 0.0f)
		{
			FVector RelativeLocation = GetMesh()->GetRelativeLocation();
			RelativeLocation.Z -= AdditionalIKXOffset;

			AdditionalIKXOffset = AdditionalIKXOffset * 0.9f;
			if (FMath::Abs(AdditionalIKXOffset) < 0.1f)
			{
				AdditionalIKXOffset = 0.0f;
			}

			RelativeLocation.Z += AdditionalIKXOffset;
			GetMesh()->SetRelativeLocation(RelativeLocation);

			if (AdditionalIKXOffset != 0.0f)
			{
				GetWorld()->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateUObject(this, &ACitySampleCrowdCharacter::UpdateOffset));
			}
		}
	}

	virtual void SetAdditionalMeshOffset(const float Offset) override
	{
		FVector RelativeLocation = GetMesh()->GetRelativeLocation();
		RelativeLocation.Z -= AdditionalIKXOffset;

		AdditionalIKXOffset = Offset;
		RelativeLocation.Z += AdditionalIKXOffset;
		GetMesh()->SetRelativeLocation(RelativeLocation);

		if (AdditionalIKXOffset != 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &ACitySampleCrowdCharacter::UpdateOffset));
		}
	}

	// Event for when the character has been updated. This will get called whenever the character is either built or rebuilt
	UPROPERTY(BlueprintAssignable)
	FCrowdCharacterEvent OnCharacterUpdated;

#if WITH_EDITORONLY_DATA
	// Used for test data only
	UPROPERTY(EditAnywhere, Category = "Character")
	UAnimToTextureDataAsset* FallbackAnimToTextureDataAsset;
#endif // WITH_EDITORONLY_DATA

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Character")
	void HitByCar(AActor* CarActor);

private:

	// Internal versions of the Build functions to support async loading
	void BuildCharacterFromDefinition_Internal(const FCrowdCharacterDefinition InCharacterDefinition);

	void SetupSkeletalMeshes();
	void SetupGroomComponents();
	void SetupLODSync();

	FCrowdGenderDefinition* GetGenderDefinition() const;
	FCrowdBodyOutfitDefinition* GetBodyOutfitDefinition(FCrowdGenderDefinition* InGenderDefinition = nullptr) const;

	void UpdateGrooms(const TArray<FCrowdHairDefinition>& HairDefinitions);
	void ReattachGrooms(const TArray<FCrowdHairDefinition>& HairDefinitions);
	
	void UpdateMeshes(const FCrowdCharacterDefinition& CharacterDefinition);
	void UpdateContextualAnimData(const FCrowdCharacterDefinition& CharacterDefinition);
	void UpdateMaterials(const FCrowdCharacterDefinition& CharacterDefinition);

	void UpdateHairMaterials();
	void UpdateBodyMaterials();
	void UpdateHeadMaterials();
	void UpdateOutfitMaterials();
	
	USceneComponent* ResolveComponentIdentifier(FCitySampleCharacterComponentIdentifier ComponentIdentifier);

	void UpdateLODSync();

	void UpdateSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComponent, TSoftObjectPtr<USkeletalMesh> SoftSkeletalMeshPtr);

	static ESyncOption GetSyncOptionForSlot(ECrowdMeshSlots MeshSlot);

	FTimerHandle UnhideAccesoryTimerHandle;
	void UnhideAccessory();

	UPROPERTY(Config)
	float UnhideAccessoryTime = 0.15f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UMassAgentComponent> AgentComponent;

private:
	// Array of any additional skeletal meshes this character may have
	// The array indices should match the ordering in ECrowdMeshSlots
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<USkeletalMeshComponent>> AdditionalMeshes;

	// Array of Groom Components
	// If Groom Components are enabled then this array will be set up such that the indices match ECrowdHairSlots 
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UGroomComponent>> GroomComponents;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> AccessoryMeshComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<ULODSyncComponent> LODSyncComponent;

	UPROPERTY()
	FCrowdCharacterDefinition PrivateCharacterDefinition;

	TMap<FString, TSharedPtr<FStreamableHandle>> StreamingHandles;

	TArray<TSoftObjectPtr<USkeletalMesh>> StreamingMeshes;

	void LoadAnimToTextureDataAssets(const FCrowdCharacterDefinition& InCharacterDefinition);
	void AsyncLoadAnimToTextureDataAsset(TSoftObjectPtr<UAnimToTextureDataAsset> Asset, EAnimToTextureDataAssetSlots Index);
	void AnimToTextureDataAssetLoaded(TSoftObjectPtr<UAnimToTextureDataAsset> Asset, EAnimToTextureDataAssetSlots IndexLoaded);
	bool bLoadingAnimToTextureDataAssets = false;
	bool bGroomComponentAttachmentDelayed = false;

	UPROPERTY(Transient)
	UAnimToTextureDataAsset* AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_MAX];

	TSharedPtr<FStreamableHandle> AnimToTextureDataAssetsStreamingHandles[EAnimToTextureDataAssetSlots::ATTDAS_MAX];

	float AdditionalIKXOffset = 0.0f;
};