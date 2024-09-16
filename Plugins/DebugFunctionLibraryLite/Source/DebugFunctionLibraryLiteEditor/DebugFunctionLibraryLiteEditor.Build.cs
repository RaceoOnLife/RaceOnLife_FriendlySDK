// Copyright 2022 Just2Devs. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class DebugFunctionLibraryLiteEditor : ModuleRules
{
	private string PluginPath
	{
		get { return Path.Combine(PluginDirectory, "Source/");  }
	}
	
	public DebugFunctionLibraryLiteEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteEditor/Public"),
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteEditor"),
			});
		
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteEditor/Private"),
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteEditor"),
			});
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"Kismet", 
				"SlateCore", 
				"Projects"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"KismetCompiler",
				"BlueprintGraph",
				"UnrealEd",
				"ToolMenus",
				"EditorStyle",
				"InputCore",
				"GraphEditor",
				"DebugFunctionLibraryLiteUncooked",
				"DebugFunctionLibraryLite"
			}
			);
	}
}
