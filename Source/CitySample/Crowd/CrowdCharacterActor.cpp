// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdCharacterActor.h"
#include "CrowdBlueprintLibrary.h"
#include "CrowdCharacterDataAsset.h"
#include "CrowdCharacterEnums.h"
#include "CrowdVisualizationFragment.h"
#include "GroomComponent.h"
#include "MassEntityManager.h"
#include "Components/LODSyncComponent.h"
#include "Components/CapsuleComponent.h"
#include "MassAgentComponent.h"
#include "Crowd/CitySampleCrowdSettings.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "MassMovementFragments.h"
#include "Animation/MassCrowdContextualAnimTypes.h"
#include "MassCommonFragments.h"
#include "MassCrowdAnimationTypes.h"
#include "MassRepresentationFragments.h"
#include "MassLookAtFragments.h"
#include "TimerManager.h"
#include "CitySample/CitySample.h"

extern TAutoConsoleVariable<int32> CVarCrowdMinLOD;

ACitySampleCrowdCharacter::ACitySampleCrowdCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = nullptr;

	// Adjust Capsule dimensions
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCapsuleHalfHeight(88.f);
		Capsule->SetCapsuleRadius(34.f);
	}

	AgentComponent = CreateDefaultSubobject<UMassAgentComponent>(TEXT("MassAgent"));

	SetupSkeletalMeshes();
	SetupLODSync();
	SetupGroomComponents();
}

void ACitySampleCrowdCharacter::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject) && ensure(AgentComponent))
	{
		if (const UCitySampleCrowdSettings* Settings = UCitySampleCrowdSettings::Get())
		{
			if (UMassEntityConfigAsset* AssetInstance = Settings->GetAgentConfig())
			{
				AgentComponent->SetEntityConfig(FMassEntityConfig(*AssetInstance));
			}
		}

		// Since it is the anim BP driving the orientation of the character on CitySample and is not executed on the server, 
		// we need to turn on orient towards acceleration on for replicated worlds 
		if (UWorld* World = GetWorld())
		{
			if (World->GetNetMode() != NM_Standalone)
			{
				if (UCharacterMovementComponent* CharMovement = GetCharacterMovement())
				{
					CharMovement->bOrientRotationToMovement = true;
				}
			}
		}
	}
}

void ACitySampleCrowdCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!HasAnyFlags(RF_ClassDefaultObject) && bShouldBuildOnConstruct)
	{
		BuildCharacter();
	}
}

#if WITH_EDITOR
void ACitySampleCrowdCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (FProperty* MemberProperty = PropertyChangedEvent.MemberProperty)
	{
		const FName MemberPropertyName = MemberProperty->GetFName();

		if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ACitySampleCrowdCharacter, CharacterOptions) ||
			MemberPropertyName == GET_MEMBER_NAME_CHECKED(ACitySampleCrowdCharacter, CrowdCharacterData) ||
			MemberPropertyName == GET_MEMBER_NAME_CHECKED(ACitySampleCrowdCharacter, bEnableHair) ||
			MemberPropertyName == GET_MEMBER_NAME_CHECKED(ACitySampleCrowdCharacter, bUseCards))				
		{
			BuildCharacter();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ACitySampleCrowdCharacter::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	USkeletalMeshComponent* BaseMesh = GetMesh();
	if (IsValid(BaseMesh))
	{
		BaseMesh->SetSkeletalMesh(nullptr);
		for (int MaterialIdx = 0; MaterialIdx < BaseMesh->GetNumMaterials(); ++MaterialIdx)
		{
			BaseMesh->SetMaterial(MaterialIdx, nullptr);
		}
		BaseMesh->EmptyOverrideMaterials();
	}
	
	for (USkeletalMeshComponent* SkelMeshComponent : AdditionalMeshes)
	{
		if (IsValid(SkelMeshComponent))
		{
			SkelMeshComponent->SetSkeletalMesh(nullptr);
			for (int MaterialIdx = 0; MaterialIdx < SkelMeshComponent->GetNumMaterials(); ++MaterialIdx)
			{
				SkelMeshComponent->SetMaterial(MaterialIdx, nullptr);
			}
			SkelMeshComponent->EmptyOverrideMaterials();
		}
	}

	for (UGroomComponent* GroomComponent : GroomComponents)
	{
		if (IsValid(GroomComponent))
		{
			GroomComponent->SetGroomAsset(nullptr, nullptr);
			for (int MaterialIdx = 0; MaterialIdx < GroomComponent->GetNumMaterials(); ++MaterialIdx)
			{
				GroomComponent->SetMaterial(MaterialIdx, nullptr);
			}
			GroomComponent->EmptyOverrideMaterials();
		}
	}

	if (IsValid(AccessoryMeshComponent))
	{
		AccessoryMeshComponent->SetStaticMesh(nullptr);
	}
	
	Super::PreSave(ObjectSaveContext);
}

