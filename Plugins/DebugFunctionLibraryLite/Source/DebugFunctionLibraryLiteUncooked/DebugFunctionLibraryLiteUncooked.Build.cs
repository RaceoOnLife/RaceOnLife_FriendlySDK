// Copyright 2022 Just2Devs. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class DebugFunctionLibraryLiteUncooked : ModuleRules
{
	private string PluginPath
	{
		get { return Path.Combine(PluginDirectory, "Source/");  }
	}
	
	public DebugFunctionLibraryLiteUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteUncooked/Public/K2Nodes"),
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteUncooked/Public"),
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteUncooked"),
			});
		
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteUncooked/Private/K2Nodes"),
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteUncooked/Private"),
				Path.Combine(PluginPath, "DebugFunctionLibraryLiteUncooked"),
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
				"DebugFunctionLibraryLite"
			}
			);
	}
}
