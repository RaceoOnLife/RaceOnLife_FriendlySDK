// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/CitySampleMapWidget.h"

#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/CanvasPanelSlot.h"
#include "EngineUtils.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Game/CitySamplePlayerController.h"
#include "Game/CitySampleWorldInfo.h"


UCitySampleMapWidget::UCitySampleMapWidget()
{
	MapImage = nullptr;
	MapTextureOpacity = 1.0f;
}

void UCitySampleMapWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MapImage)
	{
		MapImage->SetOpacity(MapTextureOpacity);
	}

	HidePlayer(bHidePlayer);
}

void UCitySampleMapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	const bool bMapImageIsATexture = MapImage->GetBrush().GetResourceObject()->IsA(UTexture2D::StaticClass());

	if (bMapImageIsATexture)
	{
		const FIntPoint MapBaseSize = MapImage->GetBrush().GetImageSize().IntPoint() / 2;
		MapRect = { -MapBaseSize.X, -MapBaseSize.Y, MapBaseSize.X, MapBaseSize.Y };
	}	
	
	const ACitySampleWorldInfo* const WorldInfo = Cast<ACitySampleWorldInfo>(UGameplayStatics::GetActorOfClass(GetWorld(), ACitySampleWorldInfo::StaticClass()));
	USceneCaptureComponent2D* const SceneCapture = WorldInfo ? WorldInfo->GetWorldMapSceneCapture() : nullptr;

	if (SceneCapture)
	{
		if (bMapImageIsATexture)
		{
			// Use the scene capture for projection matrix only 
			InitializeMapViewProjectionMatrix(SceneCapture);
		}
		else
		{
			// Set the map image texture to the scene capture target
			SetMapTextureFromSceneCapture(SceneCapture);
		}
		
		// Initialize the tracked player when we have a scene capture and can calculate a map view projection matrix
		TrackedPlayer = GetOwningCitySamplePlayer();
	}

	UpdateMap(SceneCapture != nullptr);
}

void UCitySampleMapWidget::CaptureWorldMapToTexture()
{
	if (CachedSceneCaptureComponent.IsValid())
	{
		CachedSceneCaptureComponent->CaptureScene();
	}
}

void UCitySampleMapWidget::SetMapTexture(UTexture2D* const Texture)
{
	CachedSceneCaptureComponent.Reset();

	if (MapImage)
	{
		if (Texture)
		{
			// Store the map dimensions
			const FIntPoint MapBaseSize = { Texture->GetSizeX() / 2, Texture->GetSizeY() / 2 };
			MapRect = { -MapBaseSize.X, -MapBaseSize.Y, MapBaseSize.X, MapBaseSize.Y };

			// Set the brush size to match the render target dimensions
			FSlateBrush MapImageBrush = MapImage->GetBrush();
			MapImageBrush.SetImageSize(MapRect.Size());
			MapImage->SetBrush(MapImageBrush);
		}

		if (UMaterialInstanceDynamic* MapMaterial = MapImage->GetDynamicMaterial())
		{
			// Set the texture on the map image to the scene capture render target
			MapMaterial->SetTextureParameterValue(MapTextureParameterName, Texture);
		}
	}

	HideMap(!IsValid(Texture));
	UpdateMap();
}