void ACitySampleCrowdCharacter::BuildCharacter()
{	
	FCrowdCharacterDefinition CharacterDefinition;
	CharacterOptions.GenerateCharacterDefinition(CrowdCharacterData, CharacterDefinition);
	BuildCharacterFromDefinition(CharacterDefinition);
}

void ACitySampleCrowdCharacter::BuildCharacterFromDefinition(const FCrowdCharacterDefinition& InCharacterDefinition)
{
	const TArray<FSoftObjectPath> AssetsToLoad = InCharacterDefinition.GetSoftPathsToLoad();
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	bool bAllAssetsLoaded = Algo::AllOf(AssetsToLoad, [](const FSoftObjectPath& AssetToLoad) {
		return (AssetToLoad.ResolveObject() != nullptr);
	});

	// Get a streaming handle and clear it if we're currently loading
	TSharedPtr<FStreamableHandle> BuildStreamingHandle = StreamingHandles.FindOrAdd(TEXT("Build"));
	if (BuildStreamingHandle.IsValid() && BuildStreamingHandle->IsActive())
	{
		BuildStreamingHandle->CancelHandle();
	}
	
	if (bAllAssetsLoaded)
	{
		BuildCharacterFromDefinition_Internal(InCharacterDefinition);
	}
	else if (bShouldAsyncLoad)
	{
		BuildStreamingHandle = StreamableManager.RequestAsyncLoad(AssetsToLoad, FStreamableDelegate::CreateUObject(this, &ThisClass::BuildCharacterFromDefinition_Internal, InCharacterDefinition));
	}
	else
	{
		BuildStreamingHandle = StreamableManager.RequestSyncLoad(AssetsToLoad);
		BuildCharacterFromDefinition_Internal(InCharacterDefinition);
	}
}

void ACitySampleCrowdCharacter::BuildCharacterFromDefinition_Internal(const FCrowdCharacterDefinition InCharacterDefinition)
{
	// From this point forward everything is synchronous with the assumption that if we are async loading it would
    // be complete by the time we reach here

	// Cache the definition so we can access it later
    PrivateCharacterDefinition = InCharacterDefinition;

    LoadAnimToTextureDataAssets(InCharacterDefinition);

    UpdateMeshes(InCharacterDefinition);
    UpdateGrooms(InCharacterDefinition.HairDefinitions);
    UpdateMaterials(InCharacterDefinition);
    
    UpdateContextualAnimData(InCharacterDefinition);
    UpdateLODSync();

    // Anim Instance is not automatically reinitialized which can lead to garbage poses
    // so we manually force the initialize to prevent this.
    if (USkeletalMeshComponent* BaseMesh = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Base))
    {
    	BaseMesh->InitAnim(true);
    }

    {
    	FEditorScriptExecutionGuard ScriptExecutionGuard;

    	OnCharacterUpdated.Broadcast(InCharacterDefinition.OutfitDefinition);
    }
}

void ACitySampleCrowdCharacter::BuildCharacterFromID(const FCrowdVisualizationID& InVisualizationID)
{
	CharacterOptions = InVisualizationID.ToCharacterOptions();
	if (CrowdCharacterData)
	{
		CharacterOptions.GenerateCharacterDefinition(CrowdCharacterData, PrivateCharacterDefinition);
		BuildCharacterFromDefinition(PrivateCharacterDefinition);
	}
}

