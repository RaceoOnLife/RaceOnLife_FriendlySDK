// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleContentValidationCommandlet.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UObjectHash.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "SourceControlHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlState.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"
#include "SourceControlHelpers.h"
#include "../Validation/CitySampleEditorValidator.h"
#include "ShaderCompiler.h"
#include "SourceCodeNavigation.h"
#include "Stats/StatsMisc.h"
#include "Engine/BlueprintCore.h"
#include "Blueprint/BlueprintSupport.h"

DEFINE_LOG_CATEGORY_STATIC(LogCitySampleContentValidation, Log, Log);

int32 GMaxAssetsChangedByAHeader = 200;
static FAutoConsoleVariableRef CVarMaxAssetsChangedByAHeader(TEXT("CitySampleContentValidationCommandlet.MaxAssetsChangedByAHeader"), GMaxAssetsChangedByAHeader, TEXT("The maximum number of assets to check for content validation based on a single header change."), ECVF_Default);

class FScopedContentValidationMessageGatherer : public FOutputDevice
{
public:

	FScopedContentValidationMessageGatherer()
		: bAtLeastOneError(false)
	{
		GLog->AddOutputDevice(this);
	}

	~FScopedContentValidationMessageGatherer()
	{
		GLog->RemoveOutputDevice(this);
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		if (Verbosity <= ELogVerbosity::Error)
		{
			bAtLeastOneError = true;
		}
	}

	bool bAtLeastOneError;
};

