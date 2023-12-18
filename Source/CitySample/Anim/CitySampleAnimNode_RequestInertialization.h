// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNodeMessages.h"
#include "CitySampleAnimNode_RequestInertialization.generated.h"

UENUM()
enum class ERequestInertializationCondition : uint8
{
	OnTrue,
	OnChange,
};

// Requests inertialization based on a boolean input
USTRUCT(BlueprintInternalUseOnly)
struct CITYSAMPLE_API FCitySampleAnimNode_RequestInertialization : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink Source;

	// Determines if an inertialization blend should be triggered, depending on Condition.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta=(PinShownByDefault))
	bool bInput = false;

	// Duration of the requested inertialization blend.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float BlendTime = 0.2f;

private:
#if WITH_EDITORONLY_DATA
	// If OnTrue, request when bInput is true; if OnChanged, request when bInput changes.
	UPROPERTY(EditAnywhere, Category = Config, meta = (FoldProperty))
	ERequestInertializationCondition Condition = ERequestInertializationCondition::OnChange;

	// If set to true, inertialization will not be requested on the same frame the node becomes relevant
	UPROPERTY(EditAnywhere, Category = Settings, meta = (FoldProperty))
	bool bSkipOnBecomingRelevant = false;
#endif

public:
	ERequestInertializationCondition GetCondition() const;
	bool GetSkipOnBecomingRelevant() const;

public: // FAnimNode_Base
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

protected:
	bool bLastInput = false;
	bool bReinitialized = false;

	// Used to check if the node just became relevant.
	FGraphTraversalCounter UpdateCounter;
};

// This node must be added right after an animation asset player to function correctly. It
// requests an inertialization blend when the animation asset of the corresponding player changes.
USTRUCT(BlueprintInternalUseOnly)
struct CITYSAMPLE_API FCitySampleAnimNode_RequestInertializationOnAssetChange : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	// Must link to an asset player node for this to work correctly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink Source;

	// Duration of the requested inertialization blend.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float BlendTime = 0.2f;

public: // FAnimNode_Base
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

protected:
	const FAnimNode_AssetPlayerBase* AssetPlayerNode = nullptr;
	const UAnimationAsset* CachedAnimationAsset = nullptr;
	bool bReinitialized = false;

	// Used to check if the node just became relevant.
	FGraphTraversalCounter UpdateCounter;
};