void ACitySampleCrowdCharacter::SetupSkeletalMeshes()
{
	USkeletalMeshComponent* BaseMeshComponent = GetMesh();

	if (BaseMeshComponent)
	{
		// Lower the base mesh and rotate it to align with capsule component
		BaseMeshComponent->SetRelativeTransform(FTransform(
			FRotator(0, 90, 0),
			FVector(0, 0, -90)
		));
	}

	// The first mesh slot is the base mesh from CitySample Character so we only need to care about meshes in excess of this
	uint8 NumberOfAdditionalMeshes = static_cast<uint8>(ECrowdMeshSlots::MAX) - 1;
	AdditionalMeshes.SetNumUninitialized(NumberOfAdditionalMeshes);

	UEnum* StaticMeshSlotsEnum = StaticEnum<ECrowdMeshSlots>();
	check(StaticMeshSlotsEnum);

	for (int MeshIdx = 0; MeshIdx < NumberOfAdditionalMeshes; ++MeshIdx)
	{
		int SlotIdx = MeshIdx + 1;
		ECrowdMeshSlots MeshSlot = ECrowdMeshSlots(SlotIdx);

		FName MeshName(FString("Additional_") + StaticMeshSlotsEnum->GetNameStringByIndex(SlotIdx));
		AdditionalMeshes[MeshIdx] = CreateDefaultSubobject<USkeletalMeshComponent>(MeshName);
		if (USkeletalMeshComponent* MeshComponent = AdditionalMeshes[MeshIdx])
		{
			// Find the parent slot for our current mesh slot
			ECrowdMeshSlots ParentSlot = GetParentMeshSlot(ECrowdMeshSlots(SlotIdx));

			// We guarantee that the parent slot will appear before the child slot 
			// which ensures the parent component will be created by the time we need it
			MeshComponent->SetupAttachment(GetSkeletalMeshComponentForSlot(ParentSlot));
		}
	}

	// Setup the accessory component and attach to the body
	AccessoryMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Accessory"));
}

void ACitySampleCrowdCharacter::SetupGroomComponents()
{
	static ECrowdMeshSlots GroomAttachSlot = ECrowdMeshSlots::Head;

	USkeletalMeshComponent* GroomAttachMesh = GetSkeletalMeshComponentForSlot(GroomAttachSlot);
	if (!ensureAlwaysMsgf(GroomAttachMesh, TEXT("Unable to create Groom Components for character %s. Attachment Mesh was not valid."), *this->GetName()))
	{
		return;
	}

	uint8 NumberOfGrooms = static_cast<uint8>(ECrowdHairSlots::MAX);
	GroomComponents.SetNumUninitialized(NumberOfGrooms);

	UEnum* StaticHairSlotsEnum = StaticEnum<ECrowdHairSlots>();
	check(StaticHairSlotsEnum);

	for (int GroomIdx = 0; GroomIdx < NumberOfGrooms; ++GroomIdx)
	{
		FName GroomName(FString("Groom_") + StaticHairSlotsEnum->GetNameStringByIndex(GroomIdx));

		GroomComponents[GroomIdx] = CreateDefaultSubobject<UGroomComponent>(GroomName);

		if (GroomComponents[GroomIdx])
		{
			GroomComponents[GroomIdx]->SetupAttachment(GroomAttachMesh);
			GroomComponents[GroomIdx]->AttachmentName = "head";
		}
	}
}

void ACitySampleCrowdCharacter::SetupLODSync()
{
	LODSyncComponent = CreateDefaultSubobject <ULODSyncComponent>(TEXT("LOD Sync"));

	// Currently all custom mappings share the same mapping data
	static FLODMappingData CustomLODMappingData;
	CustomLODMappingData.Mapping = { 0, 0, 1, 1, 2, 2, 3, 3 };

	if (LODSyncComponent)
	{
		LODSyncComponent->NumLODs = 8;
		
		for (int MeshIdx = 0; MeshIdx < AdditionalMeshes.Num(); ++MeshIdx)
		{
			// Add 1 here as we don't want to use the base mesh
			ECrowdMeshSlots MeshSlot = ECrowdMeshSlots(MeshIdx + 1);
			USkeletalMeshComponent* MeshComponent = GetSkeletalMeshComponentForSlot(MeshSlot);
			if (MeshComponent)
			{
				FName ComponentName = MeshComponent->GetFName();
				LODSyncComponent->ComponentsToSync.Emplace(ComponentName, GetSyncOptionForSlot(MeshSlot));

				// Add a custom LOD mapping for certain slots
				if (MeshSlot == ECrowdMeshSlots::Body || MeshSlot == ECrowdMeshSlots::Top || MeshSlot == ECrowdMeshSlots::Bottom || MeshSlot == ECrowdMeshSlots::Shoes)
				{
					LODSyncComponent->CustomLODMapping.Add(ComponentName, CustomLODMappingData);
				}
			}
		}

		LODSyncComponent->RefreshSyncComponents();
	}
}

void ACitySampleCrowdCharacter::AsyncLoadAnimToTextureDataAsset(TSoftObjectPtr<UAnimToTextureDataAsset> Asset, EAnimToTextureDataAssetSlots Index)
{
	if (!Asset.IsValid())
	{
		if (!Asset.IsNull())
		{
			FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
			StreamableManager.RequestAsyncLoad(Asset.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::AnimToTextureDataAssetLoaded, Asset, Index));
			bLoadingAnimToTextureDataAssets = true;
		}
	}
	else
	{
		// Asset is already in memory
		AnimToTextureDataAssets[Index] = Asset.Get();
	}
}

