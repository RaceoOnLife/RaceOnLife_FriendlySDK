// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetHardReferencesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogAssetHardReferences, Log, All);

// TODO correct the usage of package / object / asset

struct FPackageReferenceInformation
{
	int32 HardReferenceCount = -1;
	int32 ReferencedByCount  =  0;

	TSet<FName> ReferencedPackages;
	TArray<FName> FinishedPackages;

	bool bHasBeenInitialized = false;

	FPackageReferenceInformation() = default;

	void Initialize(const FName PackageName)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		Initialize(PackageName, AssetRegistryModule.Get());
	}

	void Initialize(const FName& PackageName, IAssetRegistry& AssetRegistry)
	{
		// Only get the first level of dependencies as the repeated merging will take take of all the child dependencies 
		TArray<FAssetIdentifier> FoundDependencies;
		AssetRegistry.GetDependencies(PackageName, FoundDependencies, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Hard);

		for (FAssetIdentifier& DependencyIdentifier : FoundDependencies)
		{
			ReferencedPackages.Add(DependencyIdentifier.PackageName);
		}
	}

	// Returns true if the Update changed something about the reference information
	bool UpdateReferences(TMap<FName, FPackageReferenceInformation>& ReferenceMap, TArray<FName>& NewPackages)
	{
		bool bChanged = false;

		TSet<FName> CachedReferencedPackages = ReferencedPackages;
		for (const FName & ReferencedPackageName : CachedReferencedPackages)
		{
			if (!FinishedPackages.Contains(ReferencedPackageName))
			{
				if (FPackageReferenceInformation* ReferencedPackageInformation = ReferenceMap.Find(ReferencedPackageName))
				{
					bChanged |= MergeReferences(*ReferencedPackageInformation);

					// If the referenced package has been fully updated then there's no need to check it on future updates
					if (ReferencedPackageInformation->IsComplete())
					{
						FinishedPackages.Add(ReferencedPackageName);
					}
				}
				else
				{
					NewPackages.AddUnique(ReferencedPackageName);
					bChanged = true;
				}
			}
		}

		// If we both didn't find any new references while merging or new packages to search on future updates
		// then we have found all the dependencies and we can stop
		if (!bChanged)
		{
			Complete();
		}

		return bChanged;
	};

	// Returns true if new references were found
	bool MergeReferences(const FPackageReferenceInformation& OtherAssetInformation)
	{
		int PreviousCount = ReferencedPackages.Num();
		ReferencedPackages.Append(OtherAssetInformation.ReferencedPackages);

		return PreviousCount != ReferencedPackages.Num();
	};

	void Complete()
	{
		bComplete = true;
		FinishedPackages.Empty();
		HardReferenceCount = ReferencedPackages.Num();
	}

	bool IsComplete()
	{
		if (!bComplete && FinishedPackages.Num() == ReferencedPackages.Num())
		{
			Complete();
		}

		return bComplete;
	};

private:
	bool bComplete = false;
};

void CalculateReferences(TMap<FName, FPackageReferenceInformation>& ReferenceMap)
{
	while (true)
	{
		// How many assets actually changed this pass
		int ChangeCount = 0;

		// Array of new objects discovered during the update
		TArray<FName> NewPackages;
		for (TPair<FName, FPackageReferenceInformation>& ReferenceMapPair : ReferenceMap)
		{
			FPackageReferenceInformation& PackageReferenceInformation = ReferenceMapPair.Value;

			// Only run update on packages that are not already complete
			if (!PackageReferenceInformation.IsComplete())
			{
				bool bChanged = PackageReferenceInformation.UpdateReferences(ReferenceMap, NewPackages);

				if (bChanged)
				{
					ChangeCount++;
				}
			}
		}

		UE_LOG(LogAssetHardReferences, Display, TEXT("Processed %d assets and %d changed"), ReferenceMap.Num(), ChangeCount);

		if (ChangeCount == 0)
		{
			break;
		}

		// Add any new packages found during updates to the reference map and initialize them with their direct dependencies
		for (const FName& NewObject : NewPackages)
		{
			FPackageReferenceInformation& NewInformation = ReferenceMap.Add(NewObject);
			NewInformation.Initialize(NewObject);
		}
	}
}

