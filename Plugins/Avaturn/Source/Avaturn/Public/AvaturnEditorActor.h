// Copyright © 2023++ Avaturn

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AvaturnEditorActor.generated.h"

UCLASS()
class AVATURN_API AAvaturnEditorActor : public AActor
{
	GENERATED_BODY()

public:
	AAvaturnEditorActor();

	///////////// Properties /////////////

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USceneComponent* SceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* BodyMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* HeadMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* LookMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* EyesMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Avaturn", meta = (ExposeFunctionCategories = "Avaturn", AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* ShoesMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDataTable* TargetSkeletonDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> DefaultAssets = { {"clothing", ""}, {"hair_and_headwear", ""}, {"eyewear", ""}, {"footwear", ""} };


	///////////// Functions /////////////
	UFUNCTION(BlueprintCallable)
	USkeletalMesh* ExportMesh();
};