// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;

public class CitySampleTarget : TargetRules
{
	public CitySampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("CitySample");
	
		if (BuildEnvironment == TargetBuildEnvironment.Unique)
		{
			bUseLoggingInShipping = true;
		}
	}
}
