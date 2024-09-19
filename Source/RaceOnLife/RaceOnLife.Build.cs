using System.IO;
using UnrealBuildTool;

public class RaceOnLife : ModuleRules
{
	public RaceOnLife(ReadOnlyTargetRules Target) : base(Target)
	{
        // default start
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ReplicaNPC",
			"glTFRuntime",
			"CityBuilderRuntime",
            "Avaturn",
			"AdvancedSessions",
			"WebBrowser",
            "Media",
            "LowEntryExtendedStandardLibrary",
			"Foundation",
			"DTMysql",
            "XmlParser",
			"SmoothSyncPlugin",
			"InGameLevelEditor",
			"UMG",
			"TileBasedMinimap",
            "raceonlife_lib"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG" });

        // default end

        // CryptoPP start
        /*
        PublicIncludePaths.AddRange(new string[] {Path.Combine(ModuleDirectory, "../../ThirdParty/CryptoPP")});

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string CryptoPPPath = Path.Combine(ModuleDirectory, "../../ThirdParty/CryptoPP/x64/DLL_Output/Release");
            PublicAdditionalLibraries.Add(Path.Combine(CryptoPPPath, "cryptlib.lib"));
        }
        */
        // CryptoPP end

        // Windows API start

        PublicAdditionalLibraries.Add("Ole32.lib");
        PublicAdditionalLibraries.Add("Propsys.lib");

        PublicDefinitions.Add("WIN32_LEAN_AND_MEAN");
        PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");

        // Windows API end
    }
}
