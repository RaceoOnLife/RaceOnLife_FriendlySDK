#include "core/sdk_instance.h"

Usdk_instance::Usdk_instance()
{
    PlayerScore = 0;
}

void Usdk_instance::SetPlayerScore(int32 NewScore)
{
    PlayerScore = NewScore;
    UE_LOG(LogTemp, Log, TEXT("Player score set to: %d"), PlayerScore);
}

int32 Usdk_instance::GetPlayerScore() const
{
    return PlayerScore;
}
