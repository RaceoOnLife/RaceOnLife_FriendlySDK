#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseEditorActor.generated.h"

USTRUCT(BlueprintType)
struct FBaseEditorActorData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    FString ObjectName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    FTransform WorldTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    UStaticMesh* StaticMeshReference;

    FBaseEditorActorData()
        : ObjectName(TEXT("DefaultName"))
        , WorldTransform(FTransform::Identity)
        , StaticMeshReference(nullptr)
    {}
};

UCLASS()
class INGAMELEVELEDITOR_API ABaseEditorActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseEditorActor();

protected:
	virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
    FBaseEditorActorData EditorData;

};