// Take a ReferenceMap that has already computed all the references and calculate the ReferencedBy counts
void CalculateReferencedBy(TMap<FName, FPackageReferenceInformation>& ReferenceMap)
{
	for (TPair<FName, FPackageReferenceInformation>& ReferenceMapPair : ReferenceMap)
	{
		FPackageReferenceInformation& PackageReferenceInformation = ReferenceMapPair.Value;

		for (const FName& ReferencedPackage : PackageReferenceInformation.ReferencedPackages)
		{
			FPackageReferenceInformation& ReferencedAssetInformation = ReferenceMap.FindChecked(ReferencedPackage);
			ReferencedAssetInformation.ReferencedByCount++;
		}
	}
}

void SerializeHeader(FArchive* Archive)
{
	FTCHARToUTF8 UTF8Header(TEXT("Package Name,Asset Name,Asset Class,Hard Reference Count,Referenced By Count\n"));
	Archive->Serialize((UTF8CHAR*)UTF8Header.Get(), UTF8Header.Length() * sizeof(UTF8CHAR));
}

void SerializeReferenceMap(FArchive* Archive, const TMap<FName, FPackageReferenceInformation>& ReferenceMap, const TArray<FAssetData>* AssetsInFolderPtr = nullptr)
{
	auto SerializeAssetInformation = [&](const FName& PackageName, const FName& AssetName, const FTopLevelAssetPath& AssetClass, const FPackageReferenceInformation& ReferenceInformation) {
		FString CSVRow = FString::Printf(TEXT("%s,%s,%s,%d,%d\n"),
			*PackageName.ToString(),
			*AssetName.ToString(),
			*AssetClass.ToString(),
			ReferenceInformation.HardReferenceCount,
			ReferenceInformation.ReferencedByCount);

		FTCHARToUTF8 UTF8Row(*CSVRow);
		Archive->Serialize((UTF8CHAR*)UTF8Row.Get(), UTF8Row.Length() * sizeof(UTF8CHAR));
	};

	// If we have an array of assets then build the table using information about those assets otherwise dump all the packages in the ReferenceMap
	if (AssetsInFolderPtr)
	{
		const TArray<FAssetData>& AssetsInFolder = *AssetsInFolderPtr;

		for (const FAssetData& AssetInFolder : AssetsInFolder)
		{
			const FName& AssetPackageName = AssetInFolder.PackageName;
			const FPackageReferenceInformation& ReferenceInformation = ReferenceMap.FindChecked(AssetPackageName);

			SerializeAssetInformation(AssetPackageName, AssetInFolder.AssetName, AssetInFolder.AssetClassPath, ReferenceInformation);
		}
	}
	else
	{
		for (const TPair<FName, FPackageReferenceInformation>& ReferencePair : ReferenceMap)
		{
			const FName& CurrentPackageName = ReferencePair.Key;
			const FPackageReferenceInformation& CurrentReferenceInformation = ReferencePair.Value;

			SerializeAssetInformation(CurrentPackageName, NAME_None, FTopLevelAssetPath(TEXT("/Script/CoreUObject"), TEXT("Package")), CurrentReferenceInformation);
		}
	}
}

