// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class RaceOnLifeTarget : TargetRules
{
	public RaceOnLifeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;

		ExtraModuleNames.AddRange( new string[] { "RaceOnLife" } );
        //ExtraModuleNames.AddRange( new string[] { "RaceOnLife", "CitySample" } );
    }
}
