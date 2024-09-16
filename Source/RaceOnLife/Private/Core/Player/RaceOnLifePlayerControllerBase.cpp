


#include "Core/Player/RaceOnLifePlayerControllerBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/XmlParser/Public/XmlParser.h"
#include "Runtime/XmlParser/Public/XmlNode.h"
#include "Runtime/XmlParser/Public/XmlFile.h"

FString ARaceOnLifePlayerControllerBase::GetUserID() const
{
    FString UserID;

    FString LocalAppDataPath = FPlatformMisc::GetEnvironmentVariable(TEXT("localappdata"));
    FString DataDirectory = FPaths::Combine(LocalAppDataPath, TEXT("RaceOnLife"), TEXT("Saved"), TEXT("ClientData"));
    FString FilePath = FPaths::Combine(DataDirectory, TEXT("CurrentUserData.xml"));

    if (FPaths::FileExists(FilePath))
    {
        FXmlFile XmlFile(FilePath, EConstructMethod::ConstructFromFile);
        if (XmlFile.IsValid())
        {
            const FXmlNode* RootNode = XmlFile.GetRootNode();
            if (RootNode)
            {
                const FXmlNode* UserIDNode = RootNode->FindChildNode(TEXT("UserID"));
                if (UserIDNode)
                {
                    UserID = UserIDNode->GetContent();
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load XML file at path: %s"), *FilePath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CurrentUserData.xml not found at path: %s"), *FilePath);
    }

    return UserID;
}

