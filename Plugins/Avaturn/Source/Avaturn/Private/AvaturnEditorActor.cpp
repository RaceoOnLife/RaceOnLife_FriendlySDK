// Copyright © 2023++ Avaturn

#include "AvaturnEditorActor.h"
#include "AvaturnGameSubsystem.h"
#include "MeshMergeFunctionLibrary.h"


AAvaturnEditorActor::AAvaturnEditorActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneComponent);

	BodyMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	BodyMeshComponent->SetupAttachment(SceneComponent);

	HeadMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh"));
	HeadMeshComponent->SetupAttachment(BodyMeshComponent);

	LookMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LookMesh"));
	LookMeshComponent->SetupAttachment(BodyMeshComponent);

	EyesMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EyesMesh"));
	EyesMeshComponent->SetupAttachment(BodyMeshComponent);

	ShoesMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShoesMesh"));
	ShoesMeshComponent->SetupAttachment(BodyMeshComponent);
}

USkeletalMesh* AAvaturnEditorActor::ExportMesh()
{
	FSkeletalMeshMergeParams Params = FSkeletalMeshMergeParams();
	TArray<USkeletalMesh*> MeshesToMerge;

#if ENGINE_MAJOR_VERSION > 4 && ENGINE_MINOR_VERSION > 0
	MeshesToMerge.Add(BodyMeshComponent->GetSkeletalMeshAsset());
	MeshesToMerge.Add(HeadMeshComponent->GetSkeletalMeshAsset());
	MeshesToMerge.Add(LookMeshComponent->GetSkeletalMeshAsset());
	MeshesToMerge.Add(EyesMeshComponent->GetSkeletalMeshAsset());
	MeshesToMerge.Add(ShoesMeshComponent->GetSkeletalMeshAsset());
#else
	MeshesToMerge.Add(BodyMeshComponent->SkeletalMesh);
	MeshesToMerge.Add(HeadMeshComponent->SkeletalMesh);
	MeshesToMerge.Add(LookMeshComponent->SkeletalMesh);
	MeshesToMerge.Add(EyesMeshComponent->SkeletalMesh);
	MeshesToMerge.Add(ShoesMeshComponent->SkeletalMesh);
#endif

	Params.MeshesToMerge = MeshesToMerge;
	Params.Skeleton = MeshesToMerge[0]->GetSkeleton();
	
	USkeletalMesh* MergedMesh = UMeshMergeFunctionLibrary::MergeMeshes(Params);
	if (MergedMesh)
	{
		return MergedMesh;
	}
	
	return  MeshesToMerge[0];
}