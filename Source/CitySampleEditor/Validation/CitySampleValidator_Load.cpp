// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleValidator_Load.h"
#include "HAL/FileManager.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Blueprint/BlueprintSupport.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetCompilingManager.h"
#include "Engine/World.h"
#include "CitySampleEditor/CitySampleEditor.h"
#include "Engine/Level.h"
#include "ShaderCompiler.h"
#include "UObject/UObjectHash.h"

#define LOCTEXT_NAMESPACE "CitySampleValidator"

// This list only ignores log messages that occur while we are reloading an asset that is already in memory
// Should only be used for warnings that occur as a result of having the asset in memory in two different packages
TArray<FString> UCitySampleValidator_Load::InMemoryReloadLogIgnoreList = { TEXT("Enum name collision: '") };

UCitySampleValidator_Load::UCitySampleValidator_Load()
	: Super()
{
}

bool UCitySampleValidator_Load::IsEnabled() const
{
	// Commandlets do not need this validation step as they loaded the content while running.
	return !IsRunningCommandlet() && Super::IsEnabled();
}

bool UCitySampleValidator_Load::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return Super::CanValidateAsset_Implementation(InAsset) && InAsset != nullptr;
}

EDataValidationResult UCitySampleValidator_Load::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	check(InAsset);

	TArray<FString> WarningsAndErrors;
	if (GetLoadWarningsAndErrorsForPackage(InAsset->GetOutermost()->GetName(), WarningsAndErrors))
	{
		for (const FString& WarningOrError : WarningsAndErrors)
		{
			AssetFails(InAsset, FText::FromString(WarningOrError), ValidationErrors);
		}
	}
	else
	{
		AssetFails(InAsset, LOCTEXT("Load_FailedLoad", "Failed to get package load warnings and errors"), ValidationErrors);
	}

	if (GetValidationResult() != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}

	return GetValidationResult();
}

