// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CitySamplePanel.h"
#include "CitySampleMapWidget.generated.h"

class UImage;
class UCanvasPanelSlot;
class USceneCaptureComponent2D;
class UTexture2D;
class UTextureRenderTarget2D;

/**
 * CitySample UI base class for a Map widget. Can be used with either a specific texture or a scene capture render target.
 */
UCLASS()
class CITYSAMPLE_API UCitySampleMapWidget : public UCitySamplePanel
{
	GENERATED_BODY()
	
public:
	UCitySampleMapWidget();

	virtual void SynchronizeProperties() override;
	virtual void NativeOnInitialized() override;

	/** Captures the scene of the cached world map scene capture, if applicable. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void CaptureWorldMapToTexture();

	/** Sets the map image texture directly. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void SetMapTexture(UTexture2D* const Texture);

	/** Sets the map image texture and updates the cached scene capture component and the view projection matrix. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void SetMapTextureFromSceneCapture(USceneCaptureComponent2D* const SceneCapture2D);

	/** Sets the map image texture from a render target. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void SetMapTextureRenderTarget(UTextureRenderTarget2D* const Texture);

	/** Updates the scene capture, map size and visibility, and player position/rotation. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void UpdateMap(const bool bCaptureScene = false);

	/** Sets whether the map should be hidden. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void HideMap(const bool bShouldHideMap=true);

	/** Whether the map should be hidden. */
	UFUNCTION(BlueprintPure, Category = "Map Widget")
	bool IsMapHidden() const
	{
		return bHideMap;
	}

	/** Sets whether the player marker should be hidden. */
	UFUNCTION(BlueprintCallable, Category = "Player Widget")
	void HidePlayer(const bool bShouldHidePlayer=true);

	/** Whether the player marker should be hidden. */
	UFUNCTION(BlueprintPure, Category = "Player Widget")
	bool IsPlayerHidden() const
	{
		return bHidePlayer;
	}

	/** 
	 * Sets the current player map position in screen space. 
	 * @note	Input map position should not include zoom scaling or pan offsets.
	 */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void SetPlayerMapPosition(const FVector2D& MapPosition);

	/** Sets the current player map rotation in screen space. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void SetPlayerMapRotation(const float MapRotation);

	/** Sets the current player map position, projecting the world location into render target (map) space. */
	UFUNCTION(BlueprintCallable, Category = "Map Widget")
	void SetPlayerMapPositionFromWorld(const FVector& WorldLocation);

	/** Checks if the map position (in render target space) is within the render target bounds, includes zoom scaling. */
	UFUNCTION(BlueprintPure, Category = "Map Widget")
	bool IsMapPositionWithinBounds(const FVector2D& MapPosition);

protected:
	/** BP hook for handling UI updates to the map visibility. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Widget")
	void OnHideMap(const bool bShouldHideMap);

	/** BP hook for handling UI updates to the map size. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Widget")
	void OnSetMapSize(const FVector2D& Size);
	
	/** BP hook for handling UI updates to the map position. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Widget")
	void OnSetMapPosition(const FVector2D& Position);

	/** BP hook for handling UI updates to player marker visibility. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Widget")
	void OnHidePlayer(const bool bShouldHidePlayer);

	/** BP hook for handling UI updates to the player position on the map. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Widget")
	void OnSetPlayerMapPosition(const FVector2D& ScreenPosition);

	/** BP hook for handling UI updates to the player rotation on the map. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Widget")
	void OnSetPlayerMapRotation(const float ScreenRotation);

	/** The UImage widget whose material is used when setting the scene render target as a texture parameter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(BindWidget), Category = "Map Widget")
	UImage* MapImage;

	/** Name of the texture parameter on the image material to be set to the scene render target texture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Widget")
	FName MapTextureParameterName;

	/** Name of the texture parameter on the image material to be set to the scene render target texture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Widget")
	float MapTextureOpacity;

	/** Whether to hide the player marker, if applicable. */
	UPROPERTY(EditAnywhere, Category = "Map Widget|Transient")
	bool bHidePlayer;

private:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Map Widget|Transient")
	bool bHideMap;

	/** Rect representing render target (map) space. */
	FIntRect MapRect;
	
	/** Cached scene capture component that renders the map image texture target. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Map Widget|Transient")
	TWeakObjectPtr<USceneCaptureComponent2D> CachedSceneCaptureComponent;
	
	/** ViewProjection matrix used to project from world space to render target (map) space, i.e. the player position. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Map Widget|Transient")
	FMatrix MapViewProjectionMatrix;

	/** The player whose position is represented by the player marker. This is initialized to the PC that owns the map widget. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Map Widget|Transient")
	TWeakObjectPtr<const APlayerController> TrackedPlayer;

	/** Stored position of the player in render target space, a.k.a. where the player is on the map relative to the map center. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Map Widget|Transient")
	FVector2D PlayerMapPosition;

	/** Stored rotation of the player in render target space, a.k.a. direction the player is facing relative to the map. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Map Widget|Transient")
	float PlayerMapRotation;

	void InitializeMapViewProjectionMatrix(USceneCaptureComponent2D* const SceneCapture2D);
};
