// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleEditorValidator.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "MessageLogModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/MessageDialog.h"
#include "EditorValidatorSubsystem.h"
#include "Editor.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "CitySampleValidator_Blueprints.h"
#include "CitySampleEditor/CitySampleEditor.h"
// #include "Validation/CitySampleValidator_MaterialFunctions.h"
#include "StudioAnalytics.h"
#include "ShaderCompiler.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "CitySampleEditorValidator"

TArray<FString> UCitySampleEditorValidator::RestrictedFolders = {TEXT("/Game/Cinematics/"), TEXT("/Game/Developers/"), TEXT("/Game/NeverCook/")};
TArray<FString> UCitySampleEditorValidator::TestMapsFolders = { TEXT("/Game/Maps/Test_Maps/"), TEXT("/Game/Athena/Maps/Test/"), TEXT("/Game/Athena/Apollo/Maps/Test/") };
TArray<FString> FCitySampleValidationMessageGatherer::IgnorePatterns;

UCitySampleEditorValidator::UCitySampleEditorValidator()
	: Super()
{
}

void UCitySampleEditorValidator::ValidateCheckedOutContent(bool bInteractive)
{
	FStudioAnalytics::RecordEvent(TEXT("ValidateContent"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		if (bInteractive)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DiscoveringAssets", "Still discovering assets. Try again once it is complete."));
		}
		else
		{
			UE_LOG(LogCitySampleEditor, Display, TEXT("Could not run ValidateCheckedOutContent because asset discovery was still being done."));
		}
		return;
	}

	TArray<FString> ChangedPackageNames;
	TArray<FString> DeletedPackageNames;

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if (ISourceControlModule::Get().IsEnabled())
	{
		// Request the opened files at filter construction time to make sure checked out files have the correct state for the filter
		TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
		UpdateStatusOperation->SetGetOpenedOnly(true);
		SourceControlProvider.Execute(UpdateStatusOperation, EConcurrency::Synchronous);

		TArray<FSourceControlStateRef> CheckedOutFiles = SourceControlProvider.GetCachedStateByPredicate(
			[](const FSourceControlStateRef& State) { return State->IsCheckedOut() || State->IsAdded() || State->IsDeleted(); }
		);

		FString PathPart;
		FString FilenamePart;
		FString ExtensionPart;
		for (const FSourceControlStateRef& FileState : CheckedOutFiles)
		{
			FString Filename = FileState->GetFilename();
			FString PackageName;
			if (FPackageName::IsPackageFilename(Filename) && FPackageName::TryConvertFilenameToLongPackageName(Filename, PackageName))
			{
				if (FileState->IsDeleted())
				{
					DeletedPackageNames.Add(PackageName);
				}
				else
				{
					ChangedPackageNames.Add(PackageName);
				}
			}
		}
	}

	bool bAnyIssuesFound = false;
	TArray<FString> AllWarningsAndErrors;
	{
		if (bInteractive)
		{
 			UCitySampleValidator_Blueprints::SetShouldLoadReferencingBlueprintsInEditor(true);
// 			UCitySampleValidator_MaterialFunctions::SetShouldLoadReferencingMaterialsInEditor(true);

			// Dont let other async compilation warnings be attributed incorrectly to the package that is loading.
			if (FAssetCompilingManager::Get().GetNumRemainingAssets())
			{
				FScopedSlowTask SlowTask(0.f, LOCTEXT("CompilingAssetsBeforeCheckingContentTask", "Finishing asset compilations before checking content..."));
				SlowTask.MakeDialog();
				FAssetCompilingManager::Get().FinishAllCompilation();
			}
		}
		{
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckingContentTask", "Checking content..."));
			SlowTask.MakeDialog();
			if (!ValidatePackages(ChangedPackageNames, DeletedPackageNames, 2000, AllWarningsAndErrors))
			{
				bAnyIssuesFound = true;
			}
		}
		if (bInteractive)
		{
 			UCitySampleValidator_Blueprints::SetShouldLoadReferencingBlueprintsInEditor(false);
// 			UCitySampleValidator_MaterialFunctions::SetShouldLoadReferencingMaterialsInEditor(false);
		}
	}

	{
		FCitySampleValidationMessageGatherer ScopedMessageGatherer;
		if (!ValidateProjectSettings())
		{
			bAnyIssuesFound = true;
		}
		AllWarningsAndErrors.Append(ScopedMessageGatherer.GetAllWarningsAndErrors());
	}

	if (bInteractive)
	{
		const bool bAtLeastOneMessage = (AllWarningsAndErrors.Num() != 0);
		if (bAtLeastOneMessage)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ContentValidationFailed", "!!!!!!! Your checked out content has issues. Don't submit until they are fixed !!!!!!!\r\n\r\nSee the MessageLog and OutputLog for details"));
		}
		else if (bAnyIssuesFound)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ContentValidationFailedWithNoMessages", "No errors or warnings were found, but there was an error return code. Look in the OutputLog and log file for details. You may need engineering help."));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ContentValidationPassed", "All checked out content passed. Nice job."));
		}
	}
}