UCitySampleContentValidationCommandlet::UCitySampleContentValidationCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UCitySampleContentValidationCommandlet::Main(const FString& FullCommandLine)
{
	UE_LOG(LogCitySampleContentValidation, Display, TEXT("Running CitySampleContentValidation commandlet..."));
	
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*FullCommandLine, Tokens, Switches, Params);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	int32 ReturnVal = 0;

	// Use to exclude directories when retrieving changed files
	TArray<FString> ExcludedDirectories;
	FString* ExcludedDirectoriesString = Params.Find(TEXT("ExcludedDirectories"));
	if (ExcludedDirectoriesString && !ExcludedDirectoriesString->IsEmpty())
	{
		ExcludedDirectoriesString->ParseIntoArrayWS(ExcludedDirectories);
	}

	TArray<FString> ChangedPackageNames;
	TArray<FString> DeletedPackageNames;
	TArray<FString> ChangedCode;
	TArray<FString> ChangedOtherFiles;

	// Use to validate all files changed based on a p4 filter
	// e.g. "-P4Filter=//MyGame/Main/Game/...@500,505"
	FString* P4FilterString = Params.Find(TEXT("P4Filter"));
	if (P4FilterString && !P4FilterString->IsEmpty())
	{
		FString P4CmdString = TEXT("files ") + *P4FilterString;
		if (!GetAllChangedFiles(AssetRegistry, P4CmdString, ExcludedDirectories, ChangedPackageNames, DeletedPackageNames, ChangedCode, ChangedOtherFiles))
		{
			UE_LOG(LogCitySampleContentValidation, Display, TEXT("ContentValidation returning 1. Failed to get changed files."));
			ReturnVal = 1;
		}
	}

	// Use to validate all files changed based on a specific P4 changelist
	// e.g. "-P4Changelist=505"
	FString* P4ChangelistString = Params.Find(TEXT("P4Changelist"));
	if (P4ChangelistString && !P4ChangelistString->IsEmpty())
	{
		FString P4CmdString = TEXT("opened -c ") + *P4ChangelistString;
		if (!GetAllChangedFiles(AssetRegistry, P4CmdString, ExcludedDirectories, ChangedPackageNames, DeletedPackageNames, ChangedCode, ChangedOtherFiles))
		{
			UE_LOG(LogCitySampleContentValidation, Display, TEXT("ContentValidation returning 1. Failed to get changed files."));
			ReturnVal = 1;
		}
	}

	// Use to validate all files checked out
	bool bP4Opened = Switches.Contains(TEXT("P4Opened"));
	if (bP4Opened)
	{
		check(GConfig);

		const FString& SSCIniFile = SourceControlHelpers::GetSettingsIni();
		FString Workspace;
		GConfig->GetString(TEXT("PerforceSourceControl.PerforceSourceControlSettings"), TEXT("Workspace"), Workspace, SSCIniFile);
		if (!Workspace.IsEmpty())
		{
			FString P4CmdString = FString::Printf(TEXT("-c%s opened"), *Workspace);
			if (!GetAllChangedFiles(AssetRegistry, P4CmdString, ExcludedDirectories, ChangedPackageNames, DeletedPackageNames, ChangedCode, ChangedOtherFiles))
			{
				UE_LOG(LogCitySampleContentValidation, Display, TEXT("ContentValidation returning 1. Failed to get changed files."));
				ReturnVal = 1;
			}
		}
		else
		{
			UE_LOG(LogCitySampleContentValidation, Error, TEXT("P4 workspace was not found when using P4Opened"));
			UE_LOG(LogCitySampleContentValidation, Display, TEXT("ContentValidation returning 1. Workspace not found."));
			ReturnVal = 1;
		}
	}

	int32 MaxPackagesToLoad = 2000;

	FString* InPathString = Params.Find(TEXT("InPath"));
	if (InPathString && !InPathString->IsEmpty())
	{
		GetAllPackagesInPath(AssetRegistry, *InPathString, ChangedPackageNames);
	}

	FString* OfTypeString = Params.Find(TEXT("OfType"));
	if (OfTypeString && !OfTypeString->IsEmpty())
	{
		const int32 InitialPackages = ChangedPackageNames.Num();
		GetAllPackagesOfType(*OfTypeString, ChangedPackageNames);
		MaxPackagesToLoad += ChangedPackageNames.Num() - InitialPackages;
	}

	FString* SpecificPackagesString = Params.Find(TEXT("Packages"));
	if (SpecificPackagesString && !SpecificPackagesString->IsEmpty())
	{
		TArray<FString> PackagePaths;
		SpecificPackagesString->ParseIntoArray(PackagePaths, TEXT("+"));
		ChangedPackageNames.Append(PackagePaths);
	}

	// We will be flushing shader compile as we load materials, so dont let other shader warnings be attributed incorrectly to the package that is loading.
	if (GShaderCompilingManager)
	{
		GShaderCompilingManager->FinishAllCompilation();
	}

	FString* InMaxPackagesToLoadString = Params.Find(TEXT("MaxPackagesToLoad"));
	if (InMaxPackagesToLoadString)
	{
		MaxPackagesToLoad = FCString::Atoi(**InMaxPackagesToLoadString);
	}

	TArray<FString> AllWarningsAndErrors;
	UCitySampleEditorValidator::ValidatePackages(ChangedPackageNames, DeletedPackageNames, MaxPackagesToLoad, AllWarningsAndErrors);

	if (!UCitySampleEditorValidator::ValidateProjectSettings())
	{
		ReturnVal = 1;
	}

	return ReturnVal;
}


