// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "CameraMod_PostProcessMaterial.generated.h"

/**
 * Basic class for a camera modifier that's a simple postprocess material camera overlay. Subclass in BP for specific effects.
 */
UCLASS()
class CITYSAMPLE_API UCameraMod_PostProcessMaterial : public UCameraModifier
{
	GENERATED_BODY()
	
public:
	virtual void ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings) override;
	virtual void AddedToCamera(APlayerCameraManager* Camera) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* AgentVisionPostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStartDisabled = true;

};