void ACitySampleCrowdCharacter::LoadAnimToTextureDataAssets(const FCrowdCharacterDefinition& InCharacterDefinition)
{
	bLoadingAnimToTextureDataAssets = false;

	for (int i = 0; i < EAnimToTextureDataAssetSlots::ATTDAS_MAX; i++)
	{
		if (AnimToTextureDataAssetsStreamingHandles[i].IsValid() && AnimToTextureDataAssetsStreamingHandles[i]->IsActive())
		{
			AnimToTextureDataAssetsStreamingHandles[i]->CancelHandle();
		}
		AnimToTextureDataAssets[i] = nullptr;
	}

	AsyncLoadAnimToTextureDataAsset(InCharacterDefinition.BodyDefinition.BodyData, EAnimToTextureDataAssetSlots::ATTDAS_Body);
	AsyncLoadAnimToTextureDataAsset(InCharacterDefinition.OutfitDefinition.TopData, EAnimToTextureDataAssetSlots::ATTDAS_Top);
	AsyncLoadAnimToTextureDataAsset(InCharacterDefinition.OutfitDefinition.BottomData, EAnimToTextureDataAssetSlots::ATTDAS_Bottom);
	AsyncLoadAnimToTextureDataAsset(InCharacterDefinition.OutfitDefinition.ShoesData, EAnimToTextureDataAssetSlots::ATTDAS_Shoes);
}

void ACitySampleCrowdCharacter::AnimToTextureDataAssetLoaded(TSoftObjectPtr<UAnimToTextureDataAsset> Asset, EAnimToTextureDataAssetSlots IndexLoaded)
{
	AnimToTextureDataAssets[IndexLoaded] = Asset.Get();

	for (int i = 0; i < EAnimToTextureDataAssetSlots::ATTDAS_MAX; i++)
	{
		if (!AnimToTextureDataAssets[i])
		{
			return;
		}
	}

	bLoadingAnimToTextureDataAssets = false;

	UpdateMeshes(PrivateCharacterDefinition);
	UpdateGrooms(PrivateCharacterDefinition.HairDefinitions);
	UpdateMaterials(PrivateCharacterDefinition);
}

void ACitySampleCrowdCharacter::UpdateMeshes(const FCrowdCharacterDefinition& CharacterDefinition)
{
	if (bLoadingAnimToTextureDataAssets)
	{
		return;
	}

	TArray<TSoftObjectPtr<USkeletalMesh>> CharacterMeshes = {
		CharacterDefinition.BodyDefinition.Base,
		AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Body] ? AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Body]->SkeletalMesh : nullptr,
		CharacterDefinition.Head,
		AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Top] ? AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Top]->SkeletalMesh : nullptr,
		AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Bottom] ? AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Bottom]->SkeletalMesh : nullptr,
		AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Shoes] ? AnimToTextureDataAssets[EAnimToTextureDataAssetSlots::ATTDAS_Shoes]->SkeletalMesh : nullptr
	};
	
	// Update the Skeletal Meshes
	USkeletalMeshComponent* BaseMeshComponent = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Base);
	
	for (uint8 MeshIdx = 0; MeshIdx < static_cast<uint8>(ECrowdMeshSlots::MAX); ++MeshIdx)
	{
		const ECrowdMeshSlots MeshSlot = ECrowdMeshSlots(MeshIdx);
		USkeletalMeshComponent* MeshComponent = GetSkeletalMeshComponentForSlot(MeshSlot);
		
		if (MeshComponent && CharacterMeshes.IsValidIndex(MeshIdx))
		{
			UpdateSkeletalMesh(MeshComponent, CharacterMeshes[MeshIdx]);

			// Scale the Mesh if it's the base Mesh
			if (MeshIdx == static_cast<uint8>(ECrowdMeshSlots::Base))
			{
				MeshComponent->SetRelativeScale3D(FVector(CharacterDefinition.ScaleFactor));
			}

			// Update Leader Pose Component
			// 
			// We dont want a leader pose on the base or the head
			if (MeshSlot == ECrowdMeshSlots::Base || MeshSlot == ECrowdMeshSlots::Head)
			{
				// There is a bug where the leader pose is getting serialized, let's set this to null just in case.
				MeshComponent->SetLeaderPoseComponent(nullptr);
			}
			else
			{
				// Only set the leader pose component if we have a valid mesh on the base mesh
				if (BaseMeshComponent && IsValid(BaseMeshComponent->GetSkeletalMeshAsset()))
				{
					MeshComponent->SetLeaderPoseComponent(BaseMeshComponent);
				}
				else
				{
					MeshComponent->SetLeaderPoseComponent(nullptr);	
				}
			}
		}
	}

	// Update the Accessory
	if (AccessoryMeshComponent)
	{
		const TSoftObjectPtr<UStaticMesh> SoftAccessoryMeshPtr = CharacterDefinition.AccessoryDefinition.Mesh;
		if (SoftAccessoryMeshPtr.IsNull())
		{
			AccessoryMeshComponent->SetStaticMesh(nullptr);
			AccessoryMeshComponent->EmptyOverrideMaterials();
		}
		else
		{
			// Load Synchronous resolves to a Get if the mesh is already loaded
			AccessoryMeshComponent->SetStaticMesh(SoftAccessoryMeshPtr.LoadSynchronous());
			AccessoryMeshComponent->EmptyOverrideMaterials();
		}
		AccessoryMeshComponent->SetHiddenInGame(true);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(UnhideAccesoryTimerHandle, this, &ThisClass::UnhideAccessory, UnhideAccessoryTime, false);
		}
	}

	UCitySampleCrowdFunctionLibrary::AttachAccessory(CharacterDefinition, BaseMeshComponent, AccessoryMeshComponent);
}

