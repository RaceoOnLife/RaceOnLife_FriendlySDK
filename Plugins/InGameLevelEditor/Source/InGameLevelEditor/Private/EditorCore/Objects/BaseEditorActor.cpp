#include "EditorCore/Objects/BaseEditorActor.h"

// Sets default values
ABaseEditorActor::ABaseEditorActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABaseEditorActor::BeginPlay()
{
	Super::BeginPlay();
}