bool UCitySampleEditorValidator::ValidatePackages(const TArray<FString>& ExistingPackageNames, const TArray<FString>& DeletedPackageNames, int32 MaxPackagesToLoad, TArray<FString>& OutAllWarningsAndErrors)
{
	bool bAnyIssuesFound = false;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> AllPackagesToValidate = ExistingPackageNames;
	for (const FString& DeletedPackageName : DeletedPackageNames)
	{
		UE_LOG(LogCitySampleEditor, Display, TEXT("Adding referencers for deleted package %s to be verified"), *DeletedPackageName);
		TArray<FName> PackageReferencers;
		AssetRegistry.GetReferencers(FName(*DeletedPackageName), PackageReferencers, UE::AssetRegistry::EDependencyCategory::Package);
		for (const FName& Referencer : PackageReferencers)
		{
			const FString ReferencerString = Referencer.ToString();
			if (!DeletedPackageNames.Contains(ReferencerString) && !IsInUncookedFolder(ReferencerString) && !IsInTestMapsFolder(ReferencerString))
			{
				UE_LOG(LogCitySampleEditor, Display, TEXT("    Deleted package referencer %s was added to the queue to be verified"), *ReferencerString);
				AllPackagesToValidate.Add(ReferencerString);
			}
		}
	}

	UE_LOG(LogCitySampleEditor, Display, TEXT("Packages To Validate: %d"), AllPackagesToValidate.Num());
	FMessageLog DataValidationLog("AssetCheck");
	DataValidationLog.NewPage(LOCTEXT("ValidatePackages", "Validate Packages"));

	if (AllPackagesToValidate.Num() > MaxPackagesToLoad)
	{
		// Too much changed to verify, just pass it.
		FString WarningMessage = FString::Printf(TEXT("Assets to validate (%d) exceeded -MaxPackagesToLoad=(%d). Skipping existing package validation."), AllPackagesToValidate.Num(), MaxPackagesToLoad);
		UE_LOG(LogCitySampleEditor, Warning, TEXT("%s"), *WarningMessage);
		OutAllWarningsAndErrors.Add(WarningMessage);
		DataValidationLog.Warning(FText::FromString(WarningMessage));
	}
	else
	{
		// Load all packages that match the file filter string
		TArray<FAssetData> AssetsToCheck;
		for (const FString& PackageName : AllPackagesToValidate)
		{
			if (FPackageName::IsValidLongPackageName(PackageName) && !IsInUncookedFolder(PackageName))
			{
				int32 OldNumAssets = AssetsToCheck.Num();
				AssetRegistry.GetAssetsByPackageName(FName(*PackageName), AssetsToCheck, true);
				if (AssetsToCheck.Num() == OldNumAssets)
				{
					FString WarningMessage;
					// See if the file exists at all. Otherwise, the package contains no assets.
					if (FPackageName::DoesPackageExist(PackageName))
					{
						WarningMessage = FString::Printf(TEXT("Found no assets in package '%s'"), *PackageName);
					}
					else
					{
						if (ISourceControlModule::Get().IsEnabled())
						{
							ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
							FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
							TSharedPtr<ISourceControlState, ESPMode::ThreadSafe> FileState = SourceControlProvider.GetState(PackageFilename, EStateCacheUsage::ForceUpdate);
							if (FileState->IsAdded())
							{
								WarningMessage = FString::Printf(TEXT("Package '%s' is missing from disk. It is marked for add in perforce but missing from your hard drive."), *PackageName);
							}

							if (FileState->IsCheckedOut())
							{
								WarningMessage = FString::Printf(TEXT("Package '%s' is missing from disk. It is checked out in perforce but missing from your hard drive."), *PackageName);
							}
						}

						if (WarningMessage.IsEmpty())
						{
							WarningMessage = FString::Printf(TEXT("Package '%s' is missing from disk."), *PackageName);
						}
					}
					ensure(!WarningMessage.IsEmpty());
					UE_LOG(LogCitySampleEditor, Warning, TEXT("%s"), *WarningMessage);
					OutAllWarningsAndErrors.Add(WarningMessage);
					DataValidationLog.Warning(FText::FromString(WarningMessage));
					bAnyIssuesFound = true;
				}
			}
		}

		if (AssetsToCheck.Num() > 0)
		{
			// Preload all assets to check, so load warnings can be handled separately from validation warnings
			{
				for (const FAssetData& AssetToCheck : AssetsToCheck)
				{
					if (!AssetToCheck.IsAssetLoaded())
					{
						UE_LOG(LogCitySampleEditor, Display, TEXT("Preloading %s..."), *AssetToCheck.GetObjectPathString());

						// Start listening for load warnings
						FCitySampleValidationMessageGatherer ScopedPreloadMessageGatherer;
						
						// Load the asset
						AssetToCheck.GetAsset();

						if (ScopedPreloadMessageGatherer.GetAllWarningsAndErrors().Num() > 0)
						{
							// Repeat all errant load warnings as errors, so other CIS systems can treat them more severely (i.e. Build health will create an issue and assign it to a developer)
							for (const FString& LoadWarning : ScopedPreloadMessageGatherer.GetAllWarnings())
							{
								UE_LOG(LogCitySampleEditor, Error, TEXT("%s"), *LoadWarning);
							}

							OutAllWarningsAndErrors.Append(ScopedPreloadMessageGatherer.GetAllWarningsAndErrors());
							bAnyIssuesFound = true;
						}
					}
				}
			}

			FCitySampleValidationMessageGatherer ScopedMessageGatherer;
			FValidateAssetsSettings Settings;
			FValidateAssetsResults Results;

			Settings.bSkipExcludedDirectories = true;
			Settings.bShowIfNoFailures = true;
			
			const bool bHasInvalidFiles = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>()->ValidateAssetsWithSettings(AssetsToCheck, Settings, Results) > 0;
			if (bHasInvalidFiles || ScopedMessageGatherer.GetAllWarningsAndErrors().Num() > 0)
			{
				OutAllWarningsAndErrors.Append(ScopedMessageGatherer.GetAllWarningsAndErrors());
				bAnyIssuesFound = true;
			}
		}
	}

	return !bAnyIssuesFound;
}