void ACitySampleCrowdCharacter::UnhideAccessory()
{
	if (AccessoryMeshComponent)
	{
		AccessoryMeshComponent->SetHiddenInGame(false);
	}
}

void ACitySampleCrowdCharacter::UpdateContextualAnimData(const FCrowdCharacterDefinition& CharacterDefinition)
{
	CurrentContextualAnimDataAsset = CharacterDefinition.ContextualAnimDataAsset.LoadSynchronous();
}

void ACitySampleCrowdCharacter::UpdateGrooms(const TArray<FCrowdHairDefinition>& HairDefinitions)
{
	// Get Groom AttachMesh
	// this will be used for attaching to component later.
	USkeletalMeshComponent* GroomAttachMesh = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Head);

	if (ensure(HairDefinitions.Num() <= GroomComponents.Num()))
	{
		for (int HairIdx = 0; HairIdx < HairDefinitions.Num(); ++HairIdx)
		{
			if (UGroomComponent* GroomComponent = GroomComponents[HairIdx])
			{
				const FCrowdHairDefinition& HairDefinition = HairDefinitions[HairIdx];

				GroomComponent->SetUseCards(bUseCards);

				GroomComponent->SimulationSettings.SimulationSetup.bLocalSimulation = HairDefinition.bLocalSimulation;
				GroomComponent->SimulationSettings.SimulationSetup.LocalBone = HairDefinition.LocalBone;

				if (bEnableHair)
				{
					GroomComponent->SetGroomAsset(HairDefinition.Groom.LoadSynchronous(), HairDefinition.GroomBinding.LoadSynchronous());
					GroomComponent->PhysicsAsset = HairDefinition.PhysicsAsset.LoadSynchronous();
					
					// GroomComponent will not recalculate offset if AttachSocketName == AttachmentName
					// This will reset the AttachSocketName so offset will be recalculated in GroomComponent:TickComponent
					if (GroomAttachMesh)
					{
						GroomComponent->AttachToComponent(GroomAttachMesh, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false), NAME_None);
					}
					else
					{
						bGroomComponentAttachmentDelayed = true;
					}
				}
				else
				{
					GroomComponent->SetGroomAsset(nullptr, nullptr);
					GroomComponent->PhysicsAsset = nullptr;
				}
			}

		}
	}
}

void ACitySampleCrowdCharacter::ReattachGrooms(const TArray<FCrowdHairDefinition>& HairDefinitions)
{
	bool bFailedToAttachGroom = false;
	if (USkeletalMeshComponent* GroomAttachMesh = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Head))
	{
		for (int HairIdx = 0; HairIdx < HairDefinitions.Num(); ++HairIdx)
		{
			if (UGroomComponent* GroomComponent = GroomComponents[HairIdx])
			{
				if (GroomAttachMesh)
				{
					GroomComponent->AttachToComponent(GroomAttachMesh, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false), NAME_None);
				}
				else
				{
					bFailedToAttachGroom = true;
				}
			}
		}
	}

	if (!bFailedToAttachGroom)
	{
		bGroomComponentAttachmentDelayed = false;
	}
}

