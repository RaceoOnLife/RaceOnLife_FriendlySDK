// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class RaceOnLifeEditorTarget : TargetRules
{
	public RaceOnLifeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;

		ExtraModuleNames.AddRange( new string[] { "RaceOnLife" } );
		//ExtraModuleNames.Add("CitySample");
		//ExtraModuleNames.Add("CitySampleEditor");
		//ExtraModuleNames.Add("CitySampleAnimGraphRuntime");

		if (Type == TargetType.Editor && Platform.IsInGroup(UnrealPlatformGroup.Linux) && LinuxPlatform.bEnableThreadSanitizer)
		{
			string[] TSanDisabledPlugins = 
			{
				"NeuralNetworkInference",
				"RemoteControl",
				"Text3D",
			};

			foreach (string PluginName in TSanDisabledPlugins)
			{
				DisablePlugins.Add(PluginName);
				EnablePlugins.Remove(PluginName);
			}
		}
	}
}