UAssetHardReferencesCommandlet::UAssetHardReferencesCommandlet()
{
	HelpDescription = TEXT("Walks a directory and generates a CSV files containg the number of hard references for each asset. By default this is done without recursion starting in the project's content folder. The result will be created in a Reports subfolder within the Project Directory.");

	HelpUsage = TEXT("AssetHardReferences Usage: AssetHardReferences [-Folder=FolderName] [-Recursive]");

	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UAssetHardReferencesCommandlet::Main(const FString& FullCommandLine)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Params;

	ParseCommandLine(*FullCommandLine, Tokens, Switches, Params);

	if (Switches.Contains(TEXT("help")) || Switches.Contains(TEXT("h")))
	{
		UE_LOG(LogAssetHardReferences, Display, TEXT("%s\n%s"), *HelpDescription, *HelpUsage);
		return 0;
	}

	// Currently we only support one search folder but in theory we could support more

	FString StartingFolder = TEXT("/Game");

	if (FString* FolderParam = Params.Find(TEXT("folder")))
	{
		StartingFolder = *FolderParam;
	}
	else if (FString* ShortFolderParam = Params.Find(TEXT("f")))
	{
		StartingFolder = *ShortFolderParam;
	}

	// Remove any trailing forward slashes
	if (StartingFolder.EndsWith(TEXT("/")))
	{
		StartingFolder.LeftChopInline(1);
	}

	const bool bRecursive = Switches.Contains(TEXT("recursive")) || Switches.Contains(TEXT("r"));

	UE_LOG(LogAssetHardReferences, Display, TEXT("Starting Folder\t\t- %s"), *StartingFolder);
	UE_LOG(LogAssetHardReferences, Display, TEXT("Recursive\t\t- %s"), bRecursive ? TEXT("True") : TEXT("False"));
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Update Registry Module
	UE_LOG(LogAssetHardReferences, Display, TEXT("Searching Asset Registry..."));
	AssetRegistryModule.Get().SearchAllAssets(true);

	TMap<FName, FPackageReferenceInformation> ReferenceMap;
	TArray<FAssetData> AssetsInFolder;

	FARFilter Filter;
	Filter.bIncludeOnlyOnDiskAssets = true;
	Filter.PackagePaths.Emplace(*StartingFolder);
	Filter.bRecursivePaths = bRecursive;
	
	AssetRegistry.GetAssets(Filter, AssetsInFolder);	

	// Prepare the map, this will be used for some existence checking later
	for (const FAssetData& CurrentAssetData : AssetsInFolder)
	{
		FPackageReferenceInformation& NewReferenceInformation = ReferenceMap.Add(CurrentAssetData.PackageName);
		NewReferenceInformation.Initialize(CurrentAssetData.PackageName, AssetRegistry);
	}


	FDateTime StartTime = FDateTime::Now();

	CalculateReferences(ReferenceMap);
	CalculateReferencedBy(ReferenceMap);

	FTimespan ElapsedTime = FDateTime::Now() - StartTime;

	UE_LOG(LogAssetHardReferences, Display, TEXT("Searching assets completed in %s."), *ElapsedTime.ToString());
	
	// Output the results to CSV file

	const FString SearchFolder = StartingFolder.Replace(TEXT("/"), TEXT("_"));
	const FString OutputFilePath = FPaths::ProjectDir() / TEXT("Reports") /  FString::Printf(TEXT("%s-%s-%s-%s.csv"), FApp::GetProjectName(), TEXT("AssetHardReferences"), *SearchFolder, *FDateTime::Now().ToString());;

	FArchive* CSVArchive = IFileManager::Get().CreateFileWriter(*OutputFilePath, EFileWrite::FILEWRITE_AllowRead);

	if (!CSVArchive)
	{
		UE_LOG(LogAssetHardReferences, Error, TEXT("Unable to create output CSV file. Filename was %s"), *OutputFilePath);
		return 2;
	}

	// Add the UTF8 BOM as the file header
	UTF8CHAR UTF8BOM[] = { (UTF8CHAR)0xEF, (UTF8CHAR)0xBB, (UTF8CHAR)0xBF };
	CSVArchive->Serialize(&UTF8BOM, UE_ARRAY_COUNT(UTF8BOM) * sizeof(UTF8CHAR));

	SerializeHeader(CSVArchive);
	SerializeReferenceMap(CSVArchive, ReferenceMap, &AssetsInFolder);

	CSVArchive->Close();
	
	return 0;
};