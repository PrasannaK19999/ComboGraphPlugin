// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

using UnrealBuildTool;

public class ComboGraphDev : ModuleRules
{
	public ComboGraphDev(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ComboGraph", "GameplayTags" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

	}
}