void ACitySampleCrowdCharacter::UpdateMaterials(const FCrowdCharacterDefinition& CharacterDefinition)
{
	UpdateHairMaterials();
	UpdateOutfitMaterials();
	UpdateBodyMaterials();
	UpdateHeadMaterials();
}

void ACitySampleCrowdCharacter::UpdateHairMaterials()
{
	static FName MelaninParameterName("hairMelanin");
	static FName RednessParameterName("hairRedness");
	static FName RoughnessParameterName("HairRoughness");

	for (UGroomComponent* GroomComponent : GroomComponents)
	{
		UCitySampleCrowdFunctionLibrary::SetHairMaterials(PrivateCharacterDefinition, GroomComponent,
			MelaninParameterName, RednessParameterName, RoughnessParameterName);
	}
}

void ACitySampleCrowdCharacter::UpdateBodyMaterials()
{
	static FName MaterialSlotName("lambert1");
	static FName BodyParameterName("Color_MAIN");

	USkeletalMeshComponent* BodyMeshComponent = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Body);
	USkeletalMeshComponent* BottomMeshComponent = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Bottom);
	TSoftObjectPtr<UTexture2D> SoftBodyTexturePtr = PrivateCharacterDefinition.SkinTextureDefinition.BodyColor;

	UCitySampleCrowdFunctionLibrary::SetBodyMaterials(PrivateCharacterDefinition, BodyMeshComponent, MaterialSlotName, BodyParameterName, BottomMeshComponent);
}

void ACitySampleCrowdCharacter::UpdateHeadMaterials()
{
	static FName FaceParameterName("Color_MAIN");
	static FName ChestParameterName("Color_CHEST");

	static TArray<FName> MaterialSlots = {
		"head_shader_shader",
		"head_LOD1_shader_shader",
		"head_LOD2_shader_shader",
		"head_LOD3_shader_shader",
		"head_LOD4_shader_shader",
		"head_LOD57_shader_shader"
	};

	USkeletalMeshComponent* HeadMeshComponent = GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Head);

	// The first LOD which should use baked groom maps
	static const int BakedGroomMinimumLOD = 2;

	// Current LOD level
	int LODIndex = 0;
	for (const FName& MaterialSlot : MaterialSlots)
	{
		UCitySampleCrowdFunctionLibrary::SetHeadMaterials(PrivateCharacterDefinition, HeadMeshComponent, MaterialSlot, FaceParameterName, ChestParameterName, LODIndex);

		LODIndex += 1;
	}
}

void ACitySampleCrowdCharacter::UpdateOutfitMaterials()
{
	UCitySampleCrowdFunctionLibrary::SetOutfitMaterials(PrivateCharacterDefinition,
		GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Top), 
		GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Bottom), 
		GetSkeletalMeshComponentForSlot(ECrowdMeshSlots::Shoes),
		GetAccessoryMeshComponent()
	);
}

USceneComponent* ACitySampleCrowdCharacter::ResolveComponentIdentifier(const FCitySampleCharacterComponentIdentifier ComponentIdentifier) 
{
	if (ComponentIdentifier.ComponentType == ECitySampleCharacterComponentType::Groom)
	{
		return GetGroomComponentForSlot(ComponentIdentifier.GroomSlot);
	}
	else
	{
		return GetSkeletalMeshComponentForSlot(ComponentIdentifier.MeshSlot);
	}
}

void ACitySampleCrowdCharacter::UpdateLODSync()
{
	if (!LODSyncComponent)
	{
		return;
	}
	
	// Currently all custom mappings share the same mapping data
	static FLODMappingData CustomLODMappingData;
	CustomLODMappingData.Mapping = { 0, 0, 1, 1, 2, 2, 3, 3 };

	if (CrowdCharacterData)
	{
		// Copy over LOD settings
		LODSyncComponent->NumLODs = CrowdCharacterData->NumLODs;

		// Allow overrides to forced LOD
		LODSyncComponent->ForcedLOD = CrowdCharacterData->ForcedLOD;

		// Set the minimum LOD from the CVar
		LODSyncComponent->MinLOD = CVarCrowdMinLOD.GetValueOnAnyThread();

		LODSyncComponent->ComponentsToSync.Empty();
		LODSyncComponent->CustomLODMapping.Empty();

		for (FCitySampleComponentSync& CitySampleComponentSync : CrowdCharacterData->ComponentsToSync)
		{
			USceneComponent* Component = ResolveComponentIdentifier(CitySampleComponentSync.ComponentIdentifier);
			if (Component)
			{
				FName ComponentName = Component->GetFName();
				LODSyncComponent->ComponentsToSync.Emplace(ComponentName, CitySampleComponentSync.SyncOption);
			}
		}

		for (TPair<FCitySampleCharacterComponentIdentifier, FLODMappingData>& CustomMapping : CrowdCharacterData->CustomLODMapping)
		{
			const FCitySampleCharacterComponentIdentifier& ComponentIdentifier = CustomMapping.Key;
			USceneComponent* Component = ResolveComponentIdentifier(ComponentIdentifier);
			if (Component)
			{
				FName ComponentName = Component->GetFName();
				LODSyncComponent->CustomLODMapping.Add(ComponentName, CustomMapping.Value);
			}
		}

		LODSyncComponent->RefreshSyncComponents();
	}
}