bool UCitySampleContentValidationCommandlet::GetAllChangedFiles(IAssetRegistry& AssetRegistry, const FString& P4CmdString, const TArray<FString>& ExcludedDirectories, TArray<FString>& OutChangedPackageNames, TArray<FString>& DeletedPackageNames, TArray<FString>& OutChangedCode, TArray<FString>& OutChangedOtherFiles) const
{
	TArray<FString> Results;
	int32 ReturnCode = 0;
	if (LaunchP4(P4CmdString, Results, ReturnCode))
	{
		if (ReturnCode == 0)
		{
			if (!ExcludedDirectories.IsEmpty())
			{
				UE_LOG(LogCitySampleContentValidation, Display, TEXT("Excluded Directories:"));
				for (const FString& ExcludedDirectory : ExcludedDirectories)
				{
					UE_LOG(LogCitySampleContentValidation, Display, TEXT("    - %s"), *ExcludedDirectory);
				}
			}

			int32 NumFilesExcluded = 0;
			for (const FString& Result : Results)
			{
				FString DepotPathName;
				FString ExtraInfoAfterPound;
				if (Result.Split(TEXT("#"), &DepotPathName, &ExtraInfoAfterPound))
				{
					FString PostCitySamplePath;
					if (DepotPathName.Split(TEXT("Collaboration/CitySample/CitySample/"), nullptr, &PostCitySamplePath))
					{
						const auto DirectoryIsExcluded = [&PostCitySamplePath](const FString& Directory) { return PostCitySamplePath.StartsWith(Directory); };
						if (ExcludedDirectories.FindByPredicate(DirectoryIsExcluded))
						{
							++NumFilesExcluded;
							continue;
						}
					}

					if (DepotPathName.EndsWith(TEXT(".uasset")) || DepotPathName.EndsWith(TEXT(".umap")))
					{
						FString FullPackageName;
						{
							// Check for /Game/ assets
							FString PostContentPath;
							if (DepotPathName.Split(TEXT("CitySample/Content/"), nullptr, &PostContentPath))
							{
								if (!PostContentPath.IsEmpty())
								{
									const FString PostContentPathWithoutExtension = FPaths::GetBaseFilename(PostContentPath, false);
									FString PackageNameToTest = TEXT("/Game/") + PostContentPathWithoutExtension;
									if (!UCitySampleEditorValidator::IsInUncookedFolder(PackageNameToTest) && !UCitySampleEditorValidator::IsInTestMapsFolder(PackageNameToTest))
									{
										FullPackageName = PackageNameToTest;
									}
								}
							}
						}
						
						if (FullPackageName.IsEmpty())
						{
							// Check for plugin assets
							FString PostPluginsPath;
							if (DepotPathName.Split(TEXT("CitySample/Plugins/"), nullptr, &PostPluginsPath))
							{
								const int32 ContentFolderIdx = PostPluginsPath.Find(TEXT("/Content/"));
								if (ContentFolderIdx != INDEX_NONE)
								{
									int32 PluginFolderIdx = PostPluginsPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, ContentFolderIdx - 1);
									if (PluginFolderIdx == INDEX_NONE)
									{
										// No leading /. Directly in the /Plugins/ folder
										PluginFolderIdx = 0;
									}
									else
									{
										// Skip the leading /. A subfolder in the /Plugins/ folder
										PluginFolderIdx++;
									}
									
									const int32 PostContentFolderIdx = ContentFolderIdx + FCString::Strlen(TEXT("/Content/"));
									const FString PostContentPath = PostPluginsPath.RightChop(PostContentFolderIdx);
									const FString PluginName = PostPluginsPath.Mid(PluginFolderIdx, ContentFolderIdx - PluginFolderIdx);
									if (!PostContentPath.IsEmpty() && !PluginName.IsEmpty())
									{
										TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
										if (Plugin.IsValid() && Plugin->IsEnabled())
										{
											const FString PostContentPathWithoutExtension = FPaths::GetBaseFilename(PostContentPath, false);
											FullPackageName = FString::Printf(TEXT("/%s/%s"), *PluginName, *PostContentPathWithoutExtension);
										}
									}
								}
							}
						}

						if (!FullPackageName.IsEmpty())
						{
							if (ExtraInfoAfterPound.Contains(TEXT("delete")))
							{
								DeletedPackageNames.AddUnique(FullPackageName);
							}
							else
							{
								OutChangedPackageNames.AddUnique(FullPackageName);
							}
						}
					}
					else
					{
						if (!PostCitySamplePath.IsEmpty())
						{
							if (DepotPathName.EndsWith(TEXT(".cpp")))
							{
								OutChangedCode.Add(PostCitySamplePath);
							}
							else if (DepotPathName.EndsWith(TEXT(".h")))
							{
								OutChangedCode.Add(PostCitySamplePath);
								UCitySampleContentValidationCommandlet::GetChangedAssetsForCode(AssetRegistry, DepotPathName, OutChangedPackageNames);
							}
							else
							{
								OutChangedOtherFiles.Add(PostCitySamplePath);
							}
						}
					}
				}
			}

			UE_LOG(LogCitySampleContentValidation, Display, TEXT("Changed Files: %d, Excluded Files: %d"), Results.Num(), NumFilesExcluded);
			return true;
		}
		else
		{
			UE_LOG(LogCitySampleContentValidation, Error, TEXT("p4 returned non-zero return code %d"), ReturnCode);
		}
	}

	return false;
}

