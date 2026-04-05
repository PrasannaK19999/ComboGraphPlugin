// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ComboGraphDev : ModuleRules
{
	public ComboGraphDev(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ComboGraph" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

	}
}