bool UCitySampleValidator_Load::GetLoadWarningsAndErrorsForPackage(const FString& PackageName, TArray<FString>& OutWarningsAndErrors)
{
	check(!PackageName.IsEmpty());
	check(GEngine);

	UPackage* const ExistingPackage = FindPackage(nullptr, *PackageName);

	if (ExistingPackage == GetTransientPackage())
	{
		return true;
	}

	// Skip World or Actor packages
	if (ExistingPackage && UWorld::IsWorldOrExternalActorPackage(ExistingPackage))
	{
		return true;
	}

	// Skip compiled class packages. These should have been weeded out by IAssetRegistry::GetDependencies but weren't
	// for some reason. 
	if (ExistingPackage && PackageName.StartsWith(TEXT("/Script/")) && ExistingPackage->HasAnyPackageFlags(PKG_CompiledIn))
	{
		return true;
	}

	// Commandlets shouldn't load the temporary packages since it involves collecting garbage and may destroy objects higher in the callstack. Loading it the one time is probably good enough
	// Also since commandlets don't use RF_Standalone, this could greatly increase commandlet execution time when loading the same assets over and over
	if (ExistingPackage && !IsRunningCommandlet() && !ExistingPackage->ContainsMap() && !PackageName.EndsWith(TEXT("_BuiltData")))
	{
		// Copy the asset file to the temp directory and load it
		const FString& SrcPackageName = PackageName;
		FString SrcFilename;
		const bool bSourceFileExists = FPackageName::DoesPackageExist(SrcPackageName, &SrcFilename);
		if (bSourceFileExists)
		{
			static int32 PackageIdentifier = 0;
			FString DestPackageName = FString::Printf(TEXT("/Temp/%s_%d"), *FPackageName::GetLongPackageAssetName(ExistingPackage->GetName()), PackageIdentifier++);
			FString DestFilename = FPackageName::LongPackageNameToFilename(DestPackageName, FPaths::GetExtension(SrcFilename, true));
			uint32 CopyResult = IFileManager::Get().Copy(*DestFilename, *SrcFilename);
			if (ensure(CopyResult == COPY_OK))
			{
				// Gather all warnings and errors during the process to determine return value
				UPackage* LoadedPackage = nullptr;
				{
					FCitySampleValidationMessageGatherer::AddIgnorePatterns(InMemoryReloadLogIgnoreList);
					FCitySampleValidationMessageGatherer ScopedMessageGatherer;
					// If we are loading a blueprint, compile the original and load the duplicate with DisableCompileOnLoad, since BPs loaded on the side may not compile if there are circular references involving self
					int32 LoadFlags = LOAD_ForDiff;
					{
						TArray<UObject*> AllExistingObjects;
						GetObjectsWithOuter(ExistingPackage, AllExistingObjects, false);
						TArray<UBlueprint*> AllNonDOBPs;
						for (UObject* Obj : AllExistingObjects)
						{
							UBlueprint* BP = Cast<UBlueprint>(Obj);
							if (BP && !FBlueprintEditorUtils::IsDataOnlyBlueprint(BP))
							{
								AllNonDOBPs.Add(BP);
							}
						}
						if (AllNonDOBPs.Num() > 0)
						{
							LoadFlags |= LOAD_DisableCompileOnLoad;
							for (UBlueprint* BP : AllNonDOBPs)
							{
								check(BP);
								FKismetEditorUtilities::CompileBlueprint(BP);
							}
						}
					}
					LoadedPackage = LoadPackage(NULL, *DestPackageName, LoadFlags);

					// Make sure what we just loaded has finish compiling otherwise we won't be able
					// to reset loaders for the package or verify if errors have been emitted.
					FAssetCompilingManager::Get().FinishAllCompilation();

					for (const FString& LoadWarningOrError : ScopedMessageGatherer.GetAllWarningsAndErrors())
					{
						FString SanitizedMessage = LoadWarningOrError.Replace(*DestFilename, *SrcFilename);
						SanitizedMessage = SanitizedMessage.Replace(*DestPackageName, *SrcPackageName);
						OutWarningsAndErrors.Add(SanitizedMessage);
					}
					FCitySampleValidationMessageGatherer::RemoveIgnorePatterns(InMemoryReloadLogIgnoreList);
				}
				if (LoadedPackage)
				{
					ResetLoaders(LoadedPackage);
					IFileManager::Get().Delete(*DestFilename);
					TArray<UObject*> AllLoadedObjects;
					GetObjectsWithOuter(LoadedPackage, AllLoadedObjects, true);
					for (UObject* Obj : AllLoadedObjects)
					{
						if(Obj->IsRooted())
						{
							continue;
						}
						Obj->ClearFlags(RF_Public | RF_Standalone);
						Obj->SetFlags(RF_Transient);
						if (UWorld* WorldToDestroy = Cast<UWorld>(Obj))
						{
							WorldToDestroy->DestroyWorld(true);
						}
						Obj->MarkAsGarbage();
					}
					GEngine->ForceGarbageCollection(true);
				}
			}
			else
			{
				// Failed to copy the file to the temp folder
				UE_LOG(LogCitySampleEditor, Warning, TEXT("CitySampleValidator_Load: Failed copying file %s to destination %s"), *SrcFilename, *DestFilename);
				return false;
			}
		}
		else
		{
			// It was in memory but not yet saved probably (no source file)
			UE_LOG(LogCitySampleEditor, Warning, TEXT("Package %s source file %s does not exist on disk."), *SrcPackageName, *SrcFilename);
			return false;
		}
	}
	else if(!PackageName.Contains(ULevel::GetExternalActorsFolderName()))
	{
		// Not in memory, just load it
		FCitySampleValidationMessageGatherer ScopedMessageGatherer;
		LoadPackage(nullptr, *PackageName, LOAD_None);
		OutWarningsAndErrors = ScopedMessageGatherer.GetAllWarningsAndErrors();
	}

	return true;
}

#undef LOCTEXT_NAMESPACE