void UCitySampleContentValidationCommandlet::GetAllPackagesInPath(IAssetRegistry& AssetRegistry, const FString& InPathString, TArray<FString>& OutPackageNames) const
{
	TArray<FString> Paths;
	InPathString.ParseIntoArray(Paths, TEXT("+"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = true;

	for (const FString& Path : Paths)
	{
		Filter.PackagePaths.Add(FName(*Path));
	}

	TArray<FAssetData> AssetsInPaths;
	if (AssetRegistry.GetAssets(Filter, AssetsInPaths))
	{
		for (const FAssetData& AssetData : AssetsInPaths)
		{
			OutPackageNames.Add(AssetData.PackageName.ToString());
		}
	}
}

void UCitySampleContentValidationCommandlet::GetAllPackagesOfType(const FString& OfTypeString, TArray<FString>& OutPackageNames) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> Types;
	OfTypeString.ParseIntoArray(Types, TEXT("+"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = true;

	for (const FString& Type : Types)
	{
		FTopLevelAssetPath TypePathName = UClass::TryConvertShortTypeNameToPathName<UStruct>(Type, ELogVerbosity::Error, TEXT("UCitySampleContentValidationCommandlet"));
		if (TypePathName.IsNull())
		{
			UE_LOG(LogCitySampleContentValidation, Error, TEXT("Failed to convert short class name \"%s\" to path name. Please use class path names."), *Type);
		}
		else
		{
			Filter.ClassPaths.Add(TypePathName);
		}
	}

	TArray<FAssetData> AssetsOfType;
	if (AssetRegistry.GetAssets(Filter, AssetsOfType))
	{
		for (const FAssetData& AssetData : AssetsOfType)
		{
			OutPackageNames.Add(AssetData.PackageName.ToString());
		}
	}
}

bool UCitySampleContentValidationCommandlet::LaunchP4(const FString& Args, TArray<FString>& Output, int32& OutReturnCode) const
{
	void* PipeRead = nullptr;
	void* PipeWrite = nullptr;

	verify(FPlatformProcess::CreatePipe(PipeRead, PipeWrite));

	bool bInvoked = false;
	OutReturnCode = -1;
	FString StringOutput;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(TEXT("p4.exe"), *Args, false, true, true, nullptr, 0, nullptr, PipeWrite);
	if (ProcHandle.IsValid())
	{
		while (FPlatformProcess::IsProcRunning(ProcHandle))
		{
			StringOutput += FPlatformProcess::ReadPipe(PipeRead);
			FPlatformProcess::Sleep(0.1f);
		}

		StringOutput += FPlatformProcess::ReadPipe(PipeRead);
		FPlatformProcess::GetProcReturnCode(ProcHandle, &OutReturnCode);
		bInvoked = true;
	}
	else
	{
		UE_LOG(LogCitySampleContentValidation, Error, TEXT("Failed to launch p4."));
	}

	FPlatformProcess::ClosePipe(PipeRead, PipeWrite);

	StringOutput.ParseIntoArrayLines(Output);

	return bInvoked;
}

void UCitySampleContentValidationCommandlet::GetChangedAssetsForCode(IAssetRegistry& AssetRegistry, const FString& InChangedHeader, TArray<FString>& OutChangedPackageNames) const
{
	static struct FCachedNativeClasses
	{
	public:
		FCachedNativeClasses()
		{
			static const FName ModuleNameFName = "ModuleName";
			static const FName ModuleRelativePathFName = "ModuleRelativePath";

			for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
			{
				UClass* TestClass = *ClassIt;
				if (TestClass->HasAnyClassFlags(CLASS_Native))
				{
					FAssetData ClassAssetData(TestClass);

					FString ModuleName, ModuleRelativePath;
					ClassAssetData.GetTagValue(ModuleNameFName, ModuleName);
					ClassAssetData.GetTagValue(ModuleRelativePathFName, ModuleRelativePath);

					Classes.Add(ModuleName + TEXT("+") + ModuleRelativePath, TestClass);
				}
			}
		}

		TArray<TWeakObjectPtr<UClass>> GetClassesInHeader(const FString& ModuleName, const FString& ModuleRelativePath)
		{
			TArray<TWeakObjectPtr<UClass>> ClassesInHeader;
			Classes.MultiFind(ModuleName + TEXT("+") + ModuleRelativePath, ClassesInHeader);

			return ClassesInHeader;
		}

	private:
		TMultiMap<FString, TWeakObjectPtr<UClass>> Classes;
	} NativeClassCache;

	const FString& SSCIniFile = SourceControlHelpers::GetSettingsIni();
	FString Workspace;
	GConfig->GetString(TEXT("PerforceSourceControl.PerforceSourceControlSettings"), TEXT("Workspace"), Workspace, SSCIniFile);
	
	if (Workspace.IsEmpty())
	{
		FString ParameterValue;
		if (FParse::Value(FCommandLine::Get(), TEXT("P4Client="), ParameterValue))
		{
			Workspace = ParameterValue;
		}
	}

	if (!Workspace.IsEmpty())
	{
		TArray<FString> WhereResults;
		int32 ReturnCode = 0;
		FString P4WhereCommand = FString::Printf(TEXT("-ztag -c%s where %s"), *Workspace, *InChangedHeader);
		if (LaunchP4(P4WhereCommand, WhereResults, ReturnCode))
		{
			if (WhereResults.Num() >= 2)
			{
				FString ChangedHeaderPath = WhereResults[2];
				ChangedHeaderPath.RemoveFromStart(TEXT("... path "));
				FPaths::NormalizeFilename(ChangedHeaderPath);

				const TArray<FString>& ModuleNames = FSourceCodeNavigation::GetSourceFileDatabase().GetModuleNames();
				const FString* Module = ModuleNames.FindByPredicate([ChangedHeaderPath](const FString& ModuleBuildPath) {
					const FString ModuleFullPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(ModuleBuildPath));
					if (ChangedHeaderPath.StartsWith(ModuleFullPath))
					{
						return true;
					}
					return false;
				});

				if (Module)
				{
					SCOPE_LOG_TIME_IN_SECONDS(TEXT("Looking for blueprints affected by code changes"), nullptr);

					const FString FoundModulePath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(*Module));
					const FString FoundModulePathWithSlash = FoundModulePath / TEXT("");
					FString ChangedHeaderReleativeToModule = ChangedHeaderPath;
					FPaths::MakePathRelativeTo(ChangedHeaderReleativeToModule, *FoundModulePathWithSlash);
					FString ChangedHeaderModule = FPaths::GetBaseFilename(FoundModulePath);

					// STEP 1 - Find all the native classes inside the header that changed.
					TArray<TWeakObjectPtr<UClass>> ClassList = NativeClassCache.GetClassesInHeader(ChangedHeaderModule, ChangedHeaderReleativeToModule);

					// STEP 2 - We now need to convert the set of native classes into actual derived blueprints.
					bool bTooManyFiles = false;
					TArray<FAssetData> BlueprintsDerivedFromNativeModifiedClasses;
					for (TWeakObjectPtr<UClass> ModifiedClassPtr : ClassList)
					{
						// If we capped out on maximum number of modified files for a single header change, don't try to keep looking for more stuff.
						if (bTooManyFiles)
						{
							break;
						}

						if (UClass* ModifiedClass = ModifiedClassPtr.Get())
						{
							// This finds all native derived blueprints, both direct subclasses, or subclasses of subclasses.
							TSet<FTopLevelAssetPath> DerivedClassNames;
							TArray<FTopLevelAssetPath> ClassNames;
							ClassNames.Add(ModifiedClass->GetClassPathName());
							AssetRegistry.GetDerivedClassNames(ClassNames, TSet<FTopLevelAssetPath>(), DerivedClassNames);

							UE_LOG(LogCitySampleContentValidation, Display, TEXT("Validating Subclasses of %s in %s + %s"), *ModifiedClass->GetName(), *ChangedHeaderModule, *ChangedHeaderReleativeToModule);

							FARFilter Filter;
							Filter.bRecursiveClasses = true;
							Filter.ClassPaths.Add(UBlueprintCore::StaticClass()->GetClassPathName());

							// We enumerate all assets to find any blueprints who inherit from native classes directly - or
							// from other blueprints.
							AssetRegistry.EnumerateAssets(Filter, [&BlueprintsDerivedFromNativeModifiedClasses, &bTooManyFiles, &DerivedClassNames, ChangedHeaderModule, ChangedHeaderReleativeToModule](const FAssetData& AssetData)
							{
								// Don't check data-only blueprints, we'll be here all day.
								if (!AssetData.GetTagValueRef<bool>(FBlueprintTags::IsDataOnly))
								{
									// Need to get the generated class here to see if it's one in the derived set we care about.
									const FString ClassFromData = AssetData.GetTagValueRef<FString>(FBlueprintTags::GeneratedClassPath);
									if (!ClassFromData.IsEmpty())
									{
										const FTopLevelAssetPath ClassObjectPath(FPackageName::ExportTextPathToObjectPath(ClassFromData));
										if (DerivedClassNames.Contains(ClassObjectPath))
										{
											UE_LOG(LogCitySampleContentValidation, Display, TEXT("\tAdding %s To Validate"), *AssetData.PackageName.ToString());

											BlueprintsDerivedFromNativeModifiedClasses.Emplace(AssetData);

											if (BlueprintsDerivedFromNativeModifiedClasses.Num() >= GMaxAssetsChangedByAHeader)
											{
												bTooManyFiles = true;
												UE_LOG(LogCitySampleContentValidation, Display, TEXT("Too many assets invalidated (Max %d) by change to, %s + %s"), GMaxAssetsChangedByAHeader, *ChangedHeaderModule, *ChangedHeaderReleativeToModule);
												return false; // Stop enumerating.
											}
										}
									}
								}
								return true;
							});
						}
					}

					// STEP 3 - Report the possibly changed blueprints as affected modified packages that need
					// to be proved out.
					for (const FAssetData& BlueprintsDerivedFromNativeModifiedClass : BlueprintsDerivedFromNativeModifiedClasses)
					{
						OutChangedPackageNames.Add(BlueprintsDerivedFromNativeModifiedClass.PackageName.ToString());
					}
				}
			}
			else
			{
				UE_LOG(LogCitySampleContentValidation, Warning, TEXT("GetChangedAssetsForCode failed to run p4 'where'. WhereResults[0] = '%s'. Not adding any validation for %s"), WhereResults.Num() > 0 ? *WhereResults[0] : TEXT("Invalid"), *InChangedHeader);
			}
		}
	}
}
