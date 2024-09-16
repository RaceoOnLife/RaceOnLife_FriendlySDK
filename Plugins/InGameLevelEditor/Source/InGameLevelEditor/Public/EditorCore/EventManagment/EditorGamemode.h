

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EditorCore/EventManagment/EditorPawn.h"
#include "EditorCore/EventManagment/EditorPlayerController.h"
#include "EditorGamemode.generated.h"

/**
 * 
 */
UCLASS()
class INGAMELEVELEDITOR_API AEditorGamemode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AEditorGamemode();
};
