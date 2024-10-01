#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrustumCameraComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RACEONLIFE_API UFrustumCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFrustumCameraComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool IsActorInView(class UStaticMeshComponent* MeshComp, const FConvexVolume& FrustumVolume) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class ACameraActor* CameraActor;
		
};
