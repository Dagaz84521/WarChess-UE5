// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WarChess : ModuleRules
{
	public WarChess(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "Niagara", "EnhancedInput" });
        PrivateDependencyModuleNames.Add("Json"); // Automation fixture import/export.
    }
}