bool UCitySampleEditorValidator::ValidateProjectSettings()
{
	bool bSuccess = true;

	FMessageLog ValidationLog("AssetCheck");

	{
		bool bDeveloperMode = false;
		GConfig->GetBool(TEXT("/Script/PythonScriptPlugin.PythonScriptPluginSettings"), TEXT("bDeveloperMode"), /*out*/ bDeveloperMode, GEngineIni);

		if (bDeveloperMode)
		{
			const FString ErrorMessage(TEXT("The project setting version of Python's bDeveloperMode should not be checked in. Use the editor preference version instead!"));
			UE_LOG(LogCitySampleEditor, Error, TEXT("%s"), *ErrorMessage);
			ValidationLog.Error(FText::AsCultureInvariant(ErrorMessage));
			bSuccess = false;
		}
	}

	return bSuccess;
}

bool UCitySampleEditorValidator::IsInUncookedFolder(const FString& PackageName, FString* OutUncookedFolderName)
{
	for (const FString& RestrictedFolder : RestrictedFolders)
	{
		if (PackageName.StartsWith(RestrictedFolder))
		{
			if (OutUncookedFolderName)
			{
				FString FolderToReport = RestrictedFolder.StartsWith(TEXT("/Game/")) ? RestrictedFolder.RightChop(6) : RestrictedFolder;  
				if (FolderToReport.EndsWith(TEXT("/")))
				{
					*OutUncookedFolderName = FolderToReport.LeftChop(1);
				}
				else
				{
					*OutUncookedFolderName = FolderToReport;
				}
			}
			return true;
		}
	}

	return false;
}

bool UCitySampleEditorValidator::IsInTestMapsFolder(const FString& PackageName, FString* OutTestMapsFolderName)
{
	for (const FString& TestMapsFolder : TestMapsFolders)
	{
		if (PackageName.StartsWith(TestMapsFolder))
		{
			if (OutTestMapsFolderName)
			{
				FString FolderToReport = TestMapsFolder.StartsWith(TEXT("/Game/")) ? TestMapsFolder.RightChop(6) : TestMapsFolder;
				if (FolderToReport.EndsWith(TEXT("/")))
				{
					*OutTestMapsFolderName = FolderToReport.LeftChop(1);
				}
				else
				{
					*OutTestMapsFolderName = TestMapsFolder;
				}
			}
			return true;
		}
	}

	return false;
}

bool UCitySampleEditorValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	if (InAsset)
	{
		FString PackageName = InAsset->GetOutermost()->GetName();
		if (!IsInUncookedFolder(PackageName) && !IsInTestMapsFolder(PackageName))
		{
			return true;
		}
	}
	
	return false;
}

#undef LOCTEXT_NAMESPACE