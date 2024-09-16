// Copyright © 2023++ Avaturn

#include "AvaturnComponent.h"
#include "AvaturnAvatarLoader.h"
#include "AvaturnMemoryCache.h"
#include "AvaturnGameSubsystem.h"
#include "Utils/AvaturnGlTFConfigCreator.h"
#include "glTFRuntimeAsset.h"
#include "UObject/UObjectGlobals.h"
#include "Components/SkeletalMeshComponent.h"
//#include "Media/Public/IMediaTracks.h"

UAvaturnComponent::UAvaturnComponent()
	: TargetSkeleton(FTargetSkeleton())
	, SkeletalMeshComponent(nullptr)
	, bUseMemoryCache(false)
{
	PrimaryComponentTick.bCanEverTick = false;
	OnAvatarDownloadCompleted.BindDynamic(this, &UAvaturnComponent::OnAvatarDownloaded);
	OnSkeletalMeshCallback.BindDynamic(this, &UAvaturnComponent::OnSkeletalMeshLoaded);
}

void UAvaturnComponent::BeginPlay()
{
	Super::BeginPlay();

	const UAvaturnGameSubsystem* GameSubsystem = UGameInstance::GetSubsystem<UAvaturnGameSubsystem>(GetWorld()->GetGameInstance());

	if (IsValid(GameSubsystem))
	{
		if (GameSubsystem->MemoryCache)
		{
			LastExportResult = GameSubsystem->MemoryCache->LastExportResult;
			if (!LastExportResult.AvatarId.IsEmpty())
			{
				LastExportResult.bDataUrl = true;
				LoadAvatar(FAvatarLoadCompleted(), FAvatarLoadFailed());
			}
		}
	}
}

void UAvaturnComponent::LoadAvatar(const FAvatarLoadCompleted& OnLoadCompleted, const FAvatarLoadFailed& OnLoadFailed)
{
	if (!LastExportResult.IsValid())
	{
		(void)OnLoadFailed.ExecuteIfBound("Url is empty");
		return;
	}
	if (!GetTargetSkeleton())
	{
		(void)OnLoadFailed.ExecuteIfBound("No target skeleton set");
		return;
	}

	const UAvaturnGameSubsystem* GameSubsystem = UGameInstance::GetSubsystem<UAvaturnGameSubsystem>(GetWorld()->GetGameInstance());

	if (!bLoadNewAvatar)
	{
		if (bUseMemoryCache)
		{
			if (IsValid(GameSubsystem))
			{
				const FAvatarMemoryCacheData CacheData = GameSubsystem->MemoryCache->GetAvatarCacheData(LastExportResult);
				if (IsValid(CacheData.Mesh))
				{
					OnSkeletalMeshLoaded(CacheData.Mesh);
					return;
				}
			}
		}

		if (IsValid(GameSubsystem))
		{
			if (IsValid(GameSubsystem->MemoryCache))
			{
				GameSubsystem->MemoryCache->LastExportResult = LastExportResult;
			}
		}
	}

	OnAvatarLoadCompleted = OnLoadCompleted;
	AvatarLoader = NewObject<UAvaturnAvatarLoader>(this,TEXT("AvatarLoader"));
	AvatarLoader->ModelUrl = LastExportResult.Url;
	AvatarLoader->AvatarId = LastExportResult.AvatarId;
	AvatarLoader->Root_Bone = Root_Bone;
	AvatarLoader->LoadAvatar(LastExportResult.bDataUrl, GetTargetSkeleton(), OnAvatarDownloadCompleted, OnLoadFailed);

	bLoadNewAvatar = false;
}

void UAvaturnComponent::LoadNewAvatar(const FExportAvatarResult& ExportResult, const FAvatarLoadCompleted& OnLoadCompleted, const FAvatarLoadFailed& OnLoadFailed)
{
	LastExportResult = ExportResult;
	bLoadNewAvatar = true;
	LoadAvatar(OnLoadCompleted, OnLoadFailed);
}

void UAvaturnComponent::LoadAvatarFromFile(const FString& Path, const FAvatarLoadCompleted& OnLoadCompleted, const FAvatarLoadFailed& OnLoadFailed)
{
	LastExportResult.Url = Path;
	LastExportResult.AvatarId = "test";
	LastExportResult.bAvatarSupportsFaceAnimations = false;

	OnAvatarLoadCompleted = OnLoadCompleted;

	AvatarLoader = NewObject<UAvaturnAvatarLoader>(this, TEXT("AvatarLoader"));
	AvatarLoader->ModelUrl = Path;
	AvatarLoader->AvatarId = "test";
	AvatarLoader->Root_Bone = Root_Bone;
	AvatarLoader->LoadAvatarFromFile(Path, GetTargetSkeleton(), OnAvatarDownloadCompleted, OnLoadFailed);
}

void UAvaturnComponent::CancelAvatarLoad()
{
	if(IsValid(AvatarLoader))
	{
		AvatarLoader->CancelAvatarLoad();
	}
}

void UAvaturnComponent::OnAvatarDownloaded(USkeletalMesh* Mesh)
{
	if (bUseMemoryCache)
	{
		const UAvaturnGameSubsystem* GameSubsystem = UGameInstance::GetSubsystem<UAvaturnGameSubsystem>(GetWorld()->GetGameInstance());
		if (IsValid(GameSubsystem))
		{
			GameSubsystem->MemoryCache->AddAvatar(LastExportResult, Mesh);
		}
	}
	OnSkeletalMeshLoaded(Mesh);
}

void UAvaturnComponent::OnSkeletalMeshLoaded(USkeletalMesh* Mesh)
{
	InitSkeletalMeshComponent();
	SkeletalMeshComponent->SetAnimInstanceClass(GetTargetAnimClass());
	SkeletalMeshComponent->SetSkeletalMesh(Mesh);
	(void)OnAvatarLoadCompleted.ExecuteIfBound();
}

void UAvaturnComponent::InitSkeletalMeshComponent()
{
	if (IsValid(SkeletalMeshComponent))
	{
		return;
	}
	AActor* ThisActor = GetOwner();
	SkeletalMeshComponent = ThisActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!SkeletalMeshComponent)
	{
		SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(ThisActor, TEXT("SkeletalMesh"));
		SkeletalMeshComponent->SetupAttachment(ThisActor->GetRootComponent());
		SkeletalMeshComponent->RegisterComponent();
	}
}

USkeleton* UAvaturnComponent::GetTargetSkeleton()
{
	return TargetSkeleton.Skeleton.LoadSynchronous();
}

UClass* UAvaturnComponent::GetTargetAnimClass()
{
	return TargetSkeleton.AnimClass.LoadSynchronous();
}