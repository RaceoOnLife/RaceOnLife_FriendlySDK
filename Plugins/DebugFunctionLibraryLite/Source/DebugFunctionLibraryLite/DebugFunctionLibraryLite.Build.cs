// Copyright 2022 Just2Devs. All Rights Reserved.

using UnrealBuildTool;

public class DebugFunctionLibraryLite : ModuleRules
{
	public DebugFunctionLibraryLite(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
#if UE_4_26_OR_LATER
				"DeveloperSettings"
#endif
			}
		);
	}
}
