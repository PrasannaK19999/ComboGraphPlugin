// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

using UnrealBuildTool;

public class ComboGraphEditor : ModuleRules
{
	public ComboGraphEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Unreal core
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",

			// Editor framework
			"UnrealEd",
			"EditorFramework",
			"ToolMenus",
			"PropertyEditor",

			// Graph editor — EdGraph, SGraphEditor, pins, connections
			"GraphEditor",
			"BlueprintGraph",

			// Asset pipeline
			"AssetTools",
			"AssetRegistry",
			"ContentBrowser",
			"ContentBrowserData",

			// Validation output
			"MessageLog",

			// Plugin runtime module — UComboGraphDataAsset, FComboNodeData, UComboGraph
			"ComboGraph",

			// Tag support
			"GameplayTags"
		});
	}
}
