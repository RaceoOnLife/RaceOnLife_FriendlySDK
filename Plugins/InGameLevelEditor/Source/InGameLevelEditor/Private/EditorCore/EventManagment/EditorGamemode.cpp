#include "EditorCore/EventManagment/EditorGamemode.h"

AEditorGamemode::AEditorGamemode()
{
    DefaultPawnClass = AEditorPawn::StaticClass();
    PlayerControllerClass = AEditorPlayerController::StaticClass();
}