// Assume Mesh Component has already been validated
// Handle Index < 0 implies we skip any extra loading as we've already been loaded elsewhere
void ACitySampleCrowdCharacter::UpdateSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComponent, TSoftObjectPtr<USkeletalMesh> SoftSkeletalMeshPtr)
{
	USkeletalMesh* NewMesh = SoftSkeletalMeshPtr.LoadSynchronous();
	
	if (NewMesh && CrowdCharacterData && CrowdCharacterData->RayTracingMinLOD >= 0)
	{
		NewMesh->SetRayTracingMinLOD(CrowdCharacterData->RayTracingMinLOD);
	}
	
	// If we are not forcing InitAnim to tick animation, we won't know if we're rendering yet on the first tick but we most likely are. Force refresh bones to not t-pose for a frame
	UWorld* World = SkeletalMeshComponent->GetWorld();
	if(SkeletalMeshComponent->bUseRefPoseOnInitAnim && World)
	{
		EVisibilityBasedAnimTickOption InitialVisibilityBasedAnimTickOption = SkeletalMeshComponent->VisibilityBasedAnimTickOption;
		SkeletalMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		SkeletalMeshComponent->SetSkeletalMesh(NewMesh);

		TWeakObjectPtr<USkeletalMeshComponent> WeakSkeletalMeshComponent = SkeletalMeshComponent;
		World->GetTimerManager().SetTimerForNextTick([WeakSkeletalMeshComponent, InitialVisibilityBasedAnimTickOption]()
			{
				if (USkeletalMeshComponent* StrongSkeletalMeshComponent = WeakSkeletalMeshComponent.Get())
				{
					// After the first tick, go back to the original setting
					StrongSkeletalMeshComponent->VisibilityBasedAnimTickOption = InitialVisibilityBasedAnimTickOption;
				}
			});
	}
	else
	{
		SkeletalMeshComponent->SetSkeletalMesh(NewMesh);
	}
}

ESyncOption ACitySampleCrowdCharacter::GetSyncOptionForSlot(ECrowdMeshSlots MeshSlot)
{
	switch (MeshSlot)
	{
	case ECrowdMeshSlots::Body:
	case ECrowdMeshSlots::Head:
		return ESyncOption::Drive;
	default:
		return ESyncOption::Passive;
	}
}

USkeletalMeshComponent* ACitySampleCrowdCharacter::GetSkeletalMeshComponentForSlot(ECrowdMeshSlots BodySlot)
{
	if (BodySlot == ECrowdMeshSlots::Base)
	{
		return GetMesh();
	}
	else
	{
		// Subtract one to account for Base
		uint8 SlotIndex = static_cast<uint8>(BodySlot) - 1;

		return AdditionalMeshes.IsValidIndex(SlotIndex) ? AdditionalMeshes[SlotIndex] : nullptr;
	}
}

UGroomComponent* ACitySampleCrowdCharacter::GetGroomComponentForSlot(ECrowdHairSlots HairSlot)
{
	uint8 SlotIndex = static_cast<uint8>(HairSlot);

	return GroomComponents.IsValidIndex(SlotIndex) ? GroomComponents[SlotIndex] : nullptr;
}

UStaticMeshComponent* ACitySampleCrowdCharacter::GetAccessoryMeshComponent()
{
	return AccessoryMeshComponent;
}

void ACitySampleCrowdCharacter::SetEnableHair(const bool bNewHairEnabled, const bool bForceUpdateCharacter)
{
	bEnableHair = bNewHairEnabled;

	if (bForceUpdateCharacter)
	{
		BuildCharacter();
	}
}

void ACitySampleCrowdCharacter::Randomize()
{
	FRandomStream RandomStream;
	RandomStream.GenerateNewSeed();

	RandomizeFromStream(RandomStream);	
}

