// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

using UnrealBuildTool;

public class ComboGraph : ModuleRules
{
	public ComboGraph(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"StructUtils"   // UE 5.5+: may be folded into CoreUObject — remove if redundant
		});
	}
}