void UCitySampleMapWidget::SetMapTextureFromSceneCapture(USceneCaptureComponent2D* const SceneCapture2D)
{
	CachedSceneCaptureComponent = SceneCapture2D;

	if (SceneCapture2D)
	{
		if (MapImage)
		{
			if (UTextureRenderTarget2D* const Texture = SceneCapture2D->TextureTarget)
			{
				InitializeMapViewProjectionMatrix(SceneCapture2D);

				// Store the map dimensions
				const FIntPoint MapBaseSize = { Texture->SizeX / 2, Texture->SizeY / 2 };
				MapRect = { -MapBaseSize.X, -MapBaseSize.Y, MapBaseSize.X, MapBaseSize.Y };

				// Set the brush size to match the render target dimensions
				FSlateBrush MapImageBrush = MapImage->GetBrush();
				MapImageBrush.SetImageSize(MapRect.Size());
				MapImage->SetBrush(MapImageBrush);

				if (UMaterialInstanceDynamic* MapMaterial = MapImage->GetDynamicMaterial())
				{
					// Set the texture on the map image to the scene capture render target
					MapMaterial->SetTextureParameterValue(MapTextureParameterName, Texture);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogCitySampleUI, Warning, TEXT("%s %s failed: scene capture is null"), *GetName(), ANSI_TO_TCHAR(__FUNCTION__))
	}

	HideMap(!IsValid(SceneCapture2D) || !IsValid(SceneCapture2D->TextureTarget));
	UpdateMap();
}

void UCitySampleMapWidget::SetMapTextureRenderTarget(UTextureRenderTarget2D* const Texture)
{
	CachedSceneCaptureComponent.Reset();

	if (MapImage)
	{
		if (Texture)
		{
			// Store the map dimensions
			const FIntPoint MapBaseSize = { Texture->SizeX / 2, Texture->SizeY / 2 };
			MapRect = { -MapBaseSize.X, -MapBaseSize.Y, MapBaseSize.X, MapBaseSize.Y };

			// Set the brush size to match the render target dimensions
			FSlateBrush MapImageBrush = MapImage->GetBrush();
			MapImageBrush.SetImageSize(MapRect.Size());
			MapImage->SetBrush(MapImageBrush);
		}


		if (UMaterialInstanceDynamic* MapMaterial = MapImage->GetDynamicMaterial())
		{
			// Set the texture on the map image to the scene capture render target
			MapMaterial->SetTextureParameterValue(MapTextureParameterName, Texture);
		}
	}

	HideMap(!IsValid(Texture));
	UpdateMap();
}

void UCitySampleMapWidget::UpdateMap(const bool bCaptureScene /*= false*/)
{
	if (bHideMap)
	{
		return;
	}

	if (bCaptureScene)
	{
		CaptureWorldMapToTexture();
	}

	// Let BP handle setting up the UI for the initial map size
	OnSetMapSize(FVector2D(MapRect.Size()));

	const APlayerController* const TrackedPC = TrackedPlayer.Get();

	// Update player map position and rotation, if they exist
	if (const APawn* const Pawn = TrackedPC ? TrackedPC->GetPawn() : nullptr)
	{
		SetPlayerMapPositionFromWorld(Pawn->GetActorLocation());
		SetPlayerMapRotation(Pawn->GetActorRotation().Yaw);
	}
	
	OnHidePlayer(!TrackedPC || bHidePlayer);

	if (MapImage)
	{
		MapImage->SetOpacity(MapTextureOpacity);
	}
}

void UCitySampleMapWidget::HideMap(const bool bShouldHideMap/*=true*/)
{
	bHideMap = bShouldHideMap;
	OnHideMap(bHideMap);
}

void UCitySampleMapWidget::HidePlayer(const bool bShouldHidePlayer/*=true*/)
{
	bHidePlayer = bShouldHidePlayer;
	OnHidePlayer(bHidePlayer);
}

void UCitySampleMapWidget::SetPlayerMapPosition(const FVector2D& MapPosition)
{
	PlayerMapPosition = MapPosition;

	// Let BP handle updates for the new player map position, adjusted for scaling and panning offset
	OnSetPlayerMapPosition(PlayerMapPosition);
}

void UCitySampleMapWidget::SetPlayerMapRotation(const float MapRotation)
{
	PlayerMapRotation = MapRotation;
	
	// Let BP handle updates for the new player map position, adjusted for scaling and panning offset
	OnSetPlayerMapRotation(PlayerMapRotation);
}

void UCitySampleMapWidget::SetPlayerMapPositionFromWorld(const FVector& WorldLocation)
{
	// Projects the player world location to render target (map) space
	FSceneView::ProjectWorldToScreen(WorldLocation, MapRect, MapViewProjectionMatrix, PlayerMapPosition);
	
	// Let BP handle updates for the new player map position, adjusted for scaling and panning offset
	OnSetPlayerMapPosition(PlayerMapPosition);
}

bool UCitySampleMapWidget::IsMapPositionWithinBounds(const FVector2D& MapPosition)
{
	// Calculate the map min and max bounds scaled
	const FVector2D MinPosition = FVector2D(MapRect.Min);
	const FVector2D MaxPosition = FVector2D(MapRect.Max);

	// Returns true if the map position is within the scaled map bounds
	return (MapPosition.X < MaxPosition.X) && (MapPosition.X > MinPosition.X)
		&& (MapPosition.Y < MaxPosition.Y) && (MapPosition.Y > MinPosition.Y);
}

void UCitySampleMapWidget::InitializeMapViewProjectionMatrix(USceneCaptureComponent2D* const SceneCapture2D)
{
	check(SceneCapture2D);

	// Cache the MapViewProjection matrix for world to render target (map) space projections
	FMinimalViewInfo InMapViewInfo;
	SceneCapture2D->GetCameraView(0.0f, InMapViewInfo);

	// Get custom projection matrix, if it exists
	TOptional<FMatrix> InCustomProjectionMatrix;
	if (SceneCapture2D->bUseCustomProjectionMatrix)
	{
		InCustomProjectionMatrix = SceneCapture2D->CustomProjectionMatrix;
	}

	// The out parameters for the individual view and projection matrix will not be needed
	FMatrix MapViewMatrix, MapProjectionMatrix;

	// Cache the MapViewProjection matrix
	UGameplayStatics::CalculateViewProjectionMatricesFromMinimalView(
		InMapViewInfo,
		InCustomProjectionMatrix,
		MapViewMatrix,
		MapProjectionMatrix,
		MapViewProjectionMatrix
	);
}