void ACitySampleCrowdCharacter::RandomizeFromStream(const FRandomStream& RandomStream)
{
	if (!CrowdCharacterData)
	{
		return;
	}
	
	// Need to convert to a set first
	const TSet<ECrowdLineupVariation> RandomFixedOptionsSet(RandomFixedOptions);
	CharacterOptions.Randomize(*CrowdCharacterData, RandomFixedOptionsSet, RandomStream);
	FCrowdCharacterDefinition CrowdCharacterDefinition;
	CharacterOptions.GenerateCharacterDefinition(CrowdCharacterData, CrowdCharacterDefinition);
	BuildCharacterFromDefinition(CrowdCharacterDefinition);
}

UDataAsset* ACitySampleCrowdCharacter::GetCurrentLocomotionAnimSet() const
{
	return PrivateCharacterDefinition.LocomotionAnimSet.LoadSynchronous();
}

UDataAsset* ACitySampleCrowdCharacter::GetCurrentAccessoryAnimSet() const
{
	return PrivateCharacterDefinition.AccessoryDefinition.AccessoryAnimSet.LoadSynchronous();
}

void ACitySampleCrowdCharacter::OnGetOrSpawn(FMassEntityManager* EntitySubsystem, const FMassEntityHandle MassAgent)
{
	if (EntitySubsystem && EntitySubsystem->IsEntityActive(MassAgent))
	{
		if (FMassVelocityFragment* MassVelocityFragment = EntitySubsystem->GetFragmentDataPtr<FMassVelocityFragment>(MassAgent))
		{
			if (UCharacterMovementComponent* CharMovement = GetCharacterMovement())
			{
				CharMovement->Velocity = MassVelocityFragment->Value;
			}
		}

		// Copy mass data relevant to animation to a persistent place, to have a correct first frame of anim
		if (FTransformFragment* TransformFragment = EntitySubsystem->GetFragmentDataPtr<FTransformFragment>(MassAgent))
		{
			SpawnAnimData.MassEntityTransform = TransformFragment->GetTransform();
		}

		if (FMassLookAtFragment* LookAtFragment = EntitySubsystem->GetFragmentDataPtr<FMassLookAtFragment>(MassAgent))
		{
			SpawnAnimData.LookAtDirection = LookAtFragment->Direction;
		}

		SpawnAnimData.FarLODAnimSequence = nullptr;
		SpawnAnimData.FarLODPlaybackStartTime = 0.0f;
		if (FCrowdAnimationFragment* AnimFragment = EntitySubsystem->GetFragmentDataPtr<FCrowdAnimationFragment>(MassAgent))
		{
			if (AnimFragment->AnimToTextureData.IsValid() && AnimFragment->AnimToTextureData->Animations.IsValidIndex(AnimFragment->AnimationStateIndex))
			{
				SpawnAnimData.FarLODAnimSequence = AnimFragment->AnimToTextureData->AnimSequences[AnimFragment->AnimationStateIndex].AnimSequence;
				if (SpawnAnimData.FarLODAnimSequence)
				{
					SpawnAnimData.FarLODAnimSequence = AnimFragment->AnimToTextureData->AnimSequences[AnimFragment->AnimationStateIndex].AnimSequence;

					if (UWorld* World = GetWorld())
					{
						const float GlobalTime = World->GetTimeSeconds();
						const float SequenceLength = SpawnAnimData.FarLODAnimSequence->GetPlayLength();
						SpawnAnimData.FarLODPlaybackStartTime = FMath::Fmod(AnimFragment->GlobalStartTime - GlobalTime, SequenceLength);

						if (SpawnAnimData.FarLODPlaybackStartTime < 0.0f)
						{
							SpawnAnimData.FarLODPlaybackStartTime += SequenceLength;
						}
					}
				}
			}
			SpawnAnimData.bSwappedThisFrame = AnimFragment->bSwappedThisFrame;
		}

		if (FMassRepresentationLODFragment* RepresentationFragment = EntitySubsystem->GetFragmentDataPtr<FMassRepresentationLODFragment>(MassAgent))
		{
			SpawnAnimData.Significance = RepresentationFragment->LODSignificance;
		}

		FCitySampleCrowdVisualizationFragment* CitySampleVisualizationFragment = EntitySubsystem->GetFragmentDataPtr<FCitySampleCrowdVisualizationFragment>(MassAgent);
		if (CitySampleVisualizationFragment)
		{
			BuildCharacterFromID(CitySampleVisualizationFragment->VisualizationID);
		}
	}
}
