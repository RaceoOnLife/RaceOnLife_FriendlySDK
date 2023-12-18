// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CitySampleEditorTarget : TargetRules
{
	public CitySampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("CitySample");
		ExtraModuleNames.Add("CitySampleEditor");
		ExtraModuleNames.Add("CitySampleAnimGraphRuntime");
	}
